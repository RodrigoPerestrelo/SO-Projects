#ifndef QUEUE_H
#define QUEUE_H

typedef struct Node {
    //O 100 tá mal, mas os pipes tao numa directoria com 55 carateres
    char requestPipe[100];
    char responsePipe[100];
    struct Node* next;
} Node;

typedef struct Queue {
    Node* head;
    Node* tail;
} Queue;

Queue* initializeQueue();
int isEmptyQueue(Queue* q);
void addToQueue(Queue* q, const char* bufferRequest, const char* bufferResponse);
void removeHeadQueue(Queue* q);
Node* getHeadQueue(Queue* q);


#endif
