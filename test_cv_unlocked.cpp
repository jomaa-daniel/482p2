#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility> 
#include "mutex.h"
#include "cv.h"
using std::cout;
using std::endl;

int global = 1;
mutex m = mutex();
mutex g = mutex();
cv condition_variable = cv();

void child(char args){

}

void parent(intptr_t args)
{
    thread b = thread((thread_startfunc_t) child, (void*) 'B');
    

    try{
        
        condition_variable.wait(g);
        exit(1);
    }
    catch(...){
    }

    printf("A done\n");
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, true, true, 6);
}

