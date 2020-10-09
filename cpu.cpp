#include <vector>
#include "cpu.h"
#include <iostream> 
#include <ucontext.h>
#include "cpu_impl.h"
#include <memory>
#include "globals.h"
#include <cstring>
using std::cout;

std::queue<ucontext_t*> ready_queue;
std::vector<ucontext_t*> delete_queue; // TODO when we move something to the 
                                            // delete queue we need to check to see if something is waiting on it
std::unordered_map<ucontext_t*, std::queue<ucontext_t*>> waiting_map; // A
std::queue<cpu*> sleeping_queue;



void timer_handler()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    if (!ready_queue.empty()){
            
    ucontext_t* next_context = ready_queue.front();
    ready_queue.pop(); 
    ucontext_t* this_context = cpu::self()->impl_ptr->running_context; 
    cpu::self()->impl_ptr->running_context = next_context;
    ready_queue.push(this_context);
    swapcontext(this_context, next_context);
    }
    cpu::guard = false;
    cpu::interrupt_enable();    
}

void ipi_handler()
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    
    if (ready_queue.empty())
    {
        sleeping_queue.push(cpu::self());
        cpu::guard = false;
        cpu::interrupt_enable_suspend();
    }
    else
    {
        cpu *cpu_ptr = cpu::self();
        ucontext_t *next_context = ready_queue.front();
        ready_queue.pop();
        cpu_ptr->impl_ptr->running_context = next_context;
        setcontext(next_context); 
    }

    cpu::guard = false;
    cpu::interrupt_enable();
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
    delete[] (char*) base_pointer_map[ucp];
    base_pointer_map.erase(ucp);
    delete ucp; 
    ucp = nullptr;
    
}




// ====================== CPU IMPLEMENTATION ========================
void cpu::init(thread_startfunc_t func, void * arg)
{

    cpu::interrupt_vector_table[TIMER] = timer_handler;
    cpu::interrupt_vector_table[IPI] = ipi_handler;
    std::unique_ptr<impl> u_ptr(new cpu::impl());

    // sAfety
    while (cpu::guard.exchange(true)){}

    if (func == nullptr)
    {
        //sleep 
        cpu::guard = false;
        sleeping_queue.push(cpu::self());
        cpu::interrupt_enable_suspend();
    }
    else{
        this->impl_ptr = u_ptr.get();
        ucontext_t *thread_cont;

        thread_cont = init_context(func, arg);
        this->impl_ptr->running_context = thread_cont;
        setcontext(thread_cont);
    }
}