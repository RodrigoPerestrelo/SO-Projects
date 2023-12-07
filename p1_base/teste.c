#include "teste.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define INCREMENTO 128

int writeFile(int fd, char* buffer) {
    size_t len = strlen(buffer); // Correção: use size_t para o comprimento
    size_t done = 0;
    while (len > 0) {
        ssize_t bytes_written = write(fd, buffer + done, len); // Correção: use ssize_t para bytes_written

        if (bytes_written < 0) {
            fprintf(stderr, "write error: %s\n", strerror(errno));
            return -1;
        }

        /* might not have managed to write all, len becomes what remains */
        len -= (size_t)bytes_written; // Correção: converta bytes_written para size_t
        done += (size_t)bytes_written; // Correção: converta bytes_written para size_t
    }

    return 0;
}


