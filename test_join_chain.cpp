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


void childB(thread* args)
{
    args->join();
   m.lock();
   while (global != 3)
   {
       B.wait(m);
   }
   ++global;
   B.broadcast();
   m.unlock();
}

void childC(thread* args)
{   
   m.lock();
   while (global != 2)
   {
       B.wait(m);
   }
   ++global;
   B.broadcast();
   m.unlock();
   args->join();
}

void childD(thread* args)
{   
     m.lock();
   while (global != 1)
   {
       B.wait(m);
   }
   ++global;
   B.broadcast();
   m.unlock();
   args->join();
}

void childE(thread* args)
{   
     m.lock();
   while (global != 0)
   {
       B.wait(m);
   }
   ++global;
   B.broadcast();
   m.unlock();
   args->join();
}

void parent(intptr_t args)
{
    
    thread b = thread((thread_startfunc_t) childB, (void*) &b); // sleep
    thread c = thread((thread_startfunc_t) childC, (void*) &b); // run
    thread d = thread((thread_startfunc_t) childD, (void*) &c);
    thread e = thread((thread_startfunc_t) childE, (void*) &d);

    b.join();
    printf("Finishing thread A\n");
    c.join();
    d.join();
    e.join();
    

}

int main()
{
    cpu::boot(7, (thread_startfunc_t) parent, (void *) 100, true, true, 0);
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
