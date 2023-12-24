#include "rw_aux.h"

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <string.h> // for memset
#include <unistd.h> // for read
#include <stdio.h>  // for printf and other IO functions
#include <errno.h>  // for errno and strerror

/* Functtion that writes to the output file */
int writeFile(int fd, const char* buffer, size_t bufferSize) {
    size_t len = bufferSize;  // Usando o bufferSize fornecido.
    size_t done = 0;

    while (len > 0) {
        ssize_t bytes_written = write(fd, buffer + done, len);

        if (bytes_written < 0) {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
        }

        len -= (size_t)bytes_written;
        done += (size_t)bytes_written;
    }

    return 0;
}

int readBuffer(int fd, char *buffer, size_t bufferSize) {
   // Clear the buffer
   memset(buffer, 0, bufferSize);

   // Read data into the buffer
   ssize_t bytes_read = read(fd, buffer, bufferSize);
   if (bytes_read < 0) {
      printf("read error: %s\n", strerror(errno));
      return -1;
   }

   return 0;
}

void splitString(char *original, char *buffer1, char *buffer2) {
    char *token;

    char workingString[strlen(original) + 1];
    strcpy(workingString, original);

    token = strtok(workingString, "|");

    if (token != NULL) {
        strcpy(buffer1, token);
        token = strtok(NULL, "|");
        if (token != NULL) {
            strcpy(buffer2, token);
        } else {
            // Em caso de formato inválido, deixe o buffer2 vazio
            buffer2[0] = '\0';
        }
    } else {
        // Em caso de formato inválido, deixe ambos os buffers vazios
        buffer1[0] = '\0';
        buffer2[0] = '\0';
    }
}
