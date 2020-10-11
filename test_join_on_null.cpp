#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 


using std::cout;
using std::endl;

int global = 0;

void child(char args){
    printf("Running thread: %c\n", args);
    printf("Thread %c is doing work\n", args);
    printf("Finishing thread: %c \n", args);
}

void child2(char args){
    printf("Running thread: %c\n", args);
    printf("Thread %c is doing work\n", args);
    thread::yield();
    printf("Finishing thread: %c \n", args);
}


void A(intptr_t args)
{
    printf("Running thread: A\n");
    
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child2, (void*) 'C');
    
    printf("Created threads: \n");
    thread::yield();
    printf("Thread A joined thread B\n");
    b.join();
    printf("Finishing thread: A\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) A, (void *) 100, true, true, 6);
    printf("DONE!");
}
/*
Running thread: A
Created threads:
Thread C joins A
Running thread: B
Thread B is doing work
Finishing thread B
Running thread: C
Thread C is doing work
Finishing thread C
Input Arg: 100
Finishing thread: A
*/