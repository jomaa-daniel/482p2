#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include "mutex.h"

using std::cout;
using std::endl;

int global = 0;
mutex m = mutex();

void child(char args){
    printf("Running thread: %c on CPU: %i\n", args, cpu::self);
    m.lock();
    printf("Thread %c locked on CPU: %i\n", args, cpu::self);
    printf("Thread %c is doing work on CPU: %i\n", args, cpu::self);
    m.unlock();
    printf("Thread %c unlocked on CPU: %i\n", args, cpu::self);
    printf("Finishing thread: %c on CPU: %i\n", args, cpu::self);
}

void parent(intptr_t args)
{
    printf("Running thread: A\n");
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    thread d = thread((thread_startfunc_t) child, (void*) 'D');

    printf("Created threads: \n");

    m.lock();
    printf("Thread A locked\n");
    
    printf("Thread A yields\n");

    thread::yield();

    printf("Input Arg: %i\n",args);
    
    m.unlock();
    printf("Finishing thread: A\n");
}

int main()
{
    cpu::boot(2, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}

/*
Running thread: A
Created threads:
Thread A locked
Thread A yields
Running thread: B
Running thread: C
Input Arg: 100
Finishing thread: A
Thread B locked
Thread B is doing work
Finishing thread B
Thread C locked
Thread C is doing work
Finishing thread C
*/