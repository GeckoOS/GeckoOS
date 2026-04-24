#pragma once


typedef struct {
    void **data;
    unsigned int head;
    unsigned int tail;
} Queue;
int init_queue();
int enqueue(Queue *q, int v);
int dequeue(Queue *q, int *v);