#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "thread_impl.h"
#include <utility> 


using std::cout;
using std::endl;

cv cv_waiting;
mutex m;
int global = 0;
int waiting = 0;

void childB(thread* args)
{
    printf("Starting thread B, on CPU: %p \n", cpu::self());
    m.lock();
    printf("thread b is the %i to go",++waiting);
    printf("Thread B signals on CPU: %p \n", cpu::self());
    cv_waiting.signal();
    m.unlock();
    printf("Join on C during thread B on CPU: %p \n", cpu::self());

    args->join();
    printf("Thread B exiting on CPU: %p \n", cpu::self());
}
void childD(thread* args)
{
    printf("Starting thread D on CPU: %p \n", cpu::self());
    m.lock();
    ++waiting;
    printf("Thread D signals on CPU: %p \n", cpu::self());
    cv_waiting.signal();
    m.unlock();
    printf("Join on C during thread D on CPU: %p \n", cpu::self());

    args->join();
    printf("Thread D exiting on CPU: %p \n", cpu::self());
}

void childC(thread* args){
    printf("Start thread C on CPU: %p \n", cpu::self());
    m.lock();
    while (waiting < 2)
    {
        printf("Thread C is waiting CPU: %p \n", cpu::self());
        cv_waiting.wait(m);
    }
    m.unlock();
    printf("finishing thread C on CPU: %p \n", cpu::self());
    
}


void A(intptr_t args)
{
    printf("Running thread: A, on CPU: %p \n", cpu::self());
    
    thread c = thread((thread_startfunc_t) childC, (void*) &c);
    thread b = thread((thread_startfunc_t) childB, (void*) &c);
    thread d = thread((thread_startfunc_t) childD, (void*) &c);

    printf("Created threads: , on CPU: %p \n", cpu::self());

   
    m.lock();
    ++waiting;
    cv_waiting.signal();
    printf("Thread A signaled, on CPU: %p \n", cpu::self());
    m.unlock();
    printf("Thread A joins C , on CPU: %p \n", cpu::self());
    c.join();

    printf("Thread A running, on CPU: %p \n", cpu::self());

    printf("Finishing thread: A, on CPU: %p \n", cpu::self());

}

int main()
{
    cpu::boot(2, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}

/*
Running thread: A
Created threads:
Thread C joins A
Running thread: B
Thread C joins thread B
Running thread: C
Thread C is doing work
Finishing thread C
Input Arg: 100
Finishing thread: A
Thread B is doing work
Finishing thread B
*/
