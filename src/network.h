#pragma once
#include "stable_win32.hpp"
namespace net = std::experimental::net;

template <typename T, int N, bool mutex_acces = false> class CircularBuffer {
public:
    void push(const T &t) {
        if constexpr (mutex_acces) {
            std::lock_guard guard(mutex);
            do_push(t);
        } else {
            do_push(t);
        }
    }

    T pop() {
        if constexpr (mutex_acces) {
            std::lock_guard guard(mutex);
            return do_pop();
        } else {
            return do_pop();
        }
    }

    size_t size() { return size_; }

private:
    void do_push(const T &t) {
        while (size_ >= N)
            do_pop();
        data_[push_index_++] = t;
        if (push_index_ >= N)
            push_index_ = 0;
        ++size_;
    }

    T do_pop() {
        assert(size_ > 0);
        T ret = data_[pop_index_++];
        if (pop_index_ >= N)
            pop_index_ = 0;
        --size_;
        return ret;
    }

    size_t push_index_ = 0;
    size_t pop_index_ = 0;
    std::atomic<size_t> size_ = 0;
    std::array<T, N> data_;
    std::mutex mutex;
};

struct Network {

    Network();
    ~Network();

    net::io_context ctx;
    net::executor_work_guard<net::io_context::executor_type> work;
    std::thread work_thread;

    struct Buffer {
        std::vector<u8> bytes;
        u32 start_index = 0;

        void reset() {
            bytes.clear();
            start_index = 0;
        }

        bool empty() { return start_index >= bytes.size(); }

        template <typename T> void write(const T &t) {
            auto ptr = reinterpret_cast<const uint8_t *>(&t);
            bytes.insert(bytes.end(), ptr, ptr + sizeof(T));
        }

        template <typename T> void read(T &t) {
            if (start_index + sizeof(T) <= bytes.size()) {
                memcpy(&t, &bytes[start_index], sizeof(T));
                start_index += sizeof(T);
            }
        }
    };

    enum class Status { None, Client, Server };

    struct None {};

    using ClientID = u32;

    struct Client {

        net::ip::tcp::socket socket;
        net::ip::tcp::endpoint endpoint;
        net::ip::tcp::resolver resolver;

        Client(net::io_context &ctx);

        std::atomic_bool connected = false;
    };

    struct ConnectedClient {
        net::ip::tcp::socket socket;
        net::ip::tcp::endpoint endpoint;

        ConnectedClient(net::io_context &ctx, net::ip::tcp::socket socket_);
    };

    struct Server {

        net::ip::tcp::acceptor acceptor;
        std::unordered_map<ClientID, ConnectedClient> clients;
        net::ip::tcp::endpoint endpoint;

        ClientID unique_client_id = 1;

        Server(net::io_context &ctx);
    };

    std::mutex mutex;

    CircularBuffer<ClientID, 20, true> new_clients;

    std::variant<None, Client, Server> state;

    void update();
    void connect();

    void start_server();
    void stop_server();

    void start_accept();

    std::optional<ClientID> get_new_client();

    void send(Buffer &b, ClientID id);
    bool recv(Buffer &b, ClientID id);

    bool connected();
    void print_stats();

    u32 bytes_sent = 0;
    u32 bytes_received = 0;
};
