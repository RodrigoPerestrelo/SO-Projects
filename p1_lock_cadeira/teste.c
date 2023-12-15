#include "teste.h"
#include "eventlist.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define INCREMENTO 128

int writeFile(int fd, char* buffer) {
    size_t len = strlen(buffer); // Correção: use size_t para o comprimento
    size_t done = 0;
    while (len > 0) {
        ssize_t bytes_written = write(fd, buffer + done, len); // Correção: use ssize_t para bytes_written

        if (bytes_written < 0) {
            fprintf(stderr, "write error: %s\n", strerror(errno));
            return -1;
        }

        /* might not have managed to write all, len becomes what remains */
        len -= (size_t)bytes_written; // Correção: converta bytes_written para size_t
        done += (size_t)bytes_written; // Correção: converta bytes_written para size_t
    }

    return 0;
}

void switch_seats(size_t* xs, size_t* ys, size_t i, size_t j) {
    size_t temp = xs[i];
    xs[i] = xs[j];
    xs[j] = temp;

    temp = ys[i];
    ys[i] = ys[j];
    ys[j] = temp;
}

// Ordena lugares utilizando um algoritmo bubblesort
int sort_seats(size_t num_seats, size_t* xs, size_t* ys) {
    // Algoritmo de ordenação
    for (size_t i = 0; i < num_seats - 1; i++) {
        for (size_t j = 0; j < num_seats - i - 1; j++) {
            // Ordena por linhas
            if (xs[j] > xs[j + 1]) {
                switch_seats(xs, ys, j, j + 1);
            }
            // Se as linhas forem iguais, ordena por colunas
            else if (xs[j] == xs[j + 1] && ys[j] == ys[j + 1]) {
              return 1;
            }
            else if (xs[j] == xs[j + 1] && ys[j] > ys[j + 1]) {
                switch_seats(xs, ys, j, j + 1);
            }
        }
    }
    return 0;
}


void freeMutexes(struct Event* event, size_t num_rows, size_t num_cols) {
    int num_seats = (int)(num_rows * num_cols);

    // Destruir os mutexes
    for (int i = 0; i < num_seats; i++) {
        if (pthread_mutex_destroy(&event->seatsLock[i]) != 0) {
            fprintf(stderr, "Erro na destruição do mutex %d\n", i);
            // Você pode decidir o que fazer em caso de erro na destruição
        }
    }

    // Liberar a memória alocada para os mutexes
    free(event->seatsLock);
}