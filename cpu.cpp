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

std::queue<ucontext_t*> ready_queue;
std::vector<ucontext_t*> delete_queue; 
std::unordered_map<ucontext_t*, std::queue<ucontext_t*>> waiting_map; 
std::queue<cpu*> sleeping_queue;

void timer_handler()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    //printf("time handler was called\n");
    assert(cpus_run_different_contexts());
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context; 
    if (!ready_queue.empty() && this_context != cpu::self()->impl_ptr->sleep_context){               
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
    //printf("IPI was called\n");
    //assert(!ready_queue.empty());
    if(!ready_queue.empty())
    {
        cpu *cpu_ptr = cpu::self();
        ucontext_t *next_context = ready_queue.front();
        ready_queue.pop();
        cpu_ptr->impl_ptr->running_context = next_context;
        swapcontext(cpu::self()->impl_ptr->sleep_context, next_context); 
        //setcontext(next_context);        
    }
}


void sleeper_func(){
    while (true){
        //printf("Sleep function was called\n");
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

std::unordered_map<ucontext_t*, char*> base_pointer_map;

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
    std::unique_ptr<impl> u_ptr(new cpu::impl());
    this->impl_ptr = u_ptr.get();
    
    running_cpus.push_back(cpu::self());
        
    ucontext_t *sleep_context = new ucontext_t;

    char* stack = new char[STACK_SIZE];
    base_pointer_map[sleep_context] = stack;
    sleep_context->uc_stack.ss_sp = stack;
    sleep_context->uc_stack.ss_size = STACK_SIZE;
    sleep_context->uc_stack.ss_flags = 0;
    makecontext(sleep_context, (void (*)()) sleeper_func, 0);

    this->impl_ptr->sleep_context = sleep_context;

    if (func == nullptr)
    {
        //sleep 
        setcontext(sleep_context);

    }
    else{
        ucontext_t *thread_cont;
        thread_cont = init_context(func, arg);
        context_to_thread[thread_cont] = nullptr;
        this->impl_ptr->running_context = thread_cont;
        //printf("Thread A is %p\n", thread_cont);
        setcontext(thread_cont);
    }
}