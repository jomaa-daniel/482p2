#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout;
using std::endl;

int global = 0;

void print_args(intptr_t args)
{
    cout << "ENTER FUNC \n";
    cout << "ARG: " << args << endl;

    global += args;

    cout << "GLOBAL: " << global << endl;
    
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}