#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#ifdef __linux__
   #include <linux/limits.h>
#elif __APPLE__
   #include <sys/syslimits.h>
#endif

#define NC        "\x1b[0;0m"
#define BLACK     "\x1b[0;30m"
#define DGRAY     "\x1b[1;30m"
#define RED       "\x1b[0;31m"
#define LRED      "\x1b[1;31m"
#define GREEN     "\x1b[0;32m"
#define LGREEN    "\x1b[1;32m"
#define BROWN     "\x1b[0;33m"
#define YELLOW    "\x1b[1;33m"
#define BLUE      "\x1b[0;34m"
#define LBLUE     "\x1b[1;34m"
#define PURPLE    "\x1b[0;35m"
#define LPURPLE   "\x1b[1;35m"
#define CYAN      "\x1b[0;36m"
#define LCYAN     "\x1b[1;36m"
#define LGRAY     "\x1b[0;37m"
#define WHITE     "\x1b[1;37m"

#define errndie(msg) do {perror(msg); exit(1);} while(0)

#define spoon() \
   pid = fork(); \
   if (pid < 0) errndie("Fork"); \
   else if (pid == 0)

#define cmp(a, b) strncmp(a, b, strlen(b))

int main(int argc, char *argv[]){

   char* dir = "./tests";
   char* ext = ".test.out";
   switch(argc){
      case 3:
         ext = argv[2];
      case 2:
         if (cmp(argv[1], "-help") == 0)
            goto usage;
         else if (cmp(argv[1], "--help") == 0)
            goto usage;
         else dir = argv[1];
      case 1:
         break;
      usage:
         printf("Usage: %s [test_dir] [extension]\n\n"
                "test_dir:\tdefault = %s\n"
                "extension:\tdefault = %s\n"
                "\n",
                argv[0], dir, ext);
         exit(0);
   }
   const int ext_size = strlen(ext);
   
   struct dirent *de;
   DIR* dr = opendir(dir);
   if (dr == NULL){
      perror("DIR");
      exit(1);
   }
   int elems = 0;
   while ((de = readdir(dr)) != NULL){
      if (strncmp(de->d_name+de->d_namlen - ext_size, ext, ext_size) == 0){
         elems++;
      }
   }
   closedir(dr);

   if (elems == 0){
      fprintf(stderr, "There are no %s files in %s.\n", ext, dir);
      exit(1);
   }
   char** program = calloc(elems, sizeof(char*));
   //int* exit_code = calloc(elems, sizeof(int));
   int* exit_code = mmap(NULL, elems*sizeof(int), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   pid_t* pids =  mmap(NULL, elems*sizeof(pid_t), PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

   dr = opendir(dir);
   if (dr == NULL){
      perror("DIR");
      exit(1);
   }
   for (int i = 0; (de = readdir(dr));){
      if (strncmp(de->d_name+de->d_namlen - ext_size, ext, ext_size) == 0){
         program[i] = malloc(PATH_MAX*sizeof(char));
         snprintf(program[i], PATH_MAX, "%s/%s", dir, de->d_name);
         i++;
      }
   }
   closedir(dr);

   printf("\nRunning " YELLOW "%d" NC " tests\n\n", elems);
   pid_t pid = 0;
   for (int i = 0; i<elems; i++){
      printf(YELLOW "%d/%d\t" CYAN "%s\t" PURPLE "Started\n", i+1, elems, program[i]);
      spoon(){
         spoon(){
            char* log = malloc(PATH_MAX*sizeof(char));
            snprintf(log, PATH_MAX, "%s.log", program[i]);
            if (log){
               fflush(stdout);
               freopen(log, "w", stdout);
            }
            free(log);
            argv[0] = program[i];
            if (execvp(program[i], argv))
               errndie(program[i]);
         }
         int status;
         wait(&status);
         if (WIFEXITED(status)){
            exit_code[i] = WEXITSTATUS(status);
         }
         exit(0);
      }
      pids[i] = pid;
   }
   
   int success = 0;
   for (int i = 0; i<elems; i++){ 
      waitpid(pids[i], NULL, 0);
      printf(YELLOW "%d/%d\t" CYAN "%s\t", i+1, elems, program[i]);
      if (exit_code[i])
         printf(RED "Failure " NC "(%d)\n", exit_code[i]);
      else {
         printf(GREEN "Success\n");
         success++;
      }
   }
   printf("\n");
  
   if (success == elems)
      printf(GREEN "All tests passed!\n\n" NC);
   else
      printf(YELLOW "%d " NC "out of " YELLOW "%d " NC
            "succeeded\n\n", success, elems);

   munmap(exit_code, elems*sizeof(int));
   for (int i = 0; i<elems; i++){
      free(program[i]);
   }
   free(program);
   return 0;
}
