#include "list.h"
#include <stdlib.h>
#include "common.h"


Node *createNode(void *data) {
    Node *newNode = (Node *) malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void initList(List *list) {
    list->head = NULL;
    list->tail = NULL;
}

void addNode(List *list, void *data) {
    Node *newNode = createNode(data);

    if (list->head == NULL) {
        list->head = newNode;
        list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
}

void freeList(List *list) {
    Node *current = list->head;
    Node *temp;

    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp);
    }
}

void removeNode(List *list, void *data) {
    Node *current = list->head;
    Node *prev = NULL;

    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }


    if (current != NULL) {
        if (current == list->head) {
            list->head = current->next;
            if (list->head == NULL) {
                list->tail = NULL;
            }
        } else if (current == list->tail) {
            list->tail = prev;
            prev->next = NULL;
        } else {
            prev->next = current->next;
        }
        free(current);
    }
}
