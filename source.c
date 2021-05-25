#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

// struct for each element in queue of a semaphore of suspended threads
typedef struct
{
    pthread_cond_t *value; //process id of a process
    struct node *next;     //next elem in queue
} node;

//struct for wait queue of Semaphore
typedef struct Queue
{
    pthread_mutex_t QueueMutex; //mutex lock tto ensure mutual exclusion
                                //during enque qnd deque operations on a semaphore
    int size;
    node *front; //points to first elem in queue
    node *rear;  //points to last elem of queue
} Queue;

//struct for semaphore
typedef struct
{
    int s;                    // semaphore atomic varible
    Queue Q;                  // queue for storing suspended threads
    pthread_mutex_t Spinlock; //to make wait and signal atomic, this lock is used
} sem_t;
// insert suspended threads in queue
int enqueue(struct Queue *q, pthread_cond_t *con)
{

    pthread_mutex_lock(&q->QueueMutex);        //put a lock on queue operations
    node *temp = (node *)malloc(sizeof(node)); //create new node

    temp->value = con; //initialise its pid value
    temp->next = NULL; //assign NULL to next
    //if queue isi empty, assign it as front node
    if (q->front == NULL)
    {
        q->front = temp;
        q->rear = temp;
        q->size = 0;
    }
    //else add it to rear
    else
    {
        q->rear->next = temp;
        q->rear = temp;
    }
    //increase size of queue
    q->size += 1;
    //release the lock once done
    pthread_mutex_unlock(&q->QueueMutex);
}
// remove element from queue for suspended threads
pthread_cond_t *dequeue(struct Queue *q)
{
    pthread_mutex_lock(&q->QueueMutex); //put a lock on queue operations
    //if queue is empty,release the lock and return NULL
    if (q->size <= 0)
    {
        pthread_mutex_unlock(&q->QueueMutex);
        return NULL;
    }
    //release the front node and return
    else
    {

        pthread_cond_t *releasedSem = q->front->value;

        if (q->size == 1)
        {
            q->front = NULL;
            q->rear = NULL;
            q->size = 0;
        }
        else
        {
            q->size--;
            q->front = q->front->next;
        }
        pthread_mutex_unlock(&q->QueueMutex);

        return releasedSem;
    }

    return NULL;
}

//wait(S)
void wait(sem_t *temp)
{
    //Acquire mutex lock for the semaphore's wait/signal operations
    pthread_mutex_lock(&temp->Spinlock);
    //decrement value of semaphore
    temp->s--;
    //if value<0, then add the thread to suspended queue of semaphore
    if (temp->s < 0)
    {
        pthread_cond_t *cond_temp = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
        enqueue(&temp->Q, cond_temp);
        pthread_cond_wait(cond_temp, &temp->Spinlock);
    }

    pthread_mutex_unlock(&temp->Spinlock); //release lock once done
}
//signal(S)
void signal(sem_t *temp)
{
    //Acquire mutex lock for the semaphore's wait/signal operations
    pthread_mutex_lock(&temp->Spinlock);
    //increment semaphore value by 1
    temp->s++;
    //if processes are waiting in queue, wake the first process in queue for execution
    if (temp->s <= 0)
    {
        pthread_cond_t *cond = dequeue(&temp->Q);
        if (cond != NULL)
        {
            pthread_cond_signal(cond);
        }
    }
    pthread_mutex_unlock(&temp->Spinlock);
    // release lock once done
}
// funnction to initialise semaphore with given value
void sem_init(sem_t *temp, int pshared, unsigned int value)
{
    temp->s = value;
}

sem_t in, out, write_sem; // declare semaphore

int reader_in_counter = 0, reader_out_counter = 0;
bool writer_waiting = false;
int data = 1; // Shared data

void *writer(void *arg)
{
    int temp = (int)arg;
    printf("\nWriter %d is wanting to write data", temp);
    wait(&in);
    wait(&out);
    if (reader_in_counter == reader_out_counter)
    {
        signal(&out);
    }

    else
    {
        writer_waiting = true;
        signal(&out);
        wait(&write_sem);
        writer_waiting = false;
    }

    // CRITICAL section
    //sleep(2);
    printf("\nWriter %d is writing data", temp);
    data = data + 3;
    // CRITICAL section end

    printf("\nWriter %d wrote the data %d ", temp, data);

    signal(&in);
}

void *reader(void *arg)
{
    int temp = (int)arg;

    printf("\nReader %d is wanting to read data ", temp);
    wait(&in);
    reader_in_counter++;
    signal(&in);

    // CRITICAL section
    printf("\nReader %d read the data %d", temp, data);
    // CRITICAL section end

    wait(&out);
    reader_out_counter++;
    if (writer_waiting == true && reader_in_counter == reader_out_counter)
    {
        signal(&write_sem);
    }
    signal(&out);
}
//to test a random sequence of readers and writers
int main()
{
    int i = 0, reader_count, writer_count, max;
    // initialise semaphores
    sem_init(&in, 0, 1);
    sem_init(&out, 0, 1);
    sem_init(&write_sem, 0, 0);

    printf("\nEnter no of Readers ");
    scanf("%d", &reader_count);
    printf("\nEnter no of Writers ");
    scanf("%d", &writer_count);

    max = reader_count >= writer_count ? reader_count : writer_count;
    int id_arr[max];

    for (int i = 0; i < max; i++)
        id_arr[i] = i + 1;

    pthread_t Rthread_list[reader_count], Wthread_list[writer_count];

    //creating required number of reader and writer threads
    for (i = 0; i < reader_count; i++)
        pthread_create(&Rthread_list[i], NULL, reader, (void *)i);

    for (i = 0; i < writer_count; i++)
        pthread_create(&Wthread_list[i], NULL, writer, (void *)i);

    //allow the calling thread i.e. main to wait for the ending of the reader and writer threads.
    for (i = 0; i < writer_count; i++)
        pthread_join(Wthread_list[i], NULL);

    for (i = 0; i < reader_count; i++)
        pthread_join(Rthread_list[i], NULL);

    return 0;
}
