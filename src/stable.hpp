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

#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
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

using u8 = uint8_t;
using i8 = int8_t;

// THIRD-PARTY
#include <SFML/Graphics.hpp>
//#include <zmq.hpp>
