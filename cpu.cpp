#include <vector>
#include "cpu.h"
#include <iostream> 
#include <ucontext.h>
#include <queue>
using std::cout;


void timer_handler()
{
    cpu::interrupt_disable();

    cpu::interrupt_enable();    
}

void ipi_handler()
{

}


class impl
{
    static std::queue<ucontext_t*> ready_queue;
    
};

void cpu::init(thread_startfunc_t func, void *)
{

}