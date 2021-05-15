#define _DEFAULT_SOURCE

#include <jrutil/jrutil.h>

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

#define MAXFILES 1024
#define HASHMAP_SIZE MAXFILES*2

#define NO_MEMORY_ALLOCATION

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

#ifdef NO_MEMORY_ALLOCATION
	static char* filesmap[HASHMAP_SIZE];
	static char filepaths[MAXFILES][PATH_MAX];
#else
	char** filesmap = mmap(NULL, HASHMAP_SIZE*sizeof(char*),
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	char** filepaths = mmap(NULL, MAXFILES*sizeof(char*),
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	for (size_t i = 0; i<MAXFILES; i++){
		filepaths[i] = mmap(NULL, PATH_MAX*sizeof(char),
							PROT_READ | PROT_WRITE,
							MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}
#endif

	// find matching files
	struct dirent* de;
	DIR* dr = opendir(dir);
	if (dr == NULL)
		errndie(1, "%s", dir);
	int elems = 0;
	while ((de = readdir(dr)) != NULL){
		for (int i = 0; i<ext_count; i++){
			if (de->d_type != DT_DIR && de->d_name[0] != '.'){
				if (endcmp(de->d_name, exts[i]) == 0){
					size_t len = snprintf(filepaths[elems], PATH_MAX, "%s/%s", dir, de->d_name);
					if (jrmap_find((const void**)filesmap, HASHMAP_SIZE, filepaths[elems], len) == -1){
						if (jrmap_add((const void**)filesmap, HASHMAP_SIZE, filepaths[elems], len) == -1)
							errndie(1, "Could not add string '%s' to hashmap", filepaths[elems]);
						if (elems++ >= MAXFILES)
							errndie(1, "Exceeded max files limit");
					}
					#ifdef PEDANTIC
					else {
						memset(filepaths[elems], 0, len*sizeof(char)); // overwrite prematurely added string
					}
					#endif
				}
			}
		}
	}
	closedir(dr);

	if (elems == 0){
		fprintf(stderr, "There are no ");
		if (ext_sizes[0] != 0){
			for (int i = 0; i<ext_count; i++){
				fprintf(stderr, "%s, ", exts[i]);
			}
			fprintf(stderr, "\b\b ");
		}
		fprintf(stderr, "tests in '" C_TEST "%s" NC "'.\n", dir);
		return 0;
	}

#ifdef NO_MEMORY_ALLOCATION
	static pid_t pids[MAXFILES];
#else
	pid_t* pids = mmap(NULL, elems*sizeof(pid_t),
					PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#endif
	
	char* logs_dir_path = "logs";
	{
		struct stat st = {0};
		if (stat(logs_dir_path, &st) == -1){
			mkdir(logs_dir_path, S_IRWXU | S_IRWXG | S_IRWXO);
	}
	}

	printf("\nRunning " C_NUM "%d " NC
			"tests in '" C_TEST "%s" NC "'.\n\n",
			elems, dir);

	pid_t pid = 0;
	for (int i = 0; i<elems; i++){
		spoon(){
			spoon(){
				char log[PATH_MAX*sizeof(char)];
				snprintf(log, PATH_MAX, "%s/%s.log",
						logs_dir_path, filepaths[i]+dir_len+1);
				fflush(stdout);
				fflush(stderr);
				freopen(log, "w", stdout);
				freopen(log, "w", stderr);
				argv[0] = filepaths[i];
				if (execvp(filepaths[i], argv)){
					fprintf(stderr, "Could not execute %s\n",
							filepaths[i]);
					exit(126);
				}
			}
			int status;
			wait(&status);
			if (WIFEXITED(status)){
				status = WEXITSTATUS(status);
				printf(C_NUM "%d/%d\t" C_TEST "%s" NC ":\t",
						i+1, elems, filepaths[i]);
				if (status)
					printf(C_NO "Failure " NC "(%d)\n" NC,
							status);
				else
					printf(C_YES "Success\n" NC);
			}
			exit(status);
		}
		pids[i] = pid;
	}

	int success = 0;
	for (int i = 0; i<elems; i++){
		int status;
		waitpid(pids[i], &status, 0);
		if (status == 0)
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

#if defined(PEDANTIC) && !defined(NO_MEMORY_ALLOCATION)
	// free all the memory
	munmap(pids, elems*sizeof(pid_t));
	
	for (size_t i = 0; i<MAXFILES; i++){
		munmap(filepaths[i], PATH_MAX*sizeof(char));
	}
	munmap(filepaths, MAXFILES*sizeof(char*));

	munmap(filesmap, HASHMAP_SIZE*sizeof(void*));
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
