#include "api.h"
#include "common/rw_aux.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUFFERSIZE 1024

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  
  char *info = malloc(BUFFERSIZE * sizeof(char));
  if (info == NULL) {
    fprintf(stderr, "Error allocating memory.\n");
    return 1;
  }

  strcpy(info, "1|");
  strcat(info, req_pipe_path);
  strcat(info, "|");
  strcat(info, resp_pipe_path);

  int tx = open(server_pipe_path, O_WRONLY);
  if (tx == -1) {
    fprintf(stderr, "Error opening the server pipe.\n");
    free(info);
    return 1;
  }

  writeFile(tx, info);
  free(info);
  
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
