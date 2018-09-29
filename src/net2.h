#pragma once
#include "stable_win32.hpp"

class Net2 {
public:
    Net2();

private:
    zmq::context_t ctx;
};
