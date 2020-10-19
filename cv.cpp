#include "globals.h"
#include "cv_impl.h"
#include "cpu.h"
#include <cassert>
#include "mutex.h"
#include "mutex_impl.h"
#include "cpu_impl.h"
// ================= CV IMPL Implementation ===============
cv::impl::impl()
{

}

cv::impl::~impl()
{

}
// ================== CV Implementation =====================

cv::cv(){
    try
    {
        impl_ptr = new impl();
    }
    catch(const std::exception &e)
    {
        //Throw and prevent user code from running with invariants
        std::exception_ptr p = std::current_exception();
        cpu::guard.store(false);
        cpu::interrupt_enable();    
        std::rethrow_exception(p);
    }
    
}

cv::~cv()
{
    delete impl_ptr;
}

void cv::wait(mutex &m)
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    assert(cpus_run_different_contexts());
    // Implicit Unlock to avoid double disabling interrupts

    // if the lock is not held throw runtime
    if (!m.impl_ptr->busy) 
    {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
    } 

    // if this is not the correct holder, throw
    if(m.impl_ptr->holder!=cpu::self()->impl_ptr->running_context)
    {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
    }
    m.impl_ptr->busy = false;
    m.impl_ptr->holder = nullptr;
    // if someone is in the waiting queue wake them up on the next available cpu
    if (!m.impl_ptr->mutex_waiting_queue.empty())
    {
        ucontext_t* next_context = m.impl_ptr->mutex_waiting_queue.front();
        m.impl_ptr->mutex_waiting_queue.pop();
        ready_queue.push(next_context);
        assert(context_count(next_context) == 1);
        wake_cpu();
        m.impl_ptr->busy = true;
        m.impl_ptr->holder = next_context;
    }

    // Implicit Unlock to avoid double disabling interrupts

    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    this->impl_ptr->cv_signal_queue.push(this_context);

    // if there is someone in the ready queue, run them next
    if(!ready_queue.empty()){
        ucontext_t* next_context = ready_queue.front(); 
        assert(context_count(next_context) == 1);
        ready_queue.pop();
        cpu::self()->impl_ptr->running_context = next_context;
        swapcontext(this_context, next_context);            
    }
    else{       // else go to sleep
        swapcontext(this_context, cpu::self()->impl_ptr->sleep_context);
    }

    cpu::guard.store(false);
    cpu::interrupt_enable();
    m.lock();
}

void cv::signal()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());
    // if no one is on the signal queue just skip
    if(!this->impl_ptr->cv_signal_queue.empty()){
        ucontext_t *con = this->impl_ptr->cv_signal_queue.front();
        this->impl_ptr->cv_signal_queue.pop();
        ready_queue.push(con);
        assert(context_count(con) == 1);
        wake_cpu();
    }   

    cpu::guard.store(false);
    cpu::interrupt_enable();   
}

void cv::broadcast()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}

    assert(cpus_run_different_contexts());
    // wake everyone on the signal queue
    while(!this->impl_ptr->cv_signal_queue.empty()){
        ucontext_t *con = this->impl_ptr->cv_signal_queue.front();
        this->impl_ptr->cv_signal_queue.pop();
        ready_queue.push(con);
        assert(context_count(con) == 1);
        wake_cpu();
    }   

    cpu::guard.store(false);
    cpu::interrupt_enable();   
}

