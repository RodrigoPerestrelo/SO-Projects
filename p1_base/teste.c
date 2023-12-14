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

// Função de comparação para qsort
int comparePairs(const void *a, const void *b) {
    // Comparação usando os valores de xs
    size_t xA = ((Pair*)a)->x;
    size_t xB = ((Pair*)b)->x;
    if (xA < xB) return -1;
    if (xA > xB) return 1;

    // Em caso de empate, desempatar usando os valores de ys
    size_t yA = ((Pair*)a)->y;
    size_t yB = ((Pair*)b)->y;
    return (yA < yB) ? -1 : (yA > yB);
}

// Função para ordenar os arrays xs e ys
void sortArrays(size_t *xs, size_t *ys, size_t num_seats) {
    // Criar um array de pares
    Pair *pairs = malloc(num_seats * sizeof(Pair));
    if (!pairs) {
        fprintf(stderr, "Erro na alocação de memória\n");
        exit(EXIT_FAILURE);
    }

    // Preencher o array de pares com os valores de xs e ys
    for (size_t i = 0; i < num_seats; ++i) {
        pairs[i].x = xs[i];
        pairs[i].y = ys[i];
    }

    // Usar qsort para ordenar o array de pares
    qsort(pairs, num_seats, sizeof(Pair), comparePairs);

    // Atualizar os arrays xs e ys com os valores ordenados
    for (size_t i = 0; i < num_seats; ++i) {
        xs[i] = pairs[i].x;
        ys[i] = pairs[i].y;
    }

    // Liberar a memória alocada para o array de pares
    free(pairs);
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