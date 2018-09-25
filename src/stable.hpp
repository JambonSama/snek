#pragma once

#ifdef _WIN32

// STD
#include <algorithm>
#include <chrono>
#include <ctime>
#pragma warning (push, 0)
#include <experimental/net>
#pragma warning (pop)

#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <stdarg.h>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
#include <cstdint>

using std::cout;
using std::endl;
using namespace std::chrono_literals;

using u32 = uint32_t;
using u64 = uint64_t;

using i32 = int32_t;
using i64 = int64_t;

// THIRD-PARTY
#pragma warning (push, 0)
#include <SFML/Graphics.hpp>
#pragma warning (pop)

#endif
