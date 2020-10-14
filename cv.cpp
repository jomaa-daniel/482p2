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
    impl_ptr = new impl();
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
    //Unlock
    //printf("Calling cv wait unlock\n");
    if (!m.impl_ptr->busy) 
    {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
    } 
    if(m.impl_ptr->holder!=cpu::self()->impl_ptr->running_context)
    {
        cpu::guard.store(false);
        cpu::interrupt_enable();
        throw std::runtime_error("");
    }
    m.impl_ptr->busy = false;
    m.impl_ptr->holder = nullptr;
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
    //Unlock
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    this->impl_ptr->cv_signal_queue.push(this_context);

    if(!ready_queue.empty()){
        ucontext_t* next_context = ready_queue.front(); 
        assert(context_count(next_context) == 1);
        ready_queue.pop();
        cpu::self()->impl_ptr->running_context = next_context;
        swapcontext(this_context, next_context);            
    }
    else{       
        swapcontext(this_context, cpu::self()->impl_ptr->sleep_context);
    }

    //printf("Calling cv wait lock\n");
    cpu::guard.store(false);
    cpu::interrupt_enable();
    m.lock();
}

void cv::signal()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());
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

