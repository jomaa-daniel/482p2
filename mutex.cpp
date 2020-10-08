#include "mutex_impl.h"
#include "mutex.h"
#include "cpu.h"
#include "thread.cpp"


mutex::mutex(){
    this->impl_ptr = new impl::impl();
}

mutex::~mutex(){
    delete this->impl_ptr;
}


void mutex::lock(){
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    if(this->impl_ptr->busy){
        this->impl_ptr->
    }
}

void mutex::unlock()
{
    cpu::interrupt_disable();

    while (cpu::guard.exchange(true))

    if (this->impl_ptr->busy)
    {
        if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();
        // context switch
        // done
        // disable
        ucontext_t * next_context = ready_queue.front();
        ready_queue.pop();
        // TODO make sure that there are interrupts while this is going
        cpu_ptr->impl_ptr->running_context = next_context;
        setcontext(next_context); // INVARIANT: interrupts disabled
        assert_interrupts_disabled();
    }
        // INVARIANT: interrupts disabled
        //SLEEP!
        sleeping_queue.push(cpu::self());
    }
}

// ====================== mutex IMPL ==========
mutex::impl::impl()
{
    this->busy = false;
}

mutex::impl::~impl()
{
    
}