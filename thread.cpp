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
#include "thread_impl.h"
#include <cassert>
#include <unordered_set>

using std::cout;

static int num_threads = 1000;
std::unordered_map<ucontext_t*, thread*> context_to_thread;
std::vector<cpu*> running_cpus;

// =============== utility functions for asserts =============

bool cpus_run_different_contexts(){
    std::unordered_set<ucontext_t *> set;
    for(auto cpu:running_cpus){
        if(cpu->impl_ptr->running_context != nullptr
         && set.count(cpu->impl_ptr->running_context) == 1){
            return false;
        }
        set.insert(cpu->impl_ptr->running_context);
    }
    return true;
}

void wake_cpu(){
    if(!sleeping_queue.empty() and !ready_queue.empty()){
        cpu *front = sleeping_queue.front();
        sleeping_queue.pop();
        assert(front != nullptr);
        front->interrupt_send();
    }
}


void clean_delete_list(){
    for (auto ptr : delete_queue)
    {
        //printf("Calling clean delete list\n");
        assert(context_to_thread.find(ptr) != end(context_to_thread));
        context_to_thread.erase(ptr);
        delete_context(ptr);
    }
    delete_queue.clear();
}

bool search_for_context(ucontext_t* this_context, std::queue<ucontext_t*> s)
{
    while (!s.empty())
    {
        if (s.front() == this_context)
        {
            return true;
        }
        s.pop();
    }
    return false;
}
bool search_for_context(ucontext_t* this_context, std::vector<ucontext_t*>& s)
{
    auto start = begin(s);
    while (start != end(s))
    {
        if (*start == this_context)
        {
            return true;
        }
    }
    return false;
}

int context_count(ucontext_t* check_context){
    int total_count = 0;
    int sz = ready_queue.size();
    for (int i = 0; i < sz; ++i)
    {
        ucontext_t* next_context = ready_queue.front();
        ready_queue.pop();
        if (check_context == next_context)
        {
            ++total_count;
        }
        ready_queue.push(next_context);
    }
    
    
    for (auto context_ptr : delete_queue)
    {
        if (context_ptr == check_context)
        {
            ++total_count;
        }
    }

    for(auto& context_ptr_pair : waiting_map){
        sz = context_ptr_pair.second.size();
        for (int i = 0; i < sz; ++i)
        {
            ucontext_t* next_context = context_ptr_pair.second.front();
            context_ptr_pair.second.pop();
            if (check_context == next_context)
            {
                ++total_count;
            }
            context_ptr_pair.second.push(next_context);
        }
    }
    return total_count;
}

int count_running_instances(ucontext_t* check)
{
    int num_seen = 0;
    for (auto cpu_ptr : running_cpus)
    {
        if (cpu_ptr->impl_ptr->running_context == check)
        {
            ++num_seen;
        }
    }
    return num_seen;
}

// ================= END utility functions ==============

void thread_exit(){
    
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    //printf("Calling thread exit\n");
    assert(cpus_run_different_contexts());
    ucontext_t *current_context = cpu::self()->impl_ptr->running_context;
    assert(context_count(current_context) == 0);
    delete_queue.push_back(current_context);
    assert(context_count(current_context) == 1);
    // inform the thread that its context is now dead
    assert(context_to_thread.find(current_context) != end(context_to_thread));
 
    thread *corres_thread = context_to_thread[current_context];
    if (corres_thread != nullptr)
    {
        corres_thread->impl_ptr->alive = false;
    }
    

    // check to see if anything is waiting on this to end
    if (waiting_map.find(current_context) != end(waiting_map))
    {
        // pop all the people waiting to call join in order FIFO
        while (!waiting_map[current_context].empty())
        {
            ucontext_t* next_context =  waiting_map[current_context].front();
            ready_queue.push(next_context);
            //printf("Thread exiting waking up person waiting for me\n");
            wake_cpu();
            waiting_map[current_context].pop();
        }  
        waiting_map.erase(current_context);
    }
 
    assert(context_count(current_context) == 1);
    assert(count_running_instances(current_context) == 1);

    if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();
        // context switch
        // done
        // disable
        ucontext_t * next_context = ready_queue.front();
        ready_queue.pop();
        
        cpu_ptr->impl_ptr->running_context = next_context;
        setcontext(next_context); // INVARIANT: interrupts disabled
        assert_interrupts_disabled();
    }
    else
    {
        setcontext(cpu::self()->impl_ptr->sleep_context);
    }
}

