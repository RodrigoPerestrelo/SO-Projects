#include "api.h"
#include "common/rw_aux.h"
#include "common/constants.h"

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

  char *buffer = malloc(sizeof(char) * MAX_PIPE_NAME * 2);
  memset(buffer, '\0', MAX_PIPE_NAME * 2);
  strncpy(buffer, request_pipe, MAX_PIPE_NAME);
  strncpy(buffer + MAX_PIPE_NAME, response_pipe, MAX_PIPE_NAME);

  writeFile(tx, buffer, sizeof(char) * MAX_PIPE_NAME * 2);

  free(buffer);
  close(tx);

  return 0;
}

int ems_quit(void) { 

  int fdReq = open(request_pipe, O_WRONLY);

  char ch = '2';
  if (write(fdReq, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }
  //TODO: close pipes
  close(fdReq);

  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {

  
  int fdReq = open(request_pipe, O_WRONLY);

  char ch = '3';
  if (write(fdReq, &ch, 1) != 1) {
    perror("Error writing char.\n");
    exit(EXIT_FAILURE);
  }

  char *buffer = malloc(sizeof(unsigned int) + (sizeof(size_t) * 2));
  memcpy(buffer, &event_id, sizeof(unsigned int));
  memcpy(buffer + sizeof(unsigned int), &num_rows, sizeof(size_t));
  memcpy(buffer + sizeof(unsigned int) + sizeof(size_t), &num_cols, sizeof(size_t));

  writeFile(fdReq, buffer, sizeof(unsigned int) + (sizeof(size_t) * 2));

  free(buffer);
  close(fdReq);
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
