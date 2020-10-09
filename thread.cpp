#include <vector>
#include <iostream> 
#include <ucontext.h>
#include "thread.h"
#include <queue>
#include <unordered_map>
#include "cpu_impl.h"
#include <vector>
#include "cpu.h"
#include "globals.h"


using std::cout;

static int num_threads = 1000;

void clean_delete_list(){
    for (auto ptr : delete_queue)
    {
        delete_context(ptr);
    }
    delete_queue.clear();
}

void thread_exit(){
    // from user code, interrupts enabled
    // TODO do we need interrupts here?
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    ucontext_t *current_context = cpu::self()->impl_ptr->running_context;
    // TODO INTERRUPTS
    delete_queue.push_back(current_context);

    // check to see if anything is waiting on this to end
    if (waiting_map.find(current_context) != end(waiting_map))
    {
        ready_queue.push(waiting_map[current_context]);
        waiting_map.erase(current_context);
    }
 
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
    else
    {
        sleeping_queue.push(cpu::self());
        cpu::guard = false;
        cpu::interrupt_enable_suspend();
    }

}

void stub(thread_startfunc_t func, void *arg){

    cpu::interrupt_enable();
    cpu::guard = false;
    //Not sure if this is correct
    func(arg);


    //cleanup
    thread_exit();
}

class thread::impl{
    public:
    ucontext_t *context_ptr;
    int thread_id;
    impl();
    ~impl();
};
thread::impl::impl()
{
    this->context_ptr = nullptr;
    this->thread_id = num_threads++;
}

thread::impl::~impl()
{
    this->context_ptr = nullptr;
}


thread::thread(thread_startfunc_t func, void* arg){
    
    this->impl_ptr = new impl();

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}

    this->impl_ptr->context_ptr = init_context(func, arg);
    
    ready_queue.push(this->impl_ptr->context_ptr);    

    if(!sleeping_queue.empty()){
        cpu * front = sleeping_queue.front();
        sleeping_queue.pop();
        front->interrupt_send();
    }

    cpu::guard = false;
    cpu::interrupt_enable();

}

thread::~thread(){
   //printf("dealloc thread: %i\n",thread::impl_ptr->thread_id);
    // assert this context is somewhere else
    delete impl_ptr;
}

void thread::yield(){
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    //No other ready threads
    if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();
        // context switch
        // done
        // disable
        ucontext_t *next_context = ready_queue.front();
        ready_queue.pop();
        ucontext_t *this_context = cpu_ptr->impl_ptr->running_context;
        // TODO make sure that there are interrupts while this is going
        ready_queue.push(this_context);
        cpu_ptr->impl_ptr->running_context = next_context;
        swapcontext(this_context, next_context); // INVARIANT: interrupts disabled
        assert_interrupts_disabled();
    }
    
    //Delete done threads
    //printf("Del %i threads in yeild\n", delete_queue.size());
    clean_delete_list();

    cpu::guard = false;
    cpu::interrupt_enable();
}

void thread::join(){
// do NOT run the original thread when this is done, threads need 
  // to be run in FIFO order. just put the caller thread on the ready Q

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    waiting_map[this->impl_ptr->context_ptr] = this_context;

    if (ready_queue.empty())
    {
        sleeping_queue.push(cpu::self());
        cpu::guard = false;
        cpu::interrupt_enable_suspend();
    }

    ucontext_t *next_context = ready_queue.front(); 
    ready_queue.pop();
    
    // INTERRUPT NEEDED
    // do we care if it gets interrupted here?
    
    cpu::self()->impl_ptr->running_context = next_context;
    
    swapcontext(this_context, next_context);

    //Delete done threads
    //printf("Del %i threads in join\n", delete_queue.size());
    clean_delete_list();
    
    cpu::guard = false;
    cpu::interrupt_enable();
}
