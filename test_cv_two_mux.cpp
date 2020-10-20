#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include "mutex.h"
#include "cv.h"
using std::cout;
using std::endl;

int i = 0;
mutex m = mutex();
mutex g = mutex();
cv condition_variable = cv();
cv cond2;
int flip = 0;
void child(int args){
    m.lock();
    while (i < 100)
    {
      condition_variable.wait(m);
    }
    ++flip;
    condition_variable.signal();
    printf("child %i\n", args);
    m.unlock();
}

void parent(intptr_t args)
{
    
    for (; i < 100; ++i)
    {
        thread((thread_startfunc_t) child, (void*) i);
    }

    g.lock();
    while (flip < 100)
    {
        condition_variable.wait(g);
    }
    g.unlock();

    condition_variable.broadcast();
    printf("Parent Done\n");

}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, true, true, 6);
}

