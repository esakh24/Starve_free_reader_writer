# Starve-Free Readers Writers Solution

Submitted by - Sakshi Verma (19114074)

The readers-writers problem is a classical synchronisation problem in computer science: we have a resource that can be accessed by readers, who do not modify the resource just read the contents, and writers, who can modify the resource. Thus, it is a necessity that when a writer is modifying the resource, no-one else (reader or writer) should access it at the same time: another writer could corrupt the resource, and another reader could read a partially modified value.However multiple readers can simultaneously read it.

This problem has **three variants**:

**1. Give readers priority** : when there is at least one reader currently reading the data, allow new readers to access it as well.Hence, it is possible that a writer could wait indefinitely while a stream of readers arrived.

**2. Give writers priority** : we give priority to writers and allow readers to wait indefinitely while a stream of writers is working.

**3. Starve free approach** : all readers and writers will be granted access to the resource in their order of arrival. If a writer arrives while readers are accessing the resource, it will wait until those readers free the resource, and then modify it. New readers arriving in the meantime will have to wait.Hence starvation is not possible

To provide starve free solution, following approach is used:

**Implementation of semaphore in c-:**

Each semaphore has a value and a Queue associated with it. To ensure starvation doesnt occur, the queue is implemented as FIFO.

Struct for semaphore -

```c
typedef struct
{
    int s;                    // semaphore atomic varible
    Queue Q;                  // queue for storing suspended threads
    pthread_mutex_t Spinlock; //to make wait and signal atomic, this lock is used
} sem_t;
```

Function to initialise semaphore with given value -

```c
void sem_init(sem_t *temp, int pshared, unsigned int value)
{
    temp->s = value;
}
```

Semaphore has two major methods- `wait` and `signal`. `wait` decreases the semaphore count by one and adds process to suspended queue until semaphore is released by other process. `signal` increments the semaphore value by 1 and wakes the next process in the suspended queue.

Both these operations must be atomic i.e. no two processes can call wait or signal at the same time. So for mutual exclusion `pthread_mutex_lock` mutex lock for pthreads is used.

Wait operation -

```c
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
```

Signal operation -

```c

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

```

Queue for semaphores -

```c
typedef struct Queue
{
    pthread_mutex_t QueueMutex; //mutex lock to ensure mutual exclusion
                                //during enqueue qnd dequeue operations on a semaphore
    int size;
    node *front; //points to first elem in queue
    node *rear;  //points to last elem of queue
} Queue;
```

Struct for each process in queue of a semaphore of suspended threads -

```c
typedef struct
{
    pthread_cond_t *value; //process id of a process
    struct node *next;     //next elem in queue
} node;
```

Both insertion and deletion from this queue must be atomic else it might lead to inconsistent state. So for mutual exclusion `pthread_mutex_lock` mutex lock for pthreads is used.

Function to insert suspended threads in queue -

```c

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
```

Function to remove thread from queue for suspended threads -

```c
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

```

The solution uses :

**SEMAPHORES** (with FIFO waiting queues)

- **in** (initialised to 1) : protects the reader_in_counter variable against conflicting accesses.hence, ensurese mutual exclusion.
- **out** (initialised to 1) : protects the reader_out_counter variable against conflicting accesses.hence, ensurese mutual exclusion.
- **write_sem** (initialised to 0) : this semaphore is used by writers to indicate their presence in case of readers already reading the data in their critical section. If there is a writer waiting, the last reader will signal this semaphore

**SHARED VARIABLES**

- **reader_in_counter** : maintains the counter of readers which have entered into the critical section till that time.
- **reader_out_counter** : maintains the counter of readers which have left the critical section till that time.
- **writer_waiting** : Boolean variable to indicate whether writer is waiting in the queue of `in` semaphore for writing while some reader/readers are reading.

```c
sem_t in, out, write_sem; // declare semaphore

int reader_in_counter = 0, reader_out_counter = 0;
bool writer_waiting = false;
int data = 1; // Shared data
```

**_EXPLANATON OF LOGIC:_**

**Reader**

- First it should acquire the `in` semaphore to increment the `reader_in_counter` and then releases it.
- And then it can read the data. After reading, to exit, it must call upon the `out` semaphore to increment the `reader_out_counter`.
- Also, if it is the last reader reading in the CS and a writer is already waiting it will signal the `write_sem` semaphore so that the writer can start its work.

The code for reader is as follows:

```c
void *reader(void *arg)
{
   wait(&in);
   reader_in_counter++;
   signal(&in);

   // Critical section
   // read data
   // Critical section end

   wait(&out);
   reader_out_counter++;
   if (writer_waiting == true && reader_in_counter == reader_out_counter)
   {
       signal(&write_sem);
   }
   signal(&out);
}

```

**Writer**

- It must call wait on both `in` and `out` semaphores as it uses both the shared variables `reader_in_counter` and `reader_out_counter` to determine its next action.
- If both of these are equal, it means there is no reader present in the CS,the writer sets to write after signalling the `out` semaphore.
- Else, some reader(s) are in their CS. The writer sets the `writer_waiting` variable to true and wait on the `write_sem` semaphore, so that the last reader reading in the CS at the moment would be notified of its presence and latter can signal the `write_sem` semaphore once it's done reading. Upon receiving the signal, writer sets the `writer_waiting` to false and enters into its CS . When done, it would release the `in` semaphore.

The code for writer is as follows:

```c

void *writer(void *arg)
{
    wait(&in);
    wait(&out);
    if (reader_in_counter == reader_out_counter)
    {
        sem_post(&out);
    }

    else
    {
        writer_waiting = true;
        signal(&out);
        wait(&write_sem);
        writer_waiting = false;
    }

    // Critical section
    //modify data
    data = data + 3;
    // Critical section end

    signal(&in);
}
```

**NO STARVATION**

Any reader or writer for starting its activity must acquire the `in` semaphore. The reader will release it after incrementing its count, while the writer will release only when finished writing. This ensures that multiple readers can enter into CS simultaneously but only till a writer arrives. After which all readers and writers will be blocked untill -

- first all the readers in CS finish execution,
- signal the writer to allow writing
- and writer releases the semaphore `in`.

Now the first thread in the waiting queue of the semaphore `in` will be executed. Hence, both readers and writers have fair chance of execution in order of their arrival in this queue eliminating any starvation.

To run code on Linux use

```
   gcc -pthread source.c
   ./a.out
```
