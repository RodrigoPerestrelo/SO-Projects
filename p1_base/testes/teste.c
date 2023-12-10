#include <stdio.h>
#include <pthread.h>

#define MAX_THREADS 5

pthread_t threads[MAX_THREADS]; // Array para armazenar as threads
int thread_active[MAX_THREADS] = {0}; // Array de flags para indicar se a thread está ativa
int thread_count = 0; // Contador de threads em execução

void *thread_function(void *arg) {
    int thread_index = *((int *)arg);
    // Processamento do comando aqui usando thread_index

    // Sinaliza que a thread terminou
    thread_active[thread_index] = 0;

    pthread_exit(NULL);
}

int main() {
    while (1) {
        // Verifica se há uma vaga para uma nova thread
        if (thread_count < MAX_THREADS) {
            // Encontra um índice de thread disponível
            int thread_index = -1;
            for (int i = 0; i < MAX_THREADS; ++i) {
                if (!thread_active[i]) {
                    thread_index = i;
                    break;
                }
            }

            // Cria uma nova thread
            if (pthread_create(&threads[thread_index], NULL, thread_function, (void *)&thread_index) != 0) {
                fprintf(stderr, "Erro ao criar a thread.\n");
                return 1;
            }

            // Incrementa o contador de threads
            thread_count++;
            // Sinaliza que a thread está ativa
            thread_active[thread_index] = 1;
        } else {
            // Aguarda um curto período de tempo (método de polling)
            usleep(1000); // Aguarda 1 milissegundo (exemplo, ajuste conforme necessário)
        }
    }

    return 0;
}
