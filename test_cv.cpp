#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include "mutex.h"
#include "cv.h"
using std::cout;
using std::endl;

int global = 0;
mutex m = mutex();
cv condition_variable = cv();

void child(char args){
    printf("Running thread: %c\n", args);
    m.lock();
    printf("Thread %c locked\n", args);
    printf("Thread %c is doing work\n", args);
    global++;
    printf("Thread %c incremented\n", args);
    m.unlock();
    printf("Thread %c unlocked\n", args);
    condition_variable.signal();
    printf("Thread %c signaled\n", args);
    printf("Finishing thread: %c \n", args); 

}


void parent(intptr_t args)
{
    printf("Running thread: A\n");
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    printf("Created threads: \n");

    m.lock();
    printf("Thread A locked\n");
    
    while(global < 2 ){
        printf("Thread A waits on %i\n", global);
        condition_variable.wait(m);
    }

    printf("Running thread: A\n");

    m.unlock();
    printf("Finishing thread: A\n");
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, true, true, 6);
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