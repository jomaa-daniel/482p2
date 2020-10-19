#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility>


using std::cout;
using std::endl;
mutex m;


void A(intptr_t args)
{

  m.lock();
  m.lock();

}

int main()
{
  cpu::boot(1, (thread_startfunc_t) A, (void *) 100, true, true, 0);
}

