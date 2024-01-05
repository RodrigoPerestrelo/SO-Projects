#include "api.h"

#include "api.h"
#include "common/constants.h"
#include "common/io.h"

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
      fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path, strerror(errno));
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

  printf("vou abrir pipes\n");

  close(fd_req_pipe);
  close(fd_resp_pipe);
  printf("a");
  fd_req_pipe = open(req_pipe_path, O_WRONLY);
  if (fd_req_pipe == -1) {
      fprintf(stderr, "[ERR]: Failed to open request pipe: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  fd_resp_pipe = open(resp_pipe_path, O_RDONLY);
  if (fd_resp_pipe == -1) {
      fprintf(stderr, "[ERR]: Failed to open response pipe: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  printf("pipes abertos: %d (request) %d (response)\n", fd_req_pipe, fd_resp_pipe);
  return 1;
}


int ems_quit(void) { 
  //TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
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
