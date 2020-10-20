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
mutex g = mutex();
cv condition_variable = cv();

void child(char args){

    m.lock();
    condition_variable.wait(m);
    m.unlock();

}


void parent(intptr_t args)
{
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    while(false){
        condition_variable.wait(g);
    }
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