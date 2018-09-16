#pragma once
#include "stable.hpp"
namespace net = std::experimental::net;

struct Network {

    Network();
    ~Network();

    net::io_context ctx;
    net::executor_work_guard<net::io_context::executor_type> work;
    std::thread work_thread;

    enum class Status { None, Client, Server };

    struct None {};

    struct Client {

        net::ip::tcp::socket socket;
        net::ip::tcp::endpoint endpoint;
        net::ip::tcp::resolver resolver;

        Client(net::io_context &ctx);
    };

    struct ConnectedClient {
        net::ip::tcp::socket socket;
        net::ip::tcp::endpoint endpoint;

        std::string buffer;

        ConnectedClient(net::io_context &ctx, net::ip::tcp::socket socket_);
        void start_read();
        void on_read(size_t size);
    };

    struct Server {
        net::ip::tcp::acceptor acceptor;
        std::vector<ConnectedClient> clients;
        net::ip::tcp::endpoint endpoint;

        std::string buffer;

        Server(net::io_context &ctx);
    };

    struct Buffer {
        std::vector<uint8_t> bytes;
        int start_index = 0;

        template <typename T> void write(const T &t) {
            auto ptr = static_cast<const uint8_t *>(&t);
            bytes.insert(bytes.end(), ptr, ptr + sizeof(T));
        }

        template <typename T> void read(T &t) {
            if (start_index + sizeof(T) < bytes.size()) {
                memcpy(&t, &bytes[start_index], sizeof(T));
                start_index += sizeof(T);
            }
        }

        template <typename T> Buffer &operator<<(const T &t) {
            write(t);
            return *this;
        }

        template <typename T> Buffer &operator>>(T &t) {
            read(t);
            return *this;
        }
    };

    std::variant<None, Client, Server> state;

    Status status();

    void update();
    void connect();

    void start_server();

    void start_accept();
};
