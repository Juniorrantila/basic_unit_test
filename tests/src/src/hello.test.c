#include <stdio.h>
#include <string.h>

int main(){
   int status = 1;
   char* str = "Hello, World!\n";
   if (printf("%s", str) == strlen(str))
      status = 0;
   return status;
}
