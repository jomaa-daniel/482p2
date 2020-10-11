#pragma once
#include <queue>
#include <vector>
#include <unordered_map>
#include <ucontext.h>
#include "cpu.h"
#include "thread.h"
#include <set>

extern  std::queue<ucontext_t*> ready_queue;
extern  std::vector<ucontext_t*> delete_queue;
extern  std::unordered_map<ucontext_t*, std::queue<ucontext_t*>> waiting_map; // A
extern  std::queue<cpu*> sleeping_queue;
extern  std::unordered_map<ucontext_t *, thread*> context_to_thread;
extern  std::vector<cpu*> running_cpus;

// ==========
bool occurs_once();
bool cpus_run_different_contexts();
void clean_delete_list();
void wake_cpu();
bool search_for_context(ucontext_t*, std::queue<ucontext_t*>&);
bool search_for_context(ucontext_t*, std::vector<ucontext_t*>&);
int context_count(ucontext_t*);
