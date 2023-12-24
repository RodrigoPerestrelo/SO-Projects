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

Queue *queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* execute_client() {

  pthread_cond_wait(&cond, &mutex);

  Node *head = getHeadQueue(queue);
  char requestPipe[100];
  strcpy(requestPipe, head->requestPipe);
  char responsePipe[100];
  strcpy(responsePipe, head->responsePipe);

  removeHeadQueue(queue);


  if (pthread_mutex_unlock(&mutex) != 0) {
    exit(EXIT_FAILURE);
  }
  unsigned int event_id;
  size_t num_rows, num_cols, num_seats;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  char *buffer;
  int fdReq = open(requestPipe, O_RDONLY);

  while (1) {
    char ch;
    if (read(fdReq, &ch, 1) != 1) {
      perror("Error reading char.\n");
      exit(EXIT_FAILURE);
    }
    switch (ch) {
      case '2':

        close(fdReq);
        return NULL;
        break;
      case '3':

        buffer = malloc(sizeof(unsigned int) + (sizeof(size_t) * 2));
        readBuffer(fdReq, buffer, sizeof(unsigned int) + (sizeof(size_t) * 2));

        memcpy(&event_id, buffer, sizeof(unsigned int));
        memcpy(&num_rows, buffer + sizeof(unsigned int), sizeof(size_t));
        memcpy(&num_cols, buffer + sizeof(unsigned int) + sizeof(size_t), sizeof(size_t));
        free(buffer);

        printf("CREATE %u %ld %ld\n", event_id, num_rows, num_cols);
        ems_create(event_id, num_rows, num_cols);
        break;
      case '4':

        size_t size_event_id = sizeof(unsigned int);
        size_t size_num_seats = sizeof(size_t);
        size_t size_reservation_seat = sizeof(size_t);
        size_t size_xs = sizeof(size_t) * MAX_RESERVATION_SIZE;
        size_t size_ys = sizeof(size_t) * MAX_RESERVATION_SIZE;

        size_t total_size = size_event_id + size_num_seats + size_xs + size_ys;

        buffer = malloc(total_size);
        char *ptr = buffer;

        readBuffer(fdReq, buffer, total_size);

        memcpy(&event_id, ptr, size_event_id);
        ptr += size_event_id;
        memcpy(&num_seats, ptr, size_num_seats);
        for (int i = 0; i < (int)num_seats; i++) {
          ptr += size_reservation_seat;
          memcpy(&xs[i], ptr, size_reservation_seat);
          ptr += size_reservation_seat;
          memcpy(&ys[i], ptr, size_reservation_seat);
        }
        free(buffer);
        
        printf("Reservas do evento: %u\n", event_id);
        for (int i = 0; i < (int)num_seats; i++) {
          printf("Lugar:(%ld %ld)\n", xs[i], ys[i]);
        }

        ems_reserve(event_id, num_seats, xs, ys);
        break;
      
      default:
        break;
    }
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

    char *buffer = malloc(sizeof(char) * (MAX_PIPE_NAME * 2));
    char *bufferRequest = malloc(sizeof(char) * MAX_PIPE_NAME);
    char *bufferResponse = malloc(sizeof(char) * MAX_PIPE_NAME);
    
    int tx = open(SERVER_FIFO, O_RDONLY);

    readBuffer(tx, buffer, MAX_PIPE_NAME * 2);
    strncpy(bufferRequest, buffer, MAX_PIPE_NAME);
    strncpy(bufferResponse, buffer + MAX_PIPE_NAME, MAX_PIPE_NAME);

    addToQueue(queue, bufferRequest, bufferResponse);
    pthread_cond_signal(&cond);

    free(bufferRequest);
    free(bufferResponse);
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