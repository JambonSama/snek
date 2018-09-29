#include "network.h"
#include "engine.h"
#include "stable_win32.hpp"

static constexpr auto PORT = "5677";
static constexpr auto PORTN = 5677;
static constexpr auto IP = "localhost";

Network::Client::Client(net::io_context &ctx) : socket(ctx), resolver(ctx) {}

Network::ConnectedClient::ConnectedClient(net::io_context &ctx,
                                          net::ip::tcp::socket socket_)
    : socket(std::move(socket_)) {}

Network::Server::Server(net::io_context &ctx) : acceptor(ctx) {}

Network::~Network() {
    ctx.stop();
    work.reset();
    work_thread.join();
}

void Network::update() {}

Network::Network()
    : work(net::make_work_guard(ctx)), work_thread([this]() { ctx.run(); }) {
    state.emplace<None>();
}

void Network::connect() {
    add_message("connecting to server");

    state.emplace<Client>(ctx);
    auto &client = std::get<Client>(state);

    net::async_connect(client.socket, client.resolver.resolve(IP, PORT),
                       [&client](auto ec, auto endpoint) {
                           if (!ec) {
                               add_message("connected to localhost");
                               client.connected = true;
                           } else {
                               add_message("Failed to connect: %s",
                                           ec.message().c_str());
                               client.connected = false;
                           }
                       });
}

void Network::start_server() {

    try {
        auto &server = state.emplace<Server>(ctx);
        server.endpoint = net::ip::tcp::endpoint(net::ip::tcp::v4(), PORTN);
        server.acceptor.open(server.endpoint.protocol());
        server.acceptor.bind(server.endpoint);
        server.acceptor.listen();
        start_accept();
    } catch (std::exception &e) {
        add_message("Failed to start server: %s", e.what());
        state.emplace<None>();
    }

    add_message("Started server");
}

void Network::stop_server() {
    try {
        if (auto server = std::get_if<Server>(&state)) {
            state.emplace<None>();
        }
    } catch (std::exception &e) {
        add_message("Exception while stopping server: %s", e.what());
    }
    add_message("Stopped server");
}

void Network::start_accept() {
    if (auto server = std::get_if<Server>(&state); server) {
        server->acceptor.async_accept(
            [this](std::error_code ec, net::ip::tcp::socket socket) {
                std::lock_guard guard(mutex);
                if (auto server = std::get_if<Server>(&state)) {
                    if (!ec) {
                        const auto id = server->unique_client_id++;
                        server->clients.try_emplace(id, ctx, std::move(socket));
                        new_clients.push(id);
                        start_accept();

                        add_message("A client has connected to the server!");
                    } else {
                    }
                }
            });
    }
}

std::optional<Network::ClientID> Network::get_new_client() {
    if (new_clients.size() > 0)
        return new_clients.pop();
    return {};
}

void Network::send(Buffer &b, ClientID id) {

    if (auto server = std::get_if<Server>(&state)) {
        auto it = server->clients.find(id);
        if (it != server->clients.end()) {
            u32 to_send = b.bytes.size();
            std::error_code ec;
            net::write(it->second.socket,
                       net::const_buffer(&to_send, sizeof(to_send)), ec);

            auto size =
                net::write(it->second.socket,
                           net::const_buffer(b.bytes.data(), to_send), ec);
            bytes_sent += size + sizeof(to_send);
            //            printf("write %u / %u bytes\n", size, to_send);
        }
    } else if (auto client = std::get_if<Client>(&state)) {
        std::error_code ec;
        u32 to_send = b.bytes.size();
        net::write(client->socket, net::const_buffer(&to_send, sizeof(to_send)),
                   ec);
        auto size = net::write(client->socket,
                               net::const_buffer(b.bytes.data(), to_send), ec);
        bytes_sent += size + sizeof(to_send);
        // printf("write %u / %u bytes\n", size, to_send);
    }
}

bool Network::recv(Buffer &b, ClientID id) {
    b.reset();

    if (auto server = std::get_if<Server>(&state)) {
        if (auto it = server->clients.find(id); it != server->clients.end()) {
            auto &client = it->second;
            std::error_code ec;
            u32 to_read = 0;
            net::read(client.socket, net::dynamic_buffer(b.bytes),
                      net::transfer_exactly(sizeof(to_read)));
            b.read(to_read);
            // b.reset();
            auto size = net::read(client.socket, net::dynamic_buffer(b.bytes),
                                  net::transfer_exactly(to_read));

            bytes_received += size + sizeof(to_read);
            // printf("read %u bytes\n", size);
            if (!ec) {
                return true;
            } else {
                add_message("Error when reading from client %d: %s", id,
                            ec.message().c_str());
                return false;
            }
        }

    } else if (auto client = std::get_if<Client>(&state)) {
        std::error_code ec;
        u32 to_read = 0;
        net::read(client->socket, net::dynamic_buffer(b.bytes),
                  net::transfer_exactly(sizeof(to_read)));
        b.read(to_read);
        // b.reset();
        auto size = net::read(client->socket, net::dynamic_buffer(b.bytes),
                              net::transfer_exactly(to_read));

        bytes_received += size + sizeof(to_read);
        if (!ec) {
            return true;
        } else {
            client->connected = false;
            add_message("Error when reading from server: %s", id,
                        ec.message().c_str());
            return false;
        }
    }
    return false;
}

bool Network::connected() {
    if (auto server = std::get_if<Server>(&state)) {
        return true;
    } else if (auto client = std::get_if<Client>(&state)) {
        return client->connected;
    }
    return false;
}

void Network::print_stats() {
    printf("Network: tx = %3u  rx = %3u\n", bytes_sent, bytes_received);
    bytes_sent = 0;
    bytes_received = 0;
}
