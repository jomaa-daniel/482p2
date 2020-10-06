#include <vector>
#include "cpu.h"
#include <iostream> 
#include <ucontext.h>
#include "cpu_impl.h"
using std::cout;


void timer_handler()
{
    cpu::interrupt_disable();
    cout << "Timer\n";
    cpu::interrupt_enable();    
}

void ipi_handler()
{
    cpu::interrupt_disable();
    cout << "ipi\n";
    cpu::interrupt_enable();
}



// ====================== CPU IMPL IMPLEMENTATION ==================
cpu::impl::~impl()
{
    delete_context(this->running_context);
}

ucontext_t* init_context(thread_startfunc_t func, void *arg){
    ucontext_t* ucp = new ucontext_t;
    getcontext(ucp); // initialize ucontext, as required by interface
    ucp->uc_link = nullptr;
    if (func) {
        char* stack = new char[STACK_SIZE];
        ucp->uc_stack.ss_sp = stack;
        ucp->uc_stack.ss_size = STACK_SIZE;
        ucp->uc_stack.ss_flags = 0;
        makecontext(ucp, (void (*)()) func, 1, arg);
    }
    return ucp;
}

void delete_context(ucontext_t *ucp){
    delete[] ucp->uc_stack.ss_sp;
    delete ucp; 
    ucp = nullptr;
}

// ====================== CPU IMPLEMENTATION ========================

void cpu::init(thread_startfunc_t func, void * arg)
{
    cpu::interrupt_vector_table[TIMER] = timer_handler;
    cpu::interrupt_vector_table[IPI] = ipi_handler;
    this->impl_ptr = new impl();
    ucontext_t *thread_cont;

    thread_cont = init_context(func, arg);
    this->impl_ptr->running_context = thread_cont;
    setcontext(thread_cont);
        
}