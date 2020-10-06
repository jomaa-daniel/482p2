#include <vector>
#include <iostream> 
#include <ucontext.h>
#include "thread.h"
#include "cpu.h"
#include <queue>
#include "cpu_impl.h"

using std::cout;

enum status 
{   ready = 0, 
    waiting = 1, 
    done = 2
};

static std::queue<ucontext_t*> ready_queue;

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

class thread::impl{
    public:
    ucontext_t *context_ptr;
    status status = ready;
};


thread::thread(thread_startfunc_t func, void* arg){

    this->impl_ptr->context_ptr = init_context(func, arg);
    
    //TODO
    cpu::interrupt_disable();
    ready_queue.push(this->impl_ptr->context_ptr);    
    //TODO
    cpu::interrupt_enable();

}

void thread::yield(){
    cpu::interrupt_disable();
    //No other ready threads
    if(!ready_queue.empty()){
        //Nothing to run so we continue running this thread.
    }
    else{
        //Move current running thread to waiting list
        cpu::cpu *cpu_ptr = cpu::self();
        ucontext_t *current_context_ptr = cpu_ptr->impl_ptr->running_context;

 
    }
    //Delete done threads

    cpu::interrupt_enable();
}


