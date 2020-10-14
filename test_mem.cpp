#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 


using std::cout;
using std::endl;
mutex m;
int global = 0;
cv cond;


void child(char args){
    m.lock();
    global--;
    m.unlock();
}


void A(intptr_t args)
{
    printf("Running parent: A\n");
    for(int i = 0; i<10000;++i){
        m.lock();
        for(int j =0; j<10000;++j){
            global++;
            thread((thread_startfunc_t) child, (void*) 'B');
        }
        while(global >0){
            cond.wait(m);
        }
        m.unlock();

    }

    printf("Finishing parent: A\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}