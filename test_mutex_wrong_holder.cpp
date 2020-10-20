#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 


using std::cout;
using std::endl;
mutex m;

void child(char args){
    m.lock();
}


void A(intptr_t args)
{
    printf("Running thread: A\n");
    
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    printf("Created threads: \n");
    thread::yield();
    try{
        m.unlock();
        exit(1);
    }
    catch(...){
    }

    printf("Input Arg: %i\n",args);

    printf("Finishing thread: A\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}
