#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include <cassert>

using std::cout;
using std::endl;
mutex m;
mutex g;
int global = 0;
cv cond;


void child(char args){
    m.lock();
    assert(false);
    printf("Got it\n");
    m.unlock();
}

void childb(char args){
    m.lock();
    global++;
    cond.signal();
}

void A(intptr_t args)
{

    printf("Running parent: A\n");
    thread((thread_startfunc_t) childb, (void*) 'B');
    try{
        g.lock();
        while(global == 0){
            cond.wait(g);
        }

        for(int i = 0; i<100;++i){
            for(int j =0; j<100;++j){
                global++;
                thread((thread_startfunc_t) child, (void*) 'B');
            }
        }
    }
    catch(...){

    }
    printf("Finishing parent: A\n");

}

int main()
{
    cpu::boot(2, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}