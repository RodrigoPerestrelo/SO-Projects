#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#include "operations.h"
#include "parser.h"
#include "main.h"

int global_num_proc = 0;
int global_num_threads = 0;

/* Função main que faz o processamento dos argumentos e chama as funções que fazem o processamento dos ficheiros */
int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  //Se o número de argumentos for diferente de 4 ou 5, o input é inválido
  if (argc != 4 && argc != 5) {
    fprintf(stderr, "Invalid arguments. See HELP for usage\n");
    return 1;
  }

  // Se o número de processos for inválido, o input é inválido
  char *endptr_proc;
  long int num_proc = strtol(argv[2], &endptr_proc, 10);
  if (*endptr_proc != '\0' || num_proc > INT_MAX || num_proc < 1) {
    fprintf(stderr, "Invalid proc value or value too large\n");
    return 1;
  }
  global_num_proc = (int)num_proc;

  // Se o número de threads for inválido, o input é inválido
  char *endptr_threads;
  long int num_threads = strtol(argv[3], &endptr_threads, 10);
  if (*endptr_threads != '\0' || num_threads > INT_MAX || num_threads < 1) {
    fprintf(stderr, "Invalid threads value or value too large\n");
    return 1;
  }
  global_num_threads = (int)num_threads;

  // Se houver quinto argumento, atribui-se o valor do delay
  if (argc == 5) {
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  iterateFiles(argv[1]);

  return 0;
}


/* Função que itera sobre os ficheiros de uma diretoria */
int iterateFiles(char* directoryPath) {
  DIR *dir;
  file *entry;

  // Abrir diretoria
  if ((dir = opendir(directoryPath)) == NULL) {
      perror("Error opening directory");
      return -1;
  }

  int activeProcesses = 0;
  int status;

  // Iterar sobre os ficheiros da diretoria
  while ((entry = readdir(dir)) != NULL) {
    if (strstr(entry->d_name, ".job") != NULL) {
      if (activeProcesses == global_num_proc) {
        wait(&status);
        //printf("%d \n",status);
        printf("O processo %d terminou devido ao wait.\n", status);
        activeProcesses--;
      }
      pid_t pid = fork();
      if (pid < 0) {
        perror("Error forking process");
        closedir(dir);
        return -1;

      } else if (pid == 0) { // Processo filho
        char *pathJobs = pathingJobs(directoryPath, entry);
        char *pathOut = pathingOut(directoryPath, entry);
        if (process_file(pathJobs, pathOut) != 0) {
          fprintf(stderr, "Error processing file: %s\n", pathJobs);
          closedir(dir);
          return -1;
        }
        free(pathJobs);
        free(pathOut);
        exit(0);

      } else { // Processo pai
        ++activeProcesses;
        printf("Numero de processos ativos:%d \n", activeProcesses);
      }
    }
  }
  printf("O PAI VAI COMECAR A AGUARDAR CONCLUSAO.\n Numero de processos ativos:%d \n", activeProcesses);
  // Aguardar a conclusão de todos os processos filhos restantes
  while (activeProcesses > 0) {
      wait(&status);
      //printf("%d \n",status);
      printf("O processo %d terminou pq o pai aguardou a sua conclusão.\n", status);
      activeProcesses--;
  }

  closedir(dir);

  return 0;
}


/* Função que cria o path para o ficheiro de input */
char *pathingJobs(char *directoryPath, struct dirent *entry) {

  size_t length = strlen(directoryPath) + strlen(entry->d_name) + 2;
  char *pathJobs = (char *)malloc(length);
  snprintf(pathJobs, length, "%s/%s", directoryPath, entry->d_name);

  return pathJobs;
}


/* Função que cria o path para o ficheiro de output */
char* pathingOut(const char *directoryPath, struct dirent *entry) {
    const char *extension_to_remove = ".jobs";
    const char *new_extension = ".out";

    // Encontrar a posição da extensão ".jobs" na string
    const char *extension_position = strstr(entry->d_name, extension_to_remove);

    // Calcular o comprimento da parte do nome do arquivo antes da extensão ".jobs"
    size_t directory_length = strlen(directoryPath);
    size_t prefix_length = strlen(entry->d_name) - strlen(extension_position); //entry->d_name é um apontador para o inicio da string e extension_position é um apontador para onde o .jobs começa

    // Alocar dinamicamente memória para a nova string
    char *pathFileOut = (char *)malloc(directory_length + prefix_length + strlen(new_extension) + 2);

    // Copiar o diretório para a nova string
    strcpy(pathFileOut, directoryPath); //copia a diretoria
    strcat(pathFileOut, "/"); //adiciona a barra
    strncat(pathFileOut, entry->d_name, prefix_length); //adiciona até da à diretoria com a barra o entry->d_name apenas até à posição do prefixo
    strcat(pathFileOut, new_extension); //adiciona a nova extensão

    return pathFileOut;
}


