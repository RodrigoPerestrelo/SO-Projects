#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Queue* initializeQueue() {
    Queue* q = (Queue*) malloc(sizeof(Queue));
    if (!q) {
        // Tratamento de erro: falha na alocação de memória
        return NULL;
    }
    q->head = q->tail = NULL;
    return q;
}

int isEmptyQueue(Queue* q) {
    return q->head == NULL;
}

void addToQueue(Queue* q, const char* data) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    if (!newNode) {
        // Tratamento de erro: falha na alocação de memória
        return;
    }
    strcpy(newNode->data, data);
    newNode->next = NULL;

    if (isEmptyQueue(q)) {
        q->head = q->tail = newNode;
        return;
    }

    q->tail->next = newNode;
    q->tail = newNode;

}

void removeHeadQueue(Queue* q) {
    if (isEmptyQueue(q)) {
        //tratar erro
        return;
    }
    
    Node* temp = q->head;
    q->head = q->head->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }

    free(temp);
}

char* getHeadQueue(Queue* q) {
    if (isEmptyQueue(q)) {
        // Tratamento de erro: fila vazia
        return NULL;
    }
    
    return q->head->data;
}