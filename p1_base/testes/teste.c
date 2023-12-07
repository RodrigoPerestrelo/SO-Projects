#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCREMENTO 10


int main() {

    char *buffer = (char *)malloc(INCREMENTO * sizeof(char));
    strcat(buffer, "avcpo,c");
    printf("%ld\n", strlen(buffer));
    return 0;
}
