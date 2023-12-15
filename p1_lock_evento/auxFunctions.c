#include "auxFunctions.h"
#include "main.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define INCREMENTO 128

/* Função que cria o path para o ficheiro de input */
char *pathingJobs(char *directoryPath, struct dirent *entry) {

  size_t length = strlen(directoryPath) + strlen(entry->d_name) + 2;
  char *pathJobs = (char *)malloc(length);

  if (pathJobs == NULL) {
    fprintf(stderr, "Erro: Falha na alocação de memória\n");
    return NULL;
  }

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

  if (pathFileOut == NULL) {
      fprintf(stderr, "Erro: Falha na alocação de memória\n");
      return NULL;
  }

  // Copiar o diretório para a nova string
  strcpy(pathFileOut, directoryPath); //copia a diretoria
  strcat(pathFileOut, "/"); //adiciona a barra
  strncat(pathFileOut, entry->d_name, prefix_length); //adiciona até da à diretoria com a barra o entry->d_name apenas até à posição do prefixo
  strcat(pathFileOut, new_extension); //adiciona a nova extensão

  return pathFileOut;
}


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

