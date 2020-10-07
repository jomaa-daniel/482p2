#include <vector>
#include <iostream> 
#include <ucontext.h>
#include "thread.h"
#include <queue>
#include <unordered_map>
#include "cpu_impl.h"
#include <vector>
#include "cpu.h"


using std::cout;


static std::queue<ucontext_t*> ready_queue;
static std::vector<ucontext_t*> delete_queue; // TODO when we move something to the 
                                            // delete queue we need to check to see if something is waiting on it
static std::unordered_map<ucontext_t*, ucontext_t*> waiting_queue; // A
static std::queue<cpu*> sleeping_queue;

void thread_exit(){
    // TODO do we need interrupts here?
    ucontext_t *current_context = cpu::self()->impl_ptr->running_context;
    // TODO INTERRUPTS
    delete_queue.push_back(current_context);

    // check to see if anything is waiting on this to end
    if (waiting_queue.find(current_context) != end(waiting_queue))
    {
        ready_queue.push(waiting_queue[current_context]);
        waiting_queue.erase(current_context);
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
    // INVARIANT: interrupts disabled
    //SLEEP!
    sleeping_queue.push(cpu::self());
    cpu::interrupt_enable_suspend();
    //enable interrupts again
}

void stub(thread_startfunc_t func, void *arg){
    //Not sure if this is correct
    func(arg);

    //Cleanup thread
    thread_exit();
}


class thread::impl{
    public:
    ucontext_t *context_ptr;
    impl();
    ~impl();
};

thread::impl::impl()
{
    this->context_ptr = nullptr;
}

thread::impl::~impl()
{
    this->context_ptr = nullptr;
}


thread::thread(thread_startfunc_t func, void* arg){
    this->impl_ptr = new impl();
    
    this->impl_ptr->context_ptr = init_context(func, arg);
    
    //TODO
    //cpu::interrupt_disable();
    ready_queue.push(this->impl_ptr->context_ptr);    

    if(!sleeping_queue.empty()){
        cpu * front = sleeping_queue.front();
        sleeping_queue.pop();
        front->interrupt_send();
    }

    //TODO
    //cpu::interrupt_enable();

}

thread::~thread(){
    delete impl_ptr;
}

void thread::yield(){
    //cpu::interrupt_disable();
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
    for (auto ptr : delete_queue)
    {
        delete_context(ptr);
    }

    //cpu::interrupt_enable();
}

void thread::join(){
// do NOT run the original thread when this is done, threads need 
  // to be run in FIFO order. just put the caller thread on the ready Q

  // INTERRUPTS NEEDED
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    waiting_queue[this->impl_ptr->context_ptr] = this_context;

    if (ready_queue.empty())
    {
        sleeping_queue.push(cpu::self());
        cpu::interrupt_enable_suspend();
    }

    ucontext_t *next_context = ready_queue.front(); 
    ready_queue.pop();
    
    // INTERRUPT NEEDED
    // do we care if it gets interrupted here?
    

    swapcontext(this_context, next_context);
    
}
