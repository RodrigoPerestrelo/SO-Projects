#ifndef QUEUE_H
#define QUEUE_H

typedef struct Node {
    char data[256];
    struct Node* next;
} Node;

typedef struct Queue {
    Node* head;
    Node* tail;
} Queue;

Queue* initializeQueue();
int isEmptyQueue(Queue* q);
void addToQueue(Queue* q, const char* data);
void removeHeadQueue(Queue* q);
char* getHeadQueue(Queue* q);


#endif
