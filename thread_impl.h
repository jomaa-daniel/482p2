#include "thread.h" 
#include <ucontext.h>
#include <string>

class thread::impl{
    public:
    ucontext_t *context_ptr;
    int thread_id;
    bool alive;
    std::string message;
    void message_sleep();
    impl();
    ~impl();
};