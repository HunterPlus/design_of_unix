#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void prompt() { printf("db > "); }

int main(int argc, char *argv[])
{
        char    buf[BUFSIZ];
        while (1) {
                prompt();

                if (fgets(buf, sizeof buf, stdin) == NULL) {
                        printf("\n");
                        continue;
                }
                buf[strlen(buf) - 1] = '\0';            /* delete newline */               
                if (strcmp(buf, ".exit") == 0)                         
                        exit(0);
                else  
                        printf("Unrecognized command '%s'.\n", buf);
        }
}

