class cpu::impl
{
    public:
    ucontext_t *running_context = nullptr;
    impl();
    ~impl();
};

void delete_context(ucontext_t* ptr);

ucontext_t* init_context(thread_startfunc_t func, void *arg);

