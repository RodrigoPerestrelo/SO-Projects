#ifndef RW_AUX_H
#define RW_AUX_H

#include <stddef.h>

int readBuffer(int fd, char *buffer, size_t bufferSize);
int writeFile(int fd, char* buffer);

#endif