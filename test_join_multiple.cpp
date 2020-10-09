#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "thread_impl.h"
#include <utility> 


using std::cout;
using std::endl;


int global = 0;


void child(thread* args){
    printf("Running thread: %c\n", args->impl_ptr->thread_id);
    if(args->impl_ptr->thread_id == 0){
        printf("Thread C joins thread B");
        args.join();
    }
    printf("Thread %c is doing work\n", args->impl_ptr->thread_id);
    printf("Finishing thread: %c \n", args->impl_ptr->thread_id);
}


void A(intptr_t args)
{
    printf("Running thread: A\n");
    
    thread b = thread((thread_startfunc_t) child, (void*) &b);
    thread c = thread((thread_startfunc_t) child, (void*) &c);
    printf("Created threads: \n");

    printf("Thread C joins A\n");

    c.join();

    printf("Thread A running\n");

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
Thread C joins thread B
Running thread: C
Thread C is doing work
Finishing thread C
Input Arg: 100
Finishing thread: A
Thread B is doing work
Finishing thread B
*/
