#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Struct representing a node in a queue_t
 */
typedef struct queue_node_t {

  struct queue_node_t *next;
  void *data;
} queue_node_t;

/**
 * Struct representing a queue
 */
struct queue_t {

  queue_node_t *head, *tail;
  int size;
  int maxSize;
  pthread_cond_t cv;
  pthread_mutex_t m;
};





/**
 *  Given data, place it on the queue.  Can be called by multiple threads.
 *  Blocks if the queue is full.
 */
void queue_push(queue_t *queue, void *data) {
 /* Your code here */

    pthread_mutex_lock(&queue->m);

    queue_node_t *newElement=malloc(sizeof(queue_node_t));
    newElement->data=data;
    newElement->next=NULL;



    if(queue->maxSize<=0){
        //never block

        if(queue->size){

                queue->tail->next=newElement;
                queue->tail=newElement;
                queue->size++;

        }else{
            //empty
           queue->head=newElement;
           queue->tail=newElement;
           queue->size++;
        }
    }else{
        //full
        while(queue->size==queue->maxSize){
            pthread_cond_wait(&queue->cv,&queue->m);
        }
        if(queue->size){

                queue->tail->next=newElement;
                queue->tail=newElement;
                queue->size++;

        }else{
            //empty
           queue->head=newElement;
           queue->tail=newElement;
           queue->size++;
        }


    }
    pthread_cond_signal(&queue->cv);
    pthread_mutex_unlock(&queue->m);
}
          


/**
 *  Retrieve the data from the front of the queue.  Can be called by multiple
 * threads.
 *  Blocks if the queue is empty.
 */
void *queue_pull(queue_t *queue) {
  /* Your code here */
    pthread_mutex_lock(&queue->m);
    void* ret=NULL;


    while(queue->size==0){
        pthread_cond_wait(&queue->cv,&queue->m);
    }


    if(queue->size>1){
            ret=queue->head->data;
            queue_node_t * temp=queue->head->next;
            free(queue->head);
            queue->head=temp;
            queue->size--;

    }else{
        ret=queue->head->data;
        free(queue->head);
        queue->head=NULL;
        queue->tail=NULL;
        queue->size--;
    }





    pthread_cond_signal(&queue->cv);
    pthread_mutex_unlock(&queue->m);
    return ret;
}


/**
 *  Allocates heap memory for a queue_t and initializes it.
 *  Returns a pointer to this allocated space.
 */
queue_t *queue_create(int maxSize) {
  /* Your code here */
    queue_t* q=malloc(sizeof(queue_t));
    q->head=NULL;
    q->tail=NULL;
    q->size=0;
    q->maxSize=maxSize;
    pthread_mutex_init(&q->m, NULL);
      pthread_cond_init(&q->cv, NULL);

    return q;
}

/**
 *  Destroys the queue, freeing any remaining nodes in it.
 */
void queue_destroy(queue_t *queue) {


/* Your code here */

    pthread_mutex_destroy(&queue->m);
    pthread_cond_destroy(&queue->cv);



    for(int i=0;i<queue->size;i++){
        queue_node_t * temp=queue->head->next;
        free(queue->head);
        queue->head=temp;

    }

    free(queue);

}
      