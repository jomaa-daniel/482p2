#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "mutex.h"
#include "cv.h"

using std::cout;
using std::endl;

mutex m;
cv child_cv;
cv parent_cv;

int global = 0;
bool go = false;

void child(char args){
    m.lock();
    
    ++global;
    parent_cv.signal();
    printf("Thread %c ticked and signaled\n", args);
    while (go == false)
    {
        printf("Thread %c is waiting for GO\n", args);
        child_cv.wait(m);
    }
    printf("Thread %c Finished\n", args);
    m.unlock();
}

void parent(intptr_t args)
{
    
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    m.lock();
    printf("Running thread: A\n");
    while (global < 2)
    {
        printf("Thread A waiting\n");   
        parent_cv.wait(m);
    }
    go = true;
    child_cv.broadcast();
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
