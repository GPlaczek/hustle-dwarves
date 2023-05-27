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

void *dequeueAt(Queue *queue, int index) {
    void *item;

    if (isEmpty(queue) || index < 0 || index > queue->rear) {
        return NULL;
    }

    item = queue->data[index];
    for (int i = index; i < queue->rear; i++) {
        queue->data[i] = queue->data[i + 1];
    }
    queue->rear--;

    if (queue->front > queue->rear) {
        // Queue is empty after dequeuing at a specific index
        queue->front = -1;
        queue->rear = -1;
    }

    return item;
}
