#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include "mutex.h"

using std::cout;
using std::endl;

int global = 0;
mutex m = mutex();

void child(char args){
    m.lock();
    printf("Running thread: %c\n", args);
    m.unlock();
}

void parent(intptr_t args)
{
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    thread c = thread((thread_startfunc_t) child, (void*) 'C');
    m.lock();
    printf("Running thread A \n", args);
    thread::yield();
    m.unlock();

}

int main() 
{
    cpu::boot(8, (thread_startfunc_t) parent, (void *) 100, true, true, 22);
}
