#ifndef LIST_H
#define LIST_H


typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct List {
    Node *head;
    Node *tail;
} List;


void initList(List *list);
void addNode(List *list, void *data);
void freeList(List *list);
void removeNode(List *list, void *data);

#endif
