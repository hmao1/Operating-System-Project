The thread library mainly consists of 3 functions, pthread_create(), pthread_exit, and
pthread_exit respectively.

#### TCB:
For the TCB(thread control table), I used array with size of 128 to store the thread context
(each thread context contain thread id, jmpbuf, and status). Specifially, I add the main thread
into the TCB and assign it with the id of 0. I also manually push the pthread_exit() to the top
of the stack so that we the start routine finish execution, it will return to pthreat_exit()

#### scheduler
For the scheduler(which is also the signal handler), once the alarm goes off,
scheduler will save the current context of running thread and switch to next ready thread.
when there is only 1 thread left over( the main thread), the scheduler will just pick that only left
over thread. To acheive round robin, I use the curent_thread_index % created_thread number, so that the
array index circle through until it gets to the next ready thread.

#### pthread_exit:

Once the thread is done with its execution, it will jump to the address of pthread_exit(since

it is address I manually push at the bottom of the stack frame for each created thread, except for
main thread)
