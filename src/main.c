#define _DEFAULT_SOURCE

#include "jrutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef __linux__
	#include <linux/limits.h>
#elif __APPLE__
	#include <sys/syslimits.h>
#else
	#include <limits.h>
#endif

#include "util.h"
#include "color.h"
#ifdef __APPLE__
	#define C_NUM YELLOW
	#define C_TEST CYAN
	#define C_YES GREEN
	#define C_NO RED
	#define C_MSG PURPLE
#else
	#define C_NUM YELLOW
	#define C_TEST LCYAN
	#define C_YES LGREEN
	#define C_NO LRED
	#define C_MSG LPURPLE
#endif

#define DEFAULT_DIR "tests"
#define DEFAULT_EXT ".test.out"
#define DELIM ':'

#define HASHMAP_INIT_SIZE 1024

int main(int argc, char *argv[]){

	char* dir = getenv("UNIT_DIR");
	if (dir == NULL)
		dir = DEFAULT_DIR;

	char* ext = getenv("UNIT_EXT");
	if (ext == NULL)
		ext = DEFAULT_EXT;

	switch(argc){
		default:
		case 3:
			ext = argv[2];
		case 2:
			if (!strcmp(argv[1], "-h") ||
					!strcmp(argv[1], "--help")) usage(0);
			else dir = argv[1];
		case 1:
			break;
	}
	const size_t dir_len = strlen(dir);

	int ext_count = fndelims(ext, DELIM) + 1;

	char* exts[ext_count];
	if (splitn(exts, ext_count, ext, strlen(ext), DELIM)){
		fprintf(stderr,
			"Could not split string %d\n", __LINE__);
		exit(1);
	}
	int ext_sizes[ext_count];
	for (int i = 0; i<ext_count; i++){
		ext_sizes[i] = strlen(exts[i]);
	}

	const size_t map_cap = HASHMAP_INIT_SIZE;
	const char** files = mmap(NULL, map_cap*sizeof(char*),
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	// find amount of matching files
	struct dirent* de;
	DIR* dr = opendir(dir);
	if (dr == NULL)
		errndie(1, "%s", dir);
	int elems = 0;
	while ((de = readdir(dr)) != NULL){
		for (int i = 0; i<ext_count; i++){
			if (de->d_type != DT_DIR && de->d_name[0] != '.'){
				if (endcmp(de->d_name, exts[i]) == 0){
					if (jrmap_find(files, map_cap, de->d_name, strlen(de->d_name)) == -1){
						char* copy = strdup(de->d_name);
						if (copy == NULL)
							errndie(1, "Could not copy string");
						if (jrmap_add(files, map_cap, copy, strlen(copy)) == -1 )
							errndie(1, "Could not add string '%s' to hashmap", copy);
						elems++;
					}
				}
			}
		}
	}
	closedir(dr);

	if (elems == 0){
		fprintf(stderr, "There are no ");
		for (int i = 0; i<ext_count; i++){
			fprintf(stderr, "%s, ", exts[i]);
		}
		fprintf(stderr, "\b\b files in %s.\n", dir);
		exit(1);
	}

	char** program = mmap(NULL, elems*sizeof(char*),
						PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	for (size_t i = 0; i<elems; i++){
		program[i] = mmap(NULL, PATH_MAX*sizeof(char),
						PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}

	// filepath for mathing files
	for (size_t i = 0, j = 0; i<map_cap; i++){
		if (files[i]){
			snprintf(program[j++], PATH_MAX, "%s/%s",
					dir, files[i]);
		}
	}

	int* exit_code = mmap(NULL, elems*sizeof(int),
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	pid_t* pids = mmap(NULL, elems*sizeof(pid_t),
					PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	char logs_dir_path[PATH_MAX];
	{
		snprintf(logs_dir_path, PATH_MAX, "%s/%s/logs",
				getenv("PWD"), dir);
		struct stat st = {0};
		if (stat(logs_dir_path, &st) == -1){
			mkdir(logs_dir_path, S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}

	if (dir[0] != '/')
		printf("\nRunning " C_NUM "%d" NC " tests in %s/%s\n\n",
				elems, getenv("PWD"), dir);
	else
		printf("\nRunning " C_NUM "%d" NC " tests in %s\n\n",
				elems, dir);
	pid_t pid = 0;
	for (int i = 0; i<elems; i++){
		spoon(){
			spoon(){
				char log[PATH_MAX*sizeof(char)];
				snprintf(log, PATH_MAX, "%s/%s.log",
						logs_dir_path, program[i]+dir_len+1);
				fflush(stdout);
				fflush(stderr);
				freopen(log, "w", stdout);
				freopen(log, "w", stderr);
				argv[0] = program[i];
				if (execvp(program[i], argv)){
					fprintf(stderr, "Could not execute %s\n",
							program[i]);
					exit(126);
				}
			}
			int status;
			wait(&status);
			if (WIFEXITED(status)){
				exit_code[i] = WEXITSTATUS(status);
				printf(C_NUM "%d/%d\t" C_TEST "%s:\t" NC,
						i+1, elems, program[i]);
				if (exit_code[i])
					printf(C_NO "Failure " NC "(%d)\n" NC,
							exit_code[i]);
				else
					printf(C_YES "Success\n" NC);
			}
			exit(0);
		}
		pids[i] = pid;
	}

	int success = 0;
	for (int i = 0; i<elems; i++){
		waitpid(pids[i], NULL, 0);
		if (exit_code[i] == 0)
			success++;
	}
	printf("\n");

	printf("status: %s " NC
		"| tests run: " C_NUM "%d " NC
		"| failure: " C_NUM "%d " NC "(" "%d%%) "
		"| success: " C_NUM "%d " NC "(%d%%)\n"
		"\n",
		elems == success ? C_YES "OK" : C_NO "FAIL",
		elems,
		elems-success, (((elems-success)*100)/elems),
		success, ((success*100)/elems));

#ifdef PEDANTIC
	// free all the memory
	for (int i = 0; i<map_cap; i++){
		free(files[i]);
	}
	munmap(files, map_cap*sizeof(char*));
	munmap(pids, elems*sizeof(pid_t));
	munmap(exit_code, elems*sizeof(*exit_code));
	for (int i = 0; i<elems; i++){
		munmap(program[i], PATH_MAX*sizeof(**program));
	}
	munmap(program, elems*sizeof(*program));
#endif
	return 0;
}

static _Noreturn void usage(int ret){
	printf("USAGE: %s [directory | options] [extension]\n"
		"\n"
		"SETTINGS:\n"
		"\tdirectory (UNIT_DIR) = %s\n"
		"\textension (UNIT_EXT) = %s\n"
		"\n"
		"OPTIONS:\n"
		"\t-h --help\tShow help message\n"
		"\n"
		, getenv("_"), getenv("UNIT_DIR"), getenv("UNIT_EXT")
	);
	exit(ret);
}
