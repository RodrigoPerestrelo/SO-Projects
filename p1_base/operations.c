#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "eventlist.h"
#include "teste.h"
#include "main.h"



#define BUFFERSIZE 1024
#define IDMAX 128

static struct EventList* event_list = NULL;
static unsigned int state_access_delay_ms = 0;
pthread_rwlock_t rwlock;
//pthread_mutex_t mutex;

/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed

  return get_event(event_list, event_id);
}

/// Gets the seat with the given index from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event Event to get the seat from.
/// @param index Index of the seat to get.
/// @return Pointer to the seat.
static unsigned int* get_seat_with_delay(struct Event* event, size_t index) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed

  return &event->data[index];
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

int ems_init(unsigned int delay_ms) {
  if (event_list != NULL) {
    fprintf(stderr, "EMS state has already been initialized\n");
    return 1;
  }

  event_list = create_list();
  state_access_delay_ms = delay_ms;

  return event_list == NULL;
}

int ems_terminate() {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  free_list(event_list);
  return 0;
}

void ems_create(void *args) {

  ThreadParameters **parameters = (ThreadParameters**)args;
  unsigned int event_id = (*parameters)->event_id;
  size_t num_rows = (*parameters)->num_rows;
  size_t num_cols = (*parameters)->num_columns;
  rwlock = (*parameters)->rwlock;

  pthread_rwlock_wrlock(&rwlock);

  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    fprintf(stderr, "Failed to create event\n");
    return;
  }

  if (get_event_with_delay(event_id) != NULL) {
    fprintf(stderr, "Event already exists\n");
    return;
  }

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    fprintf(stderr, "Error allocating memory for event\n");
    fprintf(stderr, "Failed to create event\n");
    return;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  event->data = malloc(num_rows * num_cols * sizeof(unsigned int));

  if (event->data == NULL) {
    fprintf(stderr, "Error allocating memory for event data\n");
    fprintf(stderr, "Failed to create event\n");
    free(event);
    return;
  }

  for (size_t i = 0; i < num_rows * num_cols; i++) {
    event->data[i] = 0;
  }

  if (append_to_list(event_list, event) != 0) {
    fprintf(stderr, "Error appending event to list\n");
    fprintf(stderr, "Failed to create event\n");
    free(event->data);
    free(event);
    return;
  }

  (*parameters)->thread_active_array[(*parameters)->thread_index] = 0;
  (*parameters)->active_threads--;

  pthread_rwlock_unlock(&rwlock);
  pthread_exit(NULL);

  return;
}

void ems_reserve(void *args) {

  ThreadParameters **parameters = (ThreadParameters**)args;
  unsigned int event_id = (*parameters)->event_id;
  size_t num_seats = (*parameters)->num_coords;
  size_t *xs = (*parameters)->xs;
  size_t *ys = (*parameters)->ys;
  rwlock = (*parameters)->rwlock;

  pthread_rwlock_wrlock(&rwlock);

  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    fprintf(stderr, "Failed to reserve seats\n");
    return;
  }

  struct Event* event = get_event_with_delay(event_id);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    fprintf(stderr, "Failed to reserve seats\n");
    return;
  }

  unsigned int reservation_id = ++event->reservations;

  size_t i = 0;
  for (; i < num_seats; i++) {
    size_t row = xs[i];
    size_t col = ys[i];

    if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
      fprintf(stderr, "Invalid seat\n");
      break;
    }

    if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0) {
      fprintf(stderr, "Seat already reserved\n");
      break;
    }

    *get_seat_with_delay(event, seat_index(event, row, col)) = reservation_id;
  }

  // If the reservation was not successful, free the seats that were reserved.
  if (i < num_seats) {
    event->reservations--;
    for (size_t j = 0; j < i; j++) {
      *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
    }
    fprintf(stderr, "Failed to reserve seats\n");
    return;
  }

  (*parameters)->thread_active_array[(*parameters)->thread_index] = 0;
  (*parameters)->active_threads--;

  pthread_rwlock_unlock(&rwlock);
  pthread_exit(NULL);

  return;
}

void ems_show(void* args) { // adicionar um fd

  ThreadParameters **parameters = (ThreadParameters**)args;
  unsigned int event_id = (*parameters)->event_id;
  int fdWrite = (*parameters)->file_descriptor;
  rwlock = (*parameters)->rwlock;

  pthread_rwlock_rdlock(&rwlock);
  

  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    fprintf(stderr, "Failed to show event\n");
    (*parameters)->thread_active_array[(*parameters)->thread_index] = 0;
    (*parameters)->active_threads--;
    return;
  }

  struct Event *event = get_event_with_delay(event_id);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    fprintf(stderr, "Failed to show event\n");
    (*parameters)->thread_active_array[(*parameters)->thread_index] = 0;
    (*parameters)->active_threads--;
    return;
  }

  char *buffer = malloc(((event->rows * event->cols) * sizeof(char)) * 2 + 2);
  char *current = buffer;  // Ponteiro auxiliar para rastrear a posição atual

  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1; j <= event->cols; j++) {
      unsigned int* seat = get_seat_with_delay(event, seat_index(event, i, j));
      int written = snprintf(current, 2, "%u", *seat);  // Use snprintf para evitar estouro de buffer
      current += written;  // Atualizar o ponteiro auxiliar

      if (j < event->cols) {
        int space_written = snprintf(current, 2, " ");
        current += space_written;
      }
    }
    int newline_written = snprintf(current, 2, "\n");
    current += newline_written;
  }

  *current = '\n';  // Adicionar terminador nulo no final do buffer
  current++;
  *current = '\0';
  writeFile(fdWrite, buffer);
  free(buffer);

  (*parameters)->thread_active_array[(*parameters)->thread_index] = 0;
  (*parameters)->active_threads--;

  pthread_rwlock_unlock(&rwlock);
  pthread_exit(NULL);

  return;
}


int ems_list_events(int fdWrite) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (event_list->head == NULL) {
    writeFile(fdWrite, "No events\n");
    return 0;
  }

  char buffer[BUFFERSIZE];

  struct ListNode* current = event_list->head;
  while (current != NULL) {

    char *id = malloc(IDMAX*sizeof(char));
    sprintf(id, "%u\n", (current->event)->id);
    strcat(buffer, "Event: ");
    strcat(buffer, id);
    free(id);
    current = current->next;
  }
  writeFile(fdWrite, buffer);

  return 0;
}

void ems_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}
