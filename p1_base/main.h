#ifndef EMS_MAIN_H
#define EMS_MAIN_H

#include <stdio.h>
#include <pthread.h>
#include "constants.h"

typedef struct dirent file;
#define BARRIER 1

typedef struct {
  int fdRead;
  int fdWrite;
  int barrierFlag;
  pthread_mutex_t mutex;
  size_t *xs; 
  size_t *ys;
} ThreadParameters;

int iterateFiles(char* directoryPath);
char* pathingOut(const char *directoryPath, file *entry);
char *pathingJobs(char *directoryPath, file *entry);
int process_file(char* pathJobs, char* pathOut);
void* thread_execute(void* args);
ThreadParameters *createThreadParameters(int fdWrite, int fdRead, size_t xs[MAX_RESERVATION_SIZE], size_t ys[MAX_RESERVATION_SIZE]);
void destroyThreadParameters(ThreadParameters *params);

#endif