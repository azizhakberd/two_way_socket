#ifndef QUEUE_H
#define QUEUE_H

#include<stdio.h>
#include<stdlib.h>
#include "./dynamicPtr.h"

    // Declare the queue array and front, rear variables
typedef struct
{
    int front;
    int rear;
    int max_size;
    dynamicPtr* buffer;
} Queue;

Queue* createQueue(int max_size)
{
    Queue* q = malloc(sizeof(Queue));
    q->front = -1;
    q->rear = -1;
    q->max_size = max_size;
    q->buffer = malloc(max_size*sizeof(dynamicPtr));

    return q;
}

// Function to check if the queue is full
int isFull(Queue* q)
{
    // If the next position is the front, the queue is full
    return (q->rear + 1) % q->max_size == q->front;
}

// Function to check if the queue is empty
int isEmpty(Queue* q)
{
    // If the front hasn't been set, the queue is empty
    return q->front == -1;
}

// Function to enqueue (insert) an element
void enqueue(Queue* q, dynamicPtr data)
{
    // If the queue is full, print an error message and
    // return
    if (isFull(q)) {
        printf("Queue overflow\n");
        return;
    }
    // If the queue is empty, set the front to the first
    // position
    if (q->front == -1) {
        q->front = 0;
    }
    // Add the data to the queue and move the rear pointer
    q->rear = (q->rear + 1) % q->max_size;
    q->buffer[q->rear] = data;
}

// Function to dequeue (remove) an element
dynamicPtr dequeue(Queue* q)
{
    // If the queue is empty, print an error message and
    // return -1
    if (isEmpty(q)) {
        printf("Queue underflow\n");
        dynamicPtr error;
        error.type = ERROR;
        error.data = (void*)(-1);
        return error;
    }
    // Get the data from the front of the queue
    dynamicPtr data = q->buffer[q->front];
    // If the front and rear pointers are at the same
    // position, reset them
    if (q->front == q->rear) {
        q->front = q->rear = -1;
    }
    else {
        // Otherwise, move the front pointer to the next
        // position
        q->front = (q->front + 1) % q->max_size;
    }
    // Return the dequeued data
    return data;
}

#endif