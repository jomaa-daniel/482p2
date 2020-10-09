#include "thread.h" 
#include <ucontext.h>

class thread::impl{
    public:
    ucontext_t *context_ptr;
    int thread_id;
    impl();
    ~impl();
};