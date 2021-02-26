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
#endif

#include "util.h"
#include "color.h"
#ifdef __linux__
    #define C_NUMBERS YELLOW
    #define C_TESTNAM LCYAN
    #define C_SUCCESS LGREEN
    #define C_FAILURE LRED
    #define C_MESSAGE LPURPLE
#elif __APPLE__
    #define C_NUMBERS YELLOW
    #define C_TESTNAM CYAN
    #define C_SUCCESS GREEN
    #define C_FAILURE RED
    #define C_MESSAGE PURPLE
#endif

#define DEFAULT_DIR "tests"
#define DEFAULT_EXT ".test.out"

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
         if (cmp(argv[1], "-help") == 0) goto usage;
         else dir = argv[1];
      case 1:
         break;
      usage:
      printf("USAGE: %s [directory | options] [extension]\n"
             "\n"
             "SETTINGS:\n"
             "\tdirectory (UNIT_DIR) = %s\n"
             "\textension (UNIT_EXT) = %s\n"
             "\n"
             "OPTIONS:\n"
             "\t-help\tShow help message\n"
             "\n"
             ,
             argv[0], dir, ext);
      exit(0);
   }
   const unsigned dir_len = strlen(dir);

   int ext_count = 1;
   for (unsigned i = 0, ext_len=strlen(ext); i<ext_len; i++){
      if (ext[i] == ';') ext_count++;
   }
   
   char* exts[ext_count];
   int ext_sizes[ext_count];
   exts[0] = ext;
   for (unsigned i=0, ext_idx=1, ext_len=strlen(ext); i<ext_len; i++){
      if (ext[i] == ';'){
        ext[i] = '\0';
        exts[ext_idx++] = &ext[i]+1;
      }
   }
   for (int i = 0; i<ext_count; i++){
      ext_sizes[i] = strlen(exts[i]);
   }
   
   struct dirent* de;
   DIR* dr = opendir(dir);
   if (dr == NULL)
      errndie("DIR");
   int elems = 0;
   while ((de = readdir(dr)) != NULL){
      for (int i = 0; i<ext_count; i++){
         if (strncmp(de->d_name+strlen(de->d_name) - ext_sizes[i], exts[i], ext_sizes[i]) == 0){
            if (de->d_type != DT_DIR && de->d_name[0] != '.')
               elems++;
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
   char** program = calloc(elems, sizeof(char*));
   int* exit_code = mmap(NULL, elems*sizeof(int), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   pid_t* pids =  mmap(NULL, elems*sizeof(pid_t), PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

   dr = opendir(dir);
   if (dr == NULL)
      errndie("DIR");
   for (int i = 0; (de = readdir(dr));){
      for (int j = 0; j<ext_count; j++){
         if (strncmp(de->d_name+strlen(de->d_name) - ext_sizes[j], exts[j], ext_sizes[j]) == 0){
            if (de->d_type != DT_DIR && de->d_name[0] != '.'){
               program[i] = malloc(PATH_MAX*sizeof(char));
               snprintf(program[i], PATH_MAX, "%s/%s", dir, de->d_name);
               i++;
            }
         }
      }
   }
   closedir(dr);

   char logs_dir_path[PATH_MAX];
   {
      snprintf(logs_dir_path, PATH_MAX, "%s/%s/logs", getenv("PWD"), dir);
      struct stat st = {0};
      if (stat(logs_dir_path, &st) == -1){
         mkdir(logs_dir_path, S_IRWXU | S_IRWXG | S_IRWXO);
      }
   }
   printf("\nRunning " YELLOW "%d" NC " tests\n\n", elems);
   pid_t pid = 0;
   
   for (int i = 0; i<elems; i++){
      //printf(C_NUMBERS "%d/%d\t" C_TESTNAM "%s\t" C_MESSAGE "Started\n", i+1, elems, program[i]);
      spoon(){
         spoon(){
            char log[PATH_MAX*sizeof(char)];
            snprintf(log, PATH_MAX, "%s/%s.log", logs_dir_path, program[i]+dir_len+1);
            fflush(stdout);
            fflush(stderr);
            freopen(log, "w", stdout);
            freopen(log, "w", stderr);
            argv[0] = program[i];
            if (execvp(program[i], argv))
               errndie(program[i]);
         }
         int status;
         wait(&status);
         if (WIFEXITED(status)){
            exit_code[i] = WEXITSTATUS(status);
            printf(C_NUMBERS "%d/%d\t" C_TESTNAM "%s\t", i+1, elems, program[i]);
            if (exit_code[i])
                printf(C_FAILURE "Failure " NC "(%d)\n", exit_code[i]);
            else
                printf(C_SUCCESS "Success\n");
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
  
   if (success == elems)
      printf(C_SUCCESS "All tests passed!\n\n" NC);
   else
      printf(C_NUMBERS "%d " NC "out of " C_NUMBERS "%d " NC
            "succeeded\n\n", success, elems);

   munmap(pids, elems*sizeof(pid_t));
   munmap(exit_code, elems*sizeof(int));
   for (int i = 0; i<elems; i++){
      free(program[i]);
   }
   free(program);
   return 0;
}