void stub(thread_startfunc_t func, void *arg){

    cpu::guard.store(false);
    cpu::interrupt_enable();
    //Not sure if this is correct
    func(arg);


    //cleanup
    thread_exit();
}


thread::impl::impl()
{
    this->context_ptr = nullptr;
    this->alive = true;
    this->thread_id = num_threads++;
}

thread::impl::~impl()
{
    this->context_ptr = nullptr;
}


thread::thread(thread_startfunc_t func, void* arg){

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    this->impl_ptr = new impl();

    this->impl_ptr->context_ptr = init_context(func, arg);
    //printf("Thread %i belongs to %p\n", this->impl_ptr->thread_id, this->impl_ptr->context_ptr);

    context_to_thread[this->impl_ptr->context_ptr] = this;
    
    ready_queue.push(this->impl_ptr->context_ptr);    
    //printf("INIT Thread %i\n", this->impl_ptr->thread_id);
    wake_cpu();
    

    cpu::guard.store(false);
    cpu::interrupt_enable();

}

thread::~thread(){
   //printf("dealloc thread: %i\n",thread::impl_ptr->thread_id);
    // assert this context is somewhere else
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    //printf("Destructing thread %i\n", this->impl_ptr->thread_id);
    
    if (context_to_thread.find(this->impl_ptr->context_ptr) != end(context_to_thread))
    {context_to_thread[this->impl_ptr->context_ptr] = nullptr;}

    delete impl_ptr;
    
    cpu::guard.store(false);
    cpu::interrupt_enable();
}

void thread::yield(){

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    // check to see if this threads context exists elsewhere
    assert(cpus_run_different_contexts());
    //No other ready threads
    if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();
        // context switch
        // done
        // disable
        ucontext_t *next_context = ready_queue.front();
        assert(context_count(next_context) == 1);
        ready_queue.pop();
        ucontext_t *this_context = cpu_ptr->impl_ptr->running_context;
        assert(context_count(this_context) == 0); 
        
        ready_queue.push(this_context); // TODO make sure no wake up cpu needed
        cpu_ptr->impl_ptr->running_context = next_context;
        swapcontext(this_context, next_context); // INVARIANT: interrupts disabled
        assert_interrupts_disabled();
    }
    
    //Delete done threads
    //printf("Del %i threads in yeild\n", delete_queue.size());
    clean_delete_list();

    cpu::guard.store(false);
    cpu::interrupt_enable();
}

// A: A waiting B finishes, B's CPU switches context to C, A says B.join();

void thread::join(){
// do NOT run the original thread when this is done, threads need 
  // to be run in FIFO order. just put the caller thread on the ready Q

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    assert(cpus_run_different_contexts());
    assert(this->impl_ptr != nullptr);
    assert(this->impl_ptr->context_ptr != nullptr);
    // make sure this context is not on the delete queue
    //assert(!search_for_context(this->impl_ptr->context_ptr, delete_queue));
    // the thread context still thinks that it points to a live context
    // if thi
    // if this threads context is on delete queue, continue, ignore everything and go
    
    if ( this->impl_ptr->alive == true){
        ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
        assert(context_count(this_context) == 0);
        if (waiting_map.find(this->impl_ptr->context_ptr) == end(waiting_map))
        {
            waiting_map[this->impl_ptr->context_ptr] = std::queue<ucontext_t*>();
        }
        waiting_map[this->impl_ptr->context_ptr].push(this_context);
        
        if (ready_queue.empty())
        {
            swapcontext(this_context, cpu::self()->impl_ptr->sleep_context);
        }
        else
        {
            ucontext_t *next_context = ready_queue.front(); 
            assert(context_count(next_context) == 1);
            ready_queue.pop();
            cpu::self()->impl_ptr->running_context = next_context;
            swapcontext(this_context, next_context);
        }
            
    }
    //Delete done threads
    //printf("Del %i threads in join\n", delete_queue.size());
    clean_delete_list();
    
    cpu::guard.store(false);
    cpu::interrupt_enable();
}
