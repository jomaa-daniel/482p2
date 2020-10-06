#include <vector>
#include <iostream> 
#include <ucontext.h>
#include "thread.h"
#include "cpu.h"
#include <queue>
#include <unordered_set>
#include "cpu_impl.h"

using std::cout;

enum status 
{   ready = 0, 
    waiting = 1, 
    done = 2
};

static std::queue<ucontext_t*> ready_queue;
static std::vector<ucontext_t*> delete_queue; // TODO when we move something to the 
                                            // delete queue we need to check to see if something is waiting on it
static std::unordered_set<ucontext_t*> join_queue;

class thread::impl{
    public:
    ucontext_t *context_ptr;
    status status = ready;
};


thread::thread(thread_startfunc_t func, void* arg){

    this->impl_ptr->context_ptr = init_context(func, arg);
    
    //TODO
    //cpu::interrupt_disable();
    ready_queue.push(this->impl_ptr->context_ptr);    
    //TODO
    //cpu::interrupt_enable();

}

thread::~thread(){

}

void thread::yield(){
    //cpu::interrupt_disable();
    //No other ready threads
    if(!ready_queue.empty()){
        cpu::cpu *cpu_ptr = cpu::self();
        // context switch
        // done
        // disable
        ucontext_t * next_context = ready_queue.front();
        ready_queue.pop();
        ucontext_t* this_context = cpu_ptr->impl_ptr->running_context;
        // TODO make sure that there are interrupts while this is going
        ready_queue.push(this_context);
        cpu_ptr->impl_ptr->running_context = next_context;
        swapcontext(this_context, next_context);
    }
    
    //Delete done threads
    for (auto ptr : delete_queue)
    {
        delete_context(ptr);
    }

    //cpu::interrupt_enable();
}

thread::join(){
// do NOT run the original thread when this is done, threads need 
  // to be run in FIFO order. just put the caller thread on the ready Q
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
    // TODO interrupts needed - shared data
    join_queue.insert(this_context); // question: dont we need to use get context to 
    // get a snapshot of the current context?

    
    

}


