#ifndef TESTE_H
#define TESTE_H
#include <string.h>
#include "eventlist.h"

typedef struct {
    size_t x;
    size_t y;
} Pair;

int writeFile(int fd, char* buffer);
void switch_seats(size_t* xs, size_t* ys, size_t i, size_t j);
int sort_seats(size_t num_seats, size_t* xs, size_t* ys);
void freeMutexes(struct Event*event, size_t num_rows, size_t num_cols);

#endif

