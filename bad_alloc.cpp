#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <utility>
#include <cassert>

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
  try{
    m.lock();
    printf("Running parent: A\n");
    for(int i = 0; i<10000;++i){
      for(int j =0; j<10000;++j){
        thread((thread_startfunc_t) child, (void*) 'B');
      }
    }
    m.unlock();
    printf("Finishing parent: A\n");
  }
  catch(std::bad_alloc &e){
      assert_interrupts_enabled();
    assert(cpu::guard.is_lock_free());
    printf("DONE!\n");
  }
}

int main()
{
  cpu::boot(1, (thread_startfunc_t) A, (void *) 100, false, false, 0);
}
