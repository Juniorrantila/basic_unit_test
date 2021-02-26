#pragma once

#define errndie(msg) do {perror(msg); exit(1);} while(0)

#define spoon() \
   pid = fork(); \
   if (pid < 0) errndie("Fork"); \
   else if (pid == 0)

#define cmp(a, b) strncmp(a, b, strlen(b))