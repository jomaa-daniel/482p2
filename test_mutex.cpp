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
    m.lock();
    printf("Running thread: %c\n", args);
    m.unlock();
    m.lock();
    printf("Thread %c locked\n", args);
    printf("Thread %c is doing work\n", args);
    m.unlock();
    m.lock();
    printf("Thread %c unlocked\n", args);
    printf("Finishing thread: %c \n", args);
    m.unlock();
}

void parent(intptr_t args)
{
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    m.lock();
    printf("Running thread: A\n");
    printf("Thread A locked\n");
    printf("Thread A yields\n");
    //thread::yield();
    printf("Finishing thread: A\n");
    m.unlock();
}

int main()
{
    cpu::boot(4, (thread_startfunc_t) parent, (void *) 100, true, true, 56);
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
