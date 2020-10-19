#pragma once
#include "cv.h"
#include <queue>
#include <ucontext.h>
 

class cv::impl{
    public:
    std::queue<ucontext_t *> cv_signal_queue;
    impl();
    ~impl();
};


