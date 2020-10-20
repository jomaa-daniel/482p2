# pragma once
#include <queue>
#include <ucontext.h>
#include "mutex.h"
class mutex::impl
{
    public:
    bool busy;
    std::queue<ucontext_t *> mutex_waiting_queue;
    ucontext_t *holder;
    impl();
    ~impl();
};

