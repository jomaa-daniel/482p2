#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout;
using std::endl;

int global = 0;

void B(intptr_t args){
    printf("Running thread: B\n");
    printf("Input Arg: %i\n",++args);
    printf("Finishing thread: B\n");
}

void A(intptr_t args)
{
    printf("Running thread: A\n");
    
    thread b = thread((thread_startfunc_t) B, (void*)args);
    printf("Created thread: B\n");

    printf("Thread A yields to thread B\n");

    thread::yield();

    printf("Input Arg: %i\n",args);

    printf("Finishing thread: A\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}