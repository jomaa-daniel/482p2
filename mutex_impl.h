#include <queue>
#include <ucontext.h>

class mutex::impl
{
    public:
    bool busy;
    std::queue<ucontext_t *> waiting_queue;
    impl();
    ~impl();
};

