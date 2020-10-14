#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "thread_impl.h"
#include <utility> 
#include <vector>



using std::cout;
using std::endl;

cv cv_waiting;
mutex m;
int global = 0;
int waiting = 0;
cv B, C, D;


void child(thread* args)
{
    m.lock();
    while (global < 3)
    {
        B.wait(m);
    }
    m.unlock();
    printf("B ran\n");
}

void childC(thread* args)
{   
    m.lock();
    ++global;
    B.signal();
    m.unlock();
    args->join();    

}


void parent(intptr_t args)
{
    
    thread b = thread((thread_startfunc_t) child, (void*) &b); // sleep
    thread c = thread((thread_startfunc_t) childC, (void*) &b); // run
    thread d = thread((thread_startfunc_t) childC, (void*) &b);
    thread e = thread((thread_startfunc_t) childC, (void*) &b);


    b.join();
    c.join();
    d.join();
    printf("Finishing thread A\n");
    

}

int main()
{
    cpu::boot(4, (thread_startfunc_t) parent, (void *) 100, true, true, 0);
}

//Joining chain A->B->C->D
//Joining on self (deadlock)
//multiple threads joining 1 A->B C->B D->B
// 1 thread joining multiple A->B ,A->C, A->D

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
