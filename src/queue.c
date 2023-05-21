#include "queue.h"
#include <stddef.h>


void initQueue(Queue *queue) {
    queue->front = -1;
    queue->rear = -1;
}

int isFull(Queue *queue) {
    return (queue->rear == MAX_SIZE - 1);
}

int isEmpty(Queue *queue) {
    return (queue->front == -1);
}

void enqueue(Queue *queue, void *item) {
    if (isFull(queue)) {
        return;
    }

    if (isEmpty(queue))
        queue->front = 0;

    queue->rear++;
    queue->data[queue->rear] = item;
}

void *dequeue(Queue *queue) {
    void *item;

    if (isEmpty(queue)) {
        return NULL;
    }

    item = queue->data[queue->front];
    if (queue->front == queue->rear) {
        // Last element in the queue
        queue->front = -1;
        queue->rear = -1;
    } else {
        queue->front++;
    }

    return item;
}