/* Função que processa o ficheiro de input e chama as funções que fazem as operações */
int process_file(char* pathJobs, char* pathOut) {

  int fdRead = open(pathJobs, O_RDONLY);
  int fdWrite = open(pathOut, O_CREAT | O_TRUNC | O_WRONLY , S_IRUSR | S_IWUSR);
  int line = 1;
  unsigned int thread_id = 0;

  pthread_t threads[global_num_threads];
  ThreadParameters *parameters = createThreadParameters(fdWrite);

  while (1) {
    unsigned int event_id, delay=10; //tá mal pq o delay n tá certo, n tá inicializado a nd
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

    if (parameters->active_threads < global_num_threads) {
      sleep(1); //sem isto dá erros (código atropela-se)

      if (line == (int)thread_id && delay > 0) {
        printf("Waiting... com thread ID %d\n", thread_id);
        ems_wait(delay);
        thread_id = 0; // ????
      }

      parameters->thread_index = -1;
      for (int i = 0; i < global_num_threads; ++i) {
        if (!parameters->thread_active_array[i]) {
          parameters->thread_index = i;
          break;
        }
      }

      switch (get_next(fdRead)) {
        case CMD_CREATE:
          if (parse_create(fdRead, &event_id, &num_rows, &num_columns) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }          

          parameters->event_id = event_id;
          parameters->num_rows = num_rows;
          parameters->num_columns = num_columns;

          pthread_create(&threads[parameters->thread_index], NULL, ems_create, (void*)&parameters);
          line++;
          parameters->active_threads++;
          parameters->thread_active_array[parameters->thread_index] = 1;

          break;

        case CMD_RESERVE:
          num_coords = parse_reserve(fdRead, MAX_RESERVATION_SIZE, &event_id, xs, ys);

          if (num_coords == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          parameters->event_id = event_id;
          parameters->num_coords = num_coords;
          parameters->xs = xs;
          parameters->ys = ys;

          pthread_create(&threads[parameters->thread_index], NULL, ems_reserve, (void*)&parameters);
          line++;
          parameters->active_threads++;
          parameters->thread_active_array[parameters->thread_index] = 1;

          break;

        case CMD_SHOW:
          if (parse_show(fdRead, &event_id) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }
          parameters->event_id = event_id;
          parameters->file_descriptor = fdWrite;

          pthread_create(&threads[parameters->thread_index], NULL, ems_show, (void*)&parameters);
          line++;
          parameters->active_threads++;
          parameters->thread_active_array[parameters->thread_index] = 1;

          break;

        case CMD_LIST_EVENTS:
          if (ems_list_events(fdWrite)) {
            fprintf(stderr, "Failed to list events\n");
          }

          break;

        case CMD_WAIT:
          if (parse_wait(fdRead, &delay, &thread_id) == -1) {  // thread_id is not implemented
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }
          if (delay > 0 && thread_id == 0) {
            printf("Waiting...\n");
            ems_wait(delay);
          }

          break;

        case CMD_INVALID:
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          break;

        case CMD_HELP:
          printf(
              "Available commands:\n"
              "  CREATE <event_id> <num_rows> <num_columns>\n"
              "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
              "  SHOW <event_id>\n"
              "  LIST\n"
              "  WAIT <delay_ms> [thread_id]\n"
              "  BARRIER\n"
              "  HELP\n");

          break;

        case CMD_BARRIER:
          for (int i = 0; i < global_num_threads; i++) {
            if (parameters->thread_active_array[i]) {
              pthread_join(threads[i], NULL);
            }
          }
          break;
        case CMD_EMPTY:
          break;

        case EOC:
          for (int i = 0; i < global_num_threads; i++) {
            if (parameters->thread_active_array[i]) {
              pthread_join(threads[i], NULL);
            }
          }
          destroyThreadParameters(parameters);
          close(fdRead);
          close(fdWrite);
          ems_terminate();
          return 0;
      }
    }
  }
  return 0;
}


/* Função que cria a estrutura de dados que contém os parâmetros que são passados às threads */
ThreadParameters *createThreadParameters(int fdWrite) {
  ThreadParameters *params = (ThreadParameters *)malloc(sizeof(ThreadParameters));

  // Inicializa os parâmetros
  params->thread_index = 0;
  params->active_threads = 0;
  params->file_descriptor = fdWrite;
  pthread_rwlock_init(&params->rwlock, NULL);
  //pthread_mutex_init(&params->mutex, NULL);

  // Aloca memória para thread_active_array
  params->thread_active_array = (int *)malloc((size_t)global_num_threads * sizeof(int));


  if (params->thread_active_array == NULL) {
      fprintf(stderr, "Erro ao alocar memória para thread_active_array\n");
      free(params);
      exit(EXIT_FAILURE);
  } else { // Inicializa o array
    for (int i = 0; i < global_num_threads; i++) {
      params->thread_active_array[i] = 0;
    }
  }

  return params;
}


/* Função que destroi a estrutura de dados que contém os parâmetros que são passados às threads */
void destroyThreadParameters(ThreadParameters *params) {
  // Liberta a memória alocada
  if (params != NULL) {
    free(params->thread_active_array);
    pthread_rwlock_destroy(&params->rwlock);
    //pthread_mutex_destroy(&params->mutex);
    free(params);
  }
}
