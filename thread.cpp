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

//Thread id starting num
static int num_threads = 1000;
//Map from contexts to threads,
//since a context is not aware of what thread owns it
std::unordered_map<ucontext_t*, thread*> context_to_thread;
//Keeping track of our current running cpus
// and what context they are running
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

// checks to see if there is a cpu that is a asleep and something needs to run
void wake_cpu(){
    if(!sleeping_queue.empty() and !ready_queue.empty()){
        cpu *front = sleeping_queue.front();
        sleeping_queue.pop();
        assert(front != nullptr);
        front->interrupt_send();
    }
}

// routine delete contexts
void clean_delete_list(){
    for (auto ptr : delete_queue)
    {
        assert(context_to_thread.find(ptr) != end(context_to_thread));
        context_to_thread.erase(ptr);
        delete_context(ptr);
    }
    delete_queue.clear();
}

// cross check for duplicate contexts
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

// cross check for duplicate contexts overloaded function
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

// check for the number of contexts accross all living data structures
// this should only ever return 1 or 0 if it has been deleted
// during user code
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

void waitTillN(int waitNum)
{
        // since there is yet to be a total of 3 threads,
        // this thread needs wait till the message queue has sufficient
        // number of threads adding their message
        while (message_queue.size() != waitNum)
        {
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
}



// ================= END utility functions ==============
// MIDTERM CHANGE
std::deque<ucontext_t* > message_queue;
std::unordered_map<thread*, thread*> message_waiting_map;

// MIDTERM CHANGE
std::string thread::swap3(const std::string& str)
{
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    int message_position = 0;
    message_queue.push_back(cpu::self()->impl_ptr->running_context);
    // if this is the first message it needs to wait till C pushed
    if (message_queue.size() == 1)
    {
        waitTillC(3);
        // A passes to B after C finishes
        // wake up the contexxt in the 1th position which is B, since
        // A is now woken up
        // A will pop the message from the thread impl,see thread impl 
        // impleemntation
    }
    else if (message_queue.size() == 2)
    {
        // if this is the second message i.e. B, it needs to pass to C
        // and wait for A to be done
        waitTillN(1);

        // B passes to C after A finishes
        // wake up the contexxt in the 1th position which is B, since
        // B is now woken up
        // B will pop the message from the thread impl,see thread impl 
        // impleemntation
    }
    else
    {
    // C gets the message and wakes up A
        
    }
    
    



    cpu::guard.store(false);
    cpu::interrupt_enable();
}


void thread_exit(){
    
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
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
            wake_cpu();
            waiting_map[current_context].pop();
        }  
        waiting_map.erase(current_context);
    }
 
    assert(context_count(current_context) == 1);
    assert(count_running_instances(current_context) == 1);

    if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();

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

    // User code lifetime starts here
    func(arg);
    // ends here


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
    try{
        this->impl_ptr = new impl();

        this->impl_ptr->context_ptr = init_context(func, arg);

        context_to_thread[this->impl_ptr->context_ptr] = this;
        
        ready_queue.push(this->impl_ptr->context_ptr);    
        wake_cpu();
    }
    catch(const std::exception &e)
    {   
        // prevent user code from running with invariants
        std::exception_ptr p = std::current_exception();
        cpu::guard.store(false);
        cpu::interrupt_enable();    
        std::rethrow_exception(p);
    }

    cpu::guard.store(false);
    cpu::interrupt_enable();

}

thread::~thread(){

    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)) {}
    
    if (context_to_thread.find(this->impl_ptr->context_ptr) 
            != end(context_to_thread))
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
    // if the ready queue is empty skip and continue 
    // else switch to next ready
    if(!ready_queue.empty()){
        cpu *cpu_ptr = cpu::self();
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
    clean_delete_list();

    cpu::guard.store(false);
    cpu::interrupt_enable();
}


void thread::join(){
    
    cpu::interrupt_disable();
    while (cpu::guard.exchange(true)){}
    assert(cpus_run_different_contexts());
    assert(this->impl_ptr != nullptr);
    assert(this->impl_ptr->context_ptr != nullptr);

    // the thread context still thinks that it points to a live context
    // if this threads context is on delete queue, continue, ignore everything and go
    
    // if the thread that is joined is still alive...
    if ( this->impl_ptr->alive == true){
        ucontext_t* this_context = cpu::self()->impl_ptr->running_context;
        assert(context_count(this_context) == 0);

        // create a mapping that, when the thread exits, moves all contexts
            // waiting for this context to complete onto the ready queue
        if (waiting_map.find(this->impl_ptr->context_ptr) == end(waiting_map))
        {
            waiting_map[this->impl_ptr->context_ptr] = std::queue<ucontext_t*>();
        }
        waiting_map[this->impl_ptr->context_ptr].push(this_context);
        
        // if there is nothing on the ready queue just go to sleep
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
    clean_delete_list();
    
    cpu::guard.store(false);
    cpu::interrupt_enable();
}
