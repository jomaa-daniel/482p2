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


void A(intptr_t args)
{
    printf("Running thread: A\n");
    
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    printf("Created threads: \n");

    printf("Thread C joins A\n");

    c.join();

    printf("Input Arg: %i\n",args);

    printf("Finishing thread: A\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) A, (void *) 100, false, false, 0);
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