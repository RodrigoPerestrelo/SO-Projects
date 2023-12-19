#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

void lerArquivo(const char *caminho) {
    int fd = open(caminho, O_RDONLY);

    if (fd == -1) {
        perror("Erro ao abrir o arquivo");
        // Tratar o erro conforme necessário
        return;
    }

    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        // Processa os dados lidos (por exemplo, imprimir no console)
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    if (bytesRead == -1) {
        perror("Erro ao ler o arquivo");
        // Tratar o erro conforme necessário
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <diretorio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *diretorio = argv[1];

    DIR *dir = opendir(diretorio);

    if (dir == NULL) {
        perror("Erro ao abrir o diretório");
        return EXIT_FAILURE;
    }

    struct dirent *entrada;

    while ((entrada = readdir(dir)) != NULL) {
        // Ignora entradas especiais . e ..
        if (strcmp(entrada->d_name, ".") != 0 && strcmp(entrada->d_name, "..") != 0) {
            // Verifica se a extensão é .jobs
            size_t tamanho = strlen(entrada->d_name);
            if (tamanho > 5 && strcmp(entrada->d_name + tamanho - 5, ".jobs") == 0) {
                // Constrói o caminho completo
                char caminhoCompleto[PATH_MAX];
                snprintf(caminhoCompleto, sizeof(caminhoCompleto), "%s/%s", diretorio, entrada->d_name);

                // Lê o conteúdo do arquivo
                printf("Conteúdo de %s:\n", caminhoCompleto);
                lerArquivo(caminhoCompleto);
                printf("\n");
            }
        }
    }

    closedir(dir);

    return EXIT_SUCCESS;
}
