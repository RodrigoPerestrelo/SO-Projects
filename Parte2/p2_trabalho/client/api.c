#include "api.h"
#include "common/rw_aux.h"
#include "common/constants.h"
#include "common/io.h"
#include "server/operations.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFERSIZE 1024

const char* request_pipe;
const char* response_pipe;

int fd_req_pipe = -1;
int fd_resp_pipe = -1;


int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  
  // remove pipe if it does exist
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // remove pipe if it does exist
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", response_pipe, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // create pipe
  if (mkfifo(req_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // create pipe
  if (mkfifo(resp_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  request_pipe = req_pipe_path;
  response_pipe = resp_pipe_path;

  int tx = open(server_pipe_path, O_WRONLY);

  if (tx == -1) {
    fprintf(stderr, "Error opening the server pipe.\n");
    return 1;
  }


  char ch = '1';
  if (write(tx, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  //AGUARDAR RESPOSTA
  char *buffer = malloc(sizeof(char) * MAX_PIPE_NAME * 2);
  memset(buffer, '\0', MAX_PIPE_NAME * 2);
  strncpy(buffer, request_pipe, MAX_PIPE_NAME);
  strncpy(buffer + MAX_PIPE_NAME, response_pipe, MAX_PIPE_NAME);

  writeFile(tx, buffer, sizeof(char) * MAX_PIPE_NAME * 2);

  free(buffer);
  close(tx);
  
  fd_req_pipe = open(req_pipe_path, O_WRONLY);
  fd_resp_pipe = open(resp_pipe_path, O_RDONLY);

  return 0;
}

int ems_quit(void) { 

  char ch = '2';
  if (write(fd_req_pipe, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }
  //TODO: close pipes
  close(fd_req_pipe);
  close(fd_resp_pipe);

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {

  char ch = '3';
  if (write(fd_req_pipe, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  char *buffer = malloc(sizeof(unsigned int) + (sizeof(size_t) * 2));
  if (buffer == NULL) {
    perror("Erro ao alocar memória para o buffer");
    exit(EXIT_FAILURE);
  }
  memcpy(buffer, &event_id, sizeof(unsigned int));
  memcpy(buffer + sizeof(unsigned int), &num_rows, sizeof(size_t));
  memcpy(buffer + sizeof(unsigned int) + sizeof(size_t), &num_cols, sizeof(size_t));

  writeFile(fd_req_pipe, buffer, sizeof(unsigned int) + (sizeof(size_t) * 2));
  free(buffer);


  int response;
  buffer = malloc(sizeof(int));
  readBuffer(fd_resp_pipe, buffer, sizeof(int));
  memcpy(&response, buffer, sizeof(int));
  free(buffer);

  return response;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {

  char ch = '4';

  if (write(fd_req_pipe, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  size_t size_event_id = sizeof(unsigned int);
  size_t size_num_seats = sizeof(size_t);
  size_t size_reservation_seat = sizeof(size_t);
  size_t size_xs = sizeof(size_t) * MAX_RESERVATION_SIZE;
  size_t size_ys = sizeof(size_t) * MAX_RESERVATION_SIZE;

  size_t total_size = size_event_id + size_num_seats + size_xs + size_ys;

  // Alocação de memória para o buffer
  char *buffer = malloc(total_size);
  if (buffer == NULL) {
    perror("Erro ao alocar memória para o buffer");
    exit(EXIT_FAILURE);
  }
  char *ptr = buffer;
  memcpy(ptr, &event_id, size_event_id);
  ptr += size_event_id;
  memcpy(ptr, &num_seats, size_num_seats);

  for (int i = 0; i < (int)num_seats; i++) {
    ptr += size_reservation_seat;
    memcpy(ptr, &xs[i], size_reservation_seat);
    ptr += size_reservation_seat;
    memcpy(ptr, &ys[i], size_reservation_seat);
  }

  writeFile(fd_req_pipe, buffer, total_size);
  free(buffer);

  int response;
  buffer = malloc(sizeof(int));
  readBuffer(fd_resp_pipe, buffer, sizeof(int));
  memcpy(&response, buffer, sizeof(int));
  free(buffer);

  return response;
}

int ems_show(int out_fd, unsigned int event_id) {

  char *ptr;
  char ch = '5';

  if (write(fd_req_pipe, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  char *buffer = malloc(sizeof(unsigned int) + sizeof(int));
  if (buffer == NULL) {
    perror("Erro ao alocar memória para o buffer");
    exit(EXIT_FAILURE);
  }
  memcpy(buffer, &event_id, sizeof(unsigned int));

  writeFile(fd_req_pipe, buffer, sizeof(unsigned int));
  free(buffer);

  // LER O RETORNO DO SHOW, SE FUNCIONOU OU NÃO
  int ret_value;

  buffer = malloc(sizeof(int));
  readBuffer(fd_resp_pipe, buffer, sizeof(int));
  memcpy(&ret_value, buffer, sizeof(int));
  free(buffer);

  if (ret_value) return 1;
  

  size_t num_rows, num_cols;

  buffer = malloc(sizeof(size_t) * 2);
  readBuffer(fd_resp_pipe, buffer, sizeof(size_t) * 2);
  ptr = buffer;
  memcpy(&num_rows, ptr, sizeof(size_t));
  ptr += sizeof(size_t);
  memcpy(&num_cols, ptr, sizeof(size_t));
  ptr += sizeof(size_t);
  free(buffer);

  size_t num_seats = num_rows* num_cols;
  unsigned int show_seats[num_rows * num_cols];
  memset(show_seats, 0, num_rows * num_cols * sizeof(unsigned int));

  buffer = malloc(sizeof(unsigned int) * num_cols * num_rows);
  readBuffer(fd_resp_pipe, buffer, sizeof(unsigned int) * num_seats);
  ptr = buffer;

  for (size_t i = 0; i < num_seats; i++) {
    memcpy(&show_seats[i], ptr, sizeof(unsigned int));
    ptr += sizeof(unsigned int);
  }
  
  printf("\n");
  for (size_t i = 0; i < num_cols* num_rows; i++) {
    if (i%num_cols == 0 && i != 0) {
      printf("\n");
    }
    
    printf("%u ", show_seats[i]);
  }
  printf("\n");

  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 0;
}

int ems_list_events(int out_fd) {

  char *buffer;
  char *ptr;

  char ch = '6';
  if (write(fd_req_pipe, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  int ret_value;

  buffer = malloc(sizeof(int));
  readBuffer(fd_resp_pipe, buffer, sizeof(int));
  memcpy(&ret_value, buffer, sizeof(int));
  free(buffer);

  if (ret_value) return 1;

  size_t num_events;
  buffer = malloc(sizeof(size_t));
  readBuffer(fd_resp_pipe, buffer, sizeof(size_t));
  memcpy(&num_events, buffer, sizeof(size_t));
  free(buffer);

  if (num_events == 0) {
    printf("No events\n");
  } else {
    buffer = malloc(sizeof(unsigned int) * num_events);
    readBuffer(fd_resp_pipe, buffer, sizeof(unsigned int) * num_events);
    ptr = buffer;

    unsigned int list_ids[num_events];
    memset(list_ids, 0, num_events * sizeof(unsigned int));

    for (size_t i = 0; i < num_events; i++) {
      memcpy(&list_ids[i], ptr, sizeof(unsigned int));
      printf("Event: %u\n", list_ids[i]);
      ptr += sizeof(unsigned int);
    }
    free(buffer);
  }


  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 0;
}
