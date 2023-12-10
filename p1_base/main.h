#ifndef EMS_MAIN_H
#define EMS_MAIN_H

#include <stdio.h>
#include <pthread.h>
#include "constants.h"

typedef struct dirent file;


typedef struct {
  int *thread_active_array;
  int thread_index;
  int active_threads;
  int file_descriptor;
  unsigned int event_id;
  unsigned int delay;
  size_t num_rows;
  size_t num_columns;
  size_t num_coords;
  size_t xs[MAX_RESERVATION_SIZE];
  size_t ys[MAX_RESERVATION_SIZE];
  pthread_rwlock_t rwlock;
} ThreadParameters;

int iterateFiles(char* directoryPath);
char* pathingOut(const char *directoryPath, file *entry);
char *pathingJobs(char *directoryPath, file *entry);
int process_file(char* pathJobs, char* pathOut);
ThreadParameters *createThreadParameters(int fdWrite);
void destroyThreadParameters(ThreadParameters *params);

#endif