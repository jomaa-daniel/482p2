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
    assert(m.impl_ptr->busy);

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    //Unlock
    m.impl_ptr->busy = false;
    if (!m.impl_ptr->mutex_waiting_queue.empty())
    {
        ucontext_t* next_context = m.impl_ptr->mutex_waiting_queue.front();
        m.impl_ptr->mutex_waiting_queue.pop();
        ready_queue.push(next_context);
        m.impl_ptr->busy = true;
    }
    //Unlock
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    this->impl_ptr->cv_signal_queue.push(this_context);

    if(!ready_queue.empty()){
        ucontext_t* next_context = ready_queue.front(); 
        ready_queue.pop();
        cpu::self()->impl_ptr->running_context = next_context;\
        swapcontext(this_context, next_context);            
    }
    else{       
        sleeping_queue.push(cpu::self());
        cpu::guard = false;
        cpu::interrupt_enable_suspend();
    }

    cpu::guard = false;
    cpu::interrupt_enable();
    m.lock();
}

void cv::signal()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}

    if(!this->impl_ptr->cv_signal_queue.empty()){
        ucontext_t *con = this->impl_ptr->cv_signal_queue.front();
        this->impl_ptr->cv_signal_queue.pop();
        ready_queue.push(con);
    }   

    cpu::guard = false;
    cpu::interrupt_enable();   
}

void cv::broadcast()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    
    while(!this->impl_ptr->cv_signal_queue.empty()){
        ucontext_t *con = this->impl_ptr->cv_signal_queue.front();
        this->impl_ptr->cv_signal_queue.pop();
        ready_queue.push(con);
    }   

    cpu::guard = false;
    cpu::interrupt_enable();   
}

