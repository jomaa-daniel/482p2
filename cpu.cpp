#include <vector>
#include "cpu.h"
#include <iostream> 
#include <ucontext.h>
#include "cpu_impl.h"
#include <memory>
#include "globals.h"
#include <cstring>
#include <cassert>
using std::cout;

//Ready_queue for ready contexts to be pushed when they are ready to run 
std::queue<ucontext_t*> ready_queue;
//Delete_queue to push contexts on such that the 
//next context that runs deletes them
std::vector<ucontext_t*> delete_queue; 
//Waiting_map for joined contexts waiting on another context
std::unordered_map<ucontext_t*, std::queue<ucontext_t*>> waiting_map; 
//Sleeping_queue for cpus that are suspended.
std::queue<cpu*> sleeping_queue;

/*
 The timer handler will trigger every 1ms. 
 If there are no threads on the ready queue, ignore the interrupt,
 else the handlder will switch to the next ready thread

*/

void timer_handler()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    assert(cpus_run_different_contexts());
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context; 

    // make sure that this context is not the sleep context
    // if there is nothing in the ready queue, just skip
    if (!ready_queue.empty() && 
    this_context != cpu::self()->impl_ptr->sleep_context){               
        ucontext_t* next_context = ready_queue.front();
        ready_queue.pop(); 
        cpu::self()->impl_ptr->running_context = next_context;
        ready_queue.push(this_context);
       // wake_cpu();
        swapcontext(this_context, next_context);
    }
    cpu::guard.store(false);
    cpu::interrupt_enable();    
}

void ipi_handler()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    // if there is no other thread to switch to, just skip
    if(!ready_queue.empty())
    {
        cpu *cpu_ptr = cpu::self();
        ucontext_t *next_context = ready_queue.front();
        ready_queue.pop();
        cpu_ptr->impl_ptr->running_context = next_context;
        swapcontext(cpu::self()->impl_ptr->sleep_context, next_context); 
    }
}


void sleeper_func(){
    //Always rerun since we will always need a sleeping context for each CPU
    while (true){
        sleeping_queue.push(cpu::self());
        cpu::self()->impl_ptr->running_context = cpu::self()->impl_ptr->sleep_context;
        cpu::guard.store(false);
        cpu::interrupt_enable_suspend();
        
    }
    
}


// ====================== CPU IMPL IMPLEMENTATION ==================

cpu::impl::~impl()
{
    delete_context(this->running_context);
}

cpu::impl::impl()
{
    this->running_context = nullptr;
}

// ====================== CPU IMPL IMPLEMENTATION ==================

//Map for each context to find the base of its stack pointer
std::unordered_map<ucontext_t*, char*> base_pointer_map;

//Create and initialize contexts
ucontext_t* init_context(thread_startfunc_t func, void *arg){
    ucontext_t* ucp = new ucontext_t;
    getcontext(ucp); // initialize ucontext, as required by interface
    ucp->uc_link = nullptr;
    if (func) {
        char* stack = new char[STACK_SIZE];
        base_pointer_map[ucp] = stack;
        ucp->uc_stack.ss_sp = stack;
        ucp->uc_stack.ss_size = STACK_SIZE;
        ucp->uc_stack.ss_flags = 0;
        makecontext(ucp, (void (*)()) stub, 2, func, arg);
    }
    return ucp;
}

void delete_context(ucontext_t *ucp){
    assert(ucp != nullptr);
    assert(base_pointer_map[ucp] != nullptr);
    delete[] (char*) base_pointer_map[ucp];
    base_pointer_map.erase(ucp);
    delete ucp; 
    ucp = nullptr;
    
}


// ====================== CPU IMPLEMENTATION ========================
void cpu::init(thread_startfunc_t func, void * arg)
{
    // sAfety
    while (cpu::guard.exchange(true)){}

    cpu::interrupt_vector_table[TIMER] = timer_handler;
    cpu::interrupt_vector_table[IPI] = ipi_handler;
    ucontext_t *sleep_context = nullptr;
    cpu::impl *imp = nullptr;
    try{
        imp = new cpu::impl();
        
        //Add CPU to list of CPUs        
        running_cpus.push_back(cpu::self());
            
        //Sleeper context init
        //Create special sleeper context without stub
        //Doesn't need to be cleaned up
        sleep_context = new ucontext_t;
        char* stack = new char[STACK_SIZE];
        base_pointer_map[sleep_context] = stack;
        sleep_context->uc_stack.ss_sp = stack;
        sleep_context->uc_stack.ss_size = STACK_SIZE;
        sleep_context->uc_stack.ss_flags = 0;
        makecontext(sleep_context, (void (*)()) sleeper_func, 0);
        //Sleep context init
        
    }
    catch(const std::exception &e){
        // throw and prevent user code from running with invariants
        std::exception_ptr p = std::current_exception();
        cpu::guard.store(false);
        cpu::interrupt_enable();    
        std::rethrow_exception(p);
    }

    //Reassign cpu::impl pointer to get cleaned up
    std::unique_ptr<impl> u_ptr(imp);
    this->impl_ptr = u_ptr.get();
    //Reassigned sleeper context for each CPU
    this->impl_ptr->sleep_context = sleep_context;

    if (func == nullptr)
    {
        //sleep 
        setcontext(sleep_context);

    }
    else{
        //Initial context creation and set
        ucontext_t *thread_cont;
        thread_cont = init_context(func, arg);
        context_to_thread[thread_cont] = nullptr;
        this->impl_ptr->running_context = thread_cont;
        setcontext(thread_cont);
    }
}