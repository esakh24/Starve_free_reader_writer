# Starve_free_reader_writer

To provide starve free solution, following semaphores and shared variables are used:

**SEMAPHORES**

- **in** : protects the reader_in_counter variable against conflicting accesses.
- **out** : protects the reader_out_counter variable against conflicting accesses.
- **write_sem** : this semaphore is used by writers to indicate their presence in case of readers already reading the data in their critical section. If there is a writer waiting, the last reader will signal this semaphore

**SHARED VARIABLES**

- **reader_in_counter** : counts the no of readers which have entered into the critical section .
- **reader_out_counter** : counts the no of readers which have left the critical section after reading.
- **writer_waiting** : Boolean variable to indicate whether writer is waiting in the queue for writing while some reader/readers are reading.

**_EXPLANATON OF LOGIC:_**

**Reader**

- First it should acquire the `in` semaphore to increment the `reader_in_counter` and then releases it.
- And then it can read the data. After reading, to exit, it must call upon the `out` semaphore to increment the `reader_out_counter`.
- Also, if it is the last reader reading in the CS and a writer is already waiting it will signal the `write_sem` semaphore so that the writer can start its work.

The code for reader is as follows:

```c
void *reader(void *arg)
{
   sem_wait(&in);
   reader_in_counter++;
   sem_post(&in);

   // Critical section
   // read data
   // sleep(1);
   // Critical section end

   sem_wait(&out);
   reader_out_counter++;
   if (writer_waiting == true && reader_in_counter == reader_out_counter)
   {
       sem_post(&write_sem);
   }
   sem_post(&out);
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

    // Critical section
    //sleep(2);
    data = data + 3;
    // Critical section end

    sem_post(&in);
}
```

**NO STARVATION**

Any reader or writer for starting its activity must acquire the `in` semaphore. The reader will release it after incrementing its count, while the writer will release only when finished writing. This ensures that multiple readers can enter into CS simultaneously but only till a writer arrives. After which all readers and writers will be blocked untill first all the readers in CS finish execution, signal the writer to allow writing and writer releases the semaphore `in`. Now one among all the threads waiting in the semaphore `in` wait queue will be executed according to scheduling by the OS. Hence, both readers and writers have fair chance of execution eliminating any starvation.

To run code on Linux use
```
   gcc -pthread source.c
   ./a.out
```
