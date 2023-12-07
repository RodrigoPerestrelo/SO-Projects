#ifndef TESTE_H
#define TESTE_H
#include <string.h>

int writeFile(int fd, char* buffer);

char* adicionarStringAoBuffer(char *buffer, size_t tamanho_atual, size_t bufferLenght, char *nova_string);

#endif

