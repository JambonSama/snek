// STD
#include <algorithm>
#include <chrono>
#include <ctime>
#ifdef _WIN32
#pragma warning(push, 0)
#include <experimental/net>
#pragma warning(pop)
#else
#include <experimental/net>
#endif

#include <cstdarg>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

using std::cout;
using std::endl;
using namespace std::chrono_literals;

using u32 = uint32_t;
using u64 = uint64_t;

using i32 = int32_t;
using i64 = int64_t;

// THIRD-PARTY
#include <SFML/Graphics.hpp>
