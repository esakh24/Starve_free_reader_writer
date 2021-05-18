#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

sem_t in, out, write_sem; // declare semaphore

int reader_in_counter = 0, reader_out_counter = 0;
bool writer_waiting = false;
int data = 1; // Shared data

void *reader(void *arg);
void *writer(void *arg);

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

    //destroy threads after execution
    sem_destroy(&in);
    sem_destroy(&out);
    sem_destroy(&write_sem);

    return 0;
}

void *writer(void *arg)
{
    int temp = (int)arg;
    printf("\nWriter %d is trying to write data", temp);
    sem_wait(&in);
    sem_wait(&out);
    if (reader_in_counter == reader_out_counter)
    {
        sem_post(&out);
    }

    else
    {
        writer_waiting = true;
        sem_post(&out);
        sem_wait(&write_sem);
        writer_waiting = false;
    }

    // CRITICAL section
    //sleep(2);
    printf("\nWriter %d is writing data", temp);
    data = data + 3;
    // CRITICAL section end

    printf("\nWriter %d wrote the data %d ", temp, data);

    sem_post(&in);
}

void *reader(void *arg)
{
    int temp = (int)arg;

    printf("\nReader %d is trying to read data ", temp);
    sem_wait(&in);
    reader_in_counter++;
    sem_post(&in);

    // CRITICAL section
    // sleep(1);
    printf("\nReader %d read the data %d", temp, data);
    // CRITICAL section end

    sem_wait(&out);
    reader_out_counter++;
    if (writer_waiting == true && reader_in_counter == reader_out_counter)
    {
        sem_post(&write_sem);
    }
    sem_post(&out);
}
