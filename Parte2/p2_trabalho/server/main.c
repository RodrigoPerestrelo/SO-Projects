#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


#include "common/io.h"
#include "operations.h"
#include "common/rw_aux.h"
#include "common/queue.h"
#include "common/constants.h"

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

pthread_t threads[MAX_SESSION_COUNT];
int activeClients = 0;

Queue *globalQueue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* execute_client(void* args) {

  int *thread_id = (int*)args;

  while (1) {
    
    if (activeClients < MAX_SESSION_COUNT) {
      pthread_cond_wait(&cond, &mutex);
    }
    printf("Cliente ligou-se à thread %d\n", *thread_id);

    int flag = 1;

    Node *head = getHeadQueue(globalQueue);
    char requestPipe[MAX_PIPE_NAME];
    strcpy(requestPipe, head->requestPipe);
    char responsePipe[MAX_PIPE_NAME];
    strcpy(responsePipe, head->responsePipe);

    removeHeadQueue(globalQueue);

    if (pthread_mutex_unlock(&mutex) != 0) {
      exit(EXIT_FAILURE);
    }
    unsigned int event_id;
    size_t num_rows, num_cols, num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
    char ch, *buffer, *ptr;
    int status;
    int fdReq = open(requestPipe, O_RDONLY);
    int fdResp = open(responsePipe, O_WRONLY);

    while (flag) {

      if (read(fdReq, &ch, 1) != 1) {
        perror("Error reading char.\n");
        exit(EXIT_FAILURE);
      }
      switch (ch) {
        case '2':

          printf("Ficheiro Acabou\n");
          close(fdReq);
          close(fdResp);
          flag = 0;
          activeClients--;
          break;
        case '3':

          buffer = malloc(sizeof(unsigned int) + (sizeof(size_t) * 2));
          if (buffer == NULL) {
            perror("Error allocating memory.\n");
            exit(EXIT_FAILURE);
          }
          readBuffer(fdReq, buffer, sizeof(unsigned int) + (sizeof(size_t) * 2));

          ptr = buffer;
          memcpy(&event_id, ptr, sizeof(unsigned int));
          ptr += sizeof(unsigned int);
          memcpy(&num_rows, ptr, sizeof(size_t));
          ptr += sizeof(size_t);
          memcpy(&num_cols, ptr, sizeof(size_t));
          free(buffer);

          //printf("CREATE %u %ld %ld\n", event_id, num_rows, num_cols);
          status = ems_create(event_id, num_rows, num_cols);

          buffer = malloc(sizeof(int));
          if (buffer == NULL) {
            perror("Error allocating memory.\n");
            exit(EXIT_FAILURE);
          }
          memcpy(buffer, &status, sizeof(int));
          writeFile(fdResp, buffer, sizeof(int));
          free(buffer);

          break;
        case '4':

          size_t size_event_id = sizeof(unsigned int);
          size_t size_num_seats = sizeof(size_t);
          size_t size_reservation_seat = sizeof(size_t);
          size_t size_xs = sizeof(size_t) * MAX_RESERVATION_SIZE;
          size_t size_ys = sizeof(size_t) * MAX_RESERVATION_SIZE;

          size_t total_size = size_event_id + size_num_seats + size_xs + size_ys;

          buffer = malloc(total_size);
          if (buffer == NULL) {
            perror("Error allocating memory.\n");
            exit(EXIT_FAILURE);
          }
          readBuffer(fdReq, buffer, total_size);

          ptr = buffer;
          memcpy(&event_id, ptr, size_event_id);
          ptr += size_event_id;
          memcpy(&num_seats, ptr, size_num_seats);
          ptr += size_num_seats;

          for (int i = 0; i < (int)num_seats; i++) {
            memcpy(&xs[i], ptr, size_reservation_seat);
            ptr += size_reservation_seat;
            memcpy(&ys[i], ptr, size_reservation_seat);
            ptr += size_reservation_seat;
          }
          free(buffer);

          /*
          printf("Reservas do evento: %u\n", event_id);
          for (int i = 0; i < (int)num_seats; i++) {
            printf("Lugar:(%ld %ld)\n", xs[i], ys[i]);
          }
          */
          status = ems_reserve(event_id, num_seats, xs, ys);
          buffer = malloc(sizeof(int));
          if (buffer == NULL) {
            perror("Error allocating memory.\n");
            exit(EXIT_FAILURE);
          }
          memcpy(buffer, &status, sizeof(int));
          writeFile(fdResp, buffer, sizeof(int));
          free(buffer);

          break;
        case '5':

          buffer = malloc(sizeof(unsigned int) + sizeof(int));
          if (buffer == NULL) {
            perror("Error allocating memory.\n");
            exit(EXIT_FAILURE);
          }
          readBuffer(fdReq, buffer, sizeof(unsigned int) + sizeof(int));

          memcpy(&event_id, buffer, sizeof(unsigned int));
          free(buffer);

          ems_show(fdResp, event_id);

          break;
        case '6':
          ems_list_events(fdResp);
        default:
          break;
      }
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


  globalQueue = initializeQueue();
  
  if (pthread_mutex_lock(&mutex) != 0) {
    exit(EXIT_FAILURE);
  }

  int thread_ids[MAX_SESSION_COUNT];

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    thread_ids[i] = i;
    pthread_create(&threads[i], NULL, execute_client, &thread_ids[i]);
    if (pthread_mutex_lock(&mutex) != 0) {
      exit(EXIT_FAILURE);
    }
  }

  if (pthread_mutex_unlock(&mutex) != 0) {
    exit(EXIT_FAILURE);
  }

  while (1) {

    char *buffer = malloc(sizeof(char) * (MAX_PIPE_NAME * 2));
    if (buffer == NULL) {
      perror("Error allocating memory.\n");
      exit(EXIT_FAILURE);
    }
    char *bufferRequest = malloc(sizeof(char) * MAX_PIPE_NAME);
    if (bufferRequest == NULL) {
      perror("Error allocating memory.\n");
      exit(EXIT_FAILURE);
    }
    char *bufferResponse = malloc(sizeof(char) * MAX_PIPE_NAME);
    if (bufferResponse == NULL) {
      perror("Error allocating memory.\n");
      exit(EXIT_FAILURE);
    }

    char character;
    int tx = open(SERVER_FIFO, O_RDONLY);
    printf("entrou cliente\n");
    read(tx, &character, sizeof(char));

    //VERIFICAÇÕES
    //PARA VER SE PODEMOS INICIALIZAR SESSÃO
    //EM CASO AFIRMATIVO INICILIZAR O SERVER, EM CASO NEGATIVO FAZER A LEITURA MAS

    readBuffer(tx, buffer, MAX_PIPE_NAME * 2);
    strncpy(bufferRequest, buffer, MAX_PIPE_NAME);
    strncpy(bufferResponse, buffer + MAX_PIPE_NAME, MAX_PIPE_NAME);

    addToQueue(globalQueue, bufferRequest, bufferResponse);
    activeClients++;
    pthread_cond_signal(&cond);

    free(bufferRequest);
    free(bufferResponse);
    free(buffer);
    
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
}