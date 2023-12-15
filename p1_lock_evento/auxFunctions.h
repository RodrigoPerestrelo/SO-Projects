#ifndef AUX_FUNCTIONS_H
#define AUX_FUNCTIONS_H

#include <string.h>
#include <dirent.h>

int writeFile(int fd, char* buffer);
char* pathingOut(const char *directoryPath, struct dirent *entry);
char *pathingJobs(char *directoryPath, struct dirent *entry);

#endif
