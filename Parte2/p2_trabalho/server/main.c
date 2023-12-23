#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "common/rw_aux.h"
#include "common/queue.h"

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFERSIZE 1024

Queue *queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* execute_client() {

  printf("Entrei dentro do lock do mutex\n");
  pthread_cond_wait(&cond, &mutex);
  printf("Recebi o signal\n");

  char *info = getHeadQueue(queue);
  printf("%s\n", info);
  removeHeadQueue(queue);

  if (pthread_mutex_unlock(&mutex) != 0) {
    exit(EXIT_FAILURE);
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char *SERVER_FIFO = argv[1];

  // remove pipe if it does exist
  if (unlink(SERVER_FIFO) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", SERVER_FIFO, strerror(errno));
    exit(EXIT_FAILURE);
  }
  // create pipe
  if (mkfifo(SERVER_FIFO, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }


  queue = initializeQueue();
  pthread_t thread_id;


  while (1) {

    if (pthread_mutex_lock(&mutex) != 0) {
      exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread_id, NULL, execute_client, NULL) != 0) {
      perror("Error creating thread");
      return 1;
    }

    if (pthread_mutex_lock(&mutex) != 0) {
      exit(EXIT_FAILURE);
    }

    char *buffer = malloc(sizeof(char) * BUFFERSIZE);
    int tx = open(SERVER_FIFO, O_RDONLY);
    readBuffer(tx, buffer, BUFFERSIZE);
    addToQueue(queue, buffer);
    printf("Vou dar o signal\n");
    pthread_cond_signal(&cond);
    printf("Dei o signal\n");

    free(buffer);
    if (pthread_mutex_unlock(&mutex) != 0) {
      exit(EXIT_FAILURE);
    }

    if (pthread_join(thread_id, NULL) != 0) {
      perror("Erro ao esperar pela thread");
      exit(EXIT_FAILURE);
    }
    
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
}