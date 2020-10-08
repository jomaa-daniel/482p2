#pragma once
#include <queue>
#include <vector>
#include <unordered_map>
#include <ucontext.h>
#include "cpu.h"

extern  std::queue<ucontext_t*> ready_queue;
extern  std::vector<ucontext_t*> delete_queue; // TODO when we move something to the 
                                            // delete queue we need to check to see if something is waiting on it
extern  std::unordered_map<ucontext_t*, ucontext_t*> waiting_map; // A
extern  std::queue<cpu*> sleeping_queue;