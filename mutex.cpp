#include "mutex_impl.h"
#include "mutex.h"
#include "cpu.h"
#include "globals.h"
#include "cpu_impl.h"
#include <cassert>
#include <stdexcept>

mutex::mutex(){
    this->impl_ptr = new impl();
}

mutex::~mutex(){
    delete this->impl_ptr;
}

void mutex::lock(){
   cpu::interrupt_disable();
   while (cpu::guard.exchange(true)) {}
   //print_state(this);
    assert(cpus_run_different_contexts());

    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;

    if(this->impl_ptr->busy){

        assert(context_count(this_context) == 0);
        this->impl_ptr->mutex_waiting_queue.push(this_context);

        if(!ready_queue.empty()){
            ucontext_t* next_context = ready_queue.front();
            assert(context_count(next_context) == 1);
            ready_queue.pop();
            cpu::self()->impl_ptr->running_context = next_context;
            swapcontext(this_context, next_context);            
        }
        else{ 
            sleeping_queue.push(cpu::self());
            //cpu::self()->impl_ptr->running_context = nullptr;
            cpu::guard.store(false);
            cpu::interrupt_enable_suspend();
        }
    }
    else{
        this->impl_ptr->busy = true;
        this->impl_ptr->holder = cpu::self()->impl_ptr->running_context;
    }
    //print_state(this);
    //Delete done threads
    clean_delete_list();
    cpu::guard.store(false);
    cpu::interrupt_enable();
}

void mutex::unlock()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());
    if (!this->impl_ptr->busy) {throw std::runtime_error("Unlock on unlocked mutex\n");} 
    if(this->impl_ptr->holder!=cpu::self()->impl_ptr->running_context) {throw std::runtime_error("Wrong holder\n");}
    this->impl_ptr->busy = false;
    this->impl_ptr->holder = nullptr;
    if (!this->impl_ptr->mutex_waiting_queue.empty())
    {
        ucontext_t* next_context = this->impl_ptr->mutex_waiting_queue.front();
        this->impl_ptr->mutex_waiting_queue.pop();
        ready_queue.push(next_context);
        assert(context_count(next_context) == 1);
        this->impl_ptr->busy = true;
        this->impl_ptr->holder = next_context;
        //printf("wake up guy on sleeping Q\n");
        //print_state(this);
        wake_cpu();
        //print_state(this);
    }
    //print_state(this);
    cpu::guard.store(false);
    cpu::interrupt_enable();
}

// ============== mutex IMPL ==========
mutex::impl::impl()
{
    this->busy = false;
    this->holder = nullptr;
}

mutex::impl::~impl()
{
    
}


