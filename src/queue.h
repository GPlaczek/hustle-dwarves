#ifndef QUEUE_H
#define QUEUE_H

#define MAX_SIZE 100

typedef struct {
    void *data[MAX_SIZE];
    int front;
    int rear;
} Queue;


void initQueue(Queue *queue);
int isFull(Queue *queue);
int isEmpty(Queue *queue);
void enqueue(Queue *queue, void *item);
void *dequeue(Queue *queue);

#endif
