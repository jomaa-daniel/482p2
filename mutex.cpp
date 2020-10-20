#include "mutex_impl.h"
#include "mutex.h"
#include "cpu.h"
#include "globals.h"
#include "cpu_impl.h"
#include <cassert>
#include <stdexcept>

mutex::mutex(){
    try
    {
        this->impl_ptr = new impl();
    }
    catch(const std::exception &e)
    {
        // Throw and prevent user code from running with invariants
        std::exception_ptr p = std::current_exception();
        cpu::guard.store(false);
        cpu::interrupt_enable();    
        std::rethrow_exception(p);
    }
}

mutex::~mutex(){
    delete this->impl_ptr;
}

void mutex::lock(){
   cpu::interrupt_disable();
   while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());

    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;

    // if the lock is busy right now...
    if(this->impl_ptr->busy){

        assert(context_count(this_context) == 0);
        
        this->impl_ptr->mutex_waiting_queue.push(this_context);
        // Swap with next ready context on the ready_queue
        if(!ready_queue.empty()){
            ucontext_t* next_context = ready_queue.front();
            assert(context_count(next_context) == 1);
            ready_queue.pop();
            cpu::self()->impl_ptr->running_context = next_context;
            swapcontext(this_context, next_context);            
        }
        else{ 
            // go to sleep
            swapcontext(this_context, cpu::self()->impl_ptr->sleep_context);
        }
    }
    else{
        this->impl_ptr->busy = true;
        this->impl_ptr->holder = cpu::self()->impl_ptr->running_context;
    }

    clean_delete_list();
    cpu::guard.store(false);
    cpu::interrupt_enable();
}

void mutex::unlock()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());

    // if no context is holding the lock, runtime error
    if (!this->impl_ptr->busy) {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
        } 

    // check to see if the correct context is holding the lock
    if(this->impl_ptr->holder!=cpu::self()->impl_ptr->running_context) {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
        }

    
    this->impl_ptr->busy = false;
    this->impl_ptr->holder = nullptr;

    // if someone is waiting for the lock, put it on the ready queue 
    // give it the lock and wake a sleeping cpu if there is one
    if (!this->impl_ptr->mutex_waiting_queue.empty())
    {
        ucontext_t* next_context = this->impl_ptr->mutex_waiting_queue.front();
        this->impl_ptr->mutex_waiting_queue.pop();
        ready_queue.push(next_context);
        assert(context_count(next_context) == 1);
        this->impl_ptr->busy = true;
        this->impl_ptr->holder = next_context;
        wake_cpu();
    }

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


