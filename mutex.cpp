#include "mutex_impl.h"
#include "mutex.h"
#include "cpu.h"
#include "globals.h"
#include "cpu_impl.h"

mutex::mutex(){
    this->impl_ptr = new impl();
}

mutex::~mutex(){
    delete this->impl_ptr;
}


void mutex::lock(){
   cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    if(this->impl_ptr->busy){
        ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
        this->impl_ptr->mutex_waiting_queue.push(this_context);

        if(!ready_queue.empty()){
            ucontext_t* next_context = ready_queue.front(); 
            ready_queue.pop();
            cpu::self()->impl_ptr->running_context = next_context;
            swapcontext(this_context, next_context);            
        }
        else{       
            sleeping_queue.push(cpu::self());
            cpu::guard = false;
            cpu::interrupt_enable_suspend();
        }
    }
    else{
        this->impl_ptr->busy = true;
    }

    //Delete done threads
    clean_delete_list();

    cpu::guard = false;
    cpu::interrupt_enable();
}

void mutex::unlock()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    this->impl_ptr->busy = false;
    if (!this->impl_ptr->mutex_waiting_queue.empty())
    {
        ucontext_t* next_context = this->impl_ptr->mutex_waiting_queue.front();
        this->impl_ptr->mutex_waiting_queue.pop();
        ready_queue.push(next_context);
        this->impl_ptr->busy = true;
    }

    cpu::guard = false;
    cpu::interrupt_enable();
}

// ====================== mutex IMPL ==========
mutex::impl::impl()
{
    this->busy = false;
}

mutex::impl::~impl()
{
    
}


