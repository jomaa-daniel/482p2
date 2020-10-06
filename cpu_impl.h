class cpu::impl
{
    public:
    ucontext_t *running_context = nullptr;
    impl();
    ~impl();
};