#include "network.h"
#include "main.h"
#include "stable_win32.hpp"

Network::Client::Client(net::io_context &ctx) : socket(ctx), resolver(ctx) {}

Network::ConnectedClient::ConnectedClient(net::io_context &ctx,
                                          net::ip::tcp::socket socket_)
    : socket(std::move(socket_)) {
    std::string msg;
    add_message("new client");
    //    net::async_read(socket, net::dynamic_buffer(msg),
    //                    [this](auto ec, auto size) {
    //                        add_message("received message of size %d", size);
    //                    });
    //    add_message("Received message: %s", msg.c_str());

    //    add_message("Received message: %s", msg.c_str());

    start_read();
}

void Network::ConnectedClient::start_read() {
    net::async_read(socket, net::dynamic_buffer(buffer),
                    net::transfer_at_least(1), [this](auto ec, auto size) {
                        on_read(size);
                        start_read();
                    });
}

void Network::ConnectedClient::on_read(size_t size) {
    if (size != 0) {
        add_message("Received %d bytes", size);
    } else {
        socket.close();
    }
}

Network::Server::Server(net::io_context &ctx) : acceptor(ctx) {}

Network::~Network() {
    ctx.stop();
    work.reset();
    work_thread.join();
}

Network::Status Network::status() {
    if (std::holds_alternative<Client>(state)) {
        return Status::Client;
    } else if (std::holds_alternative<Server>(state)) {
        return Status::Server;
    } else {
        return Status::None;
    }
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

    net::async_connect(
        client.socket, client.resolver.resolve("localhost", "5678"),
        [&client](auto ec, auto endpoint) {
            if (!ec) {
                add_message("connected to localhost");
                std::string msg = "Hello Snake! Kacer kac kac FF";

                //            net::async_write(client.socket,
                //            net::buffer(msg),
                //                             [](auto ec, auto size) {});
                auto n = net::write(client.socket, net::buffer(msg));
                add_message("sent message, n = %d", n);
            } else {
                add_message("Failed to connect: %s", ec.message().c_str());
            }
        });
}

void Network::start_server() {

    try {
        auto &server = state.emplace<Server>(ctx);
        server.endpoint = net::ip::tcp::endpoint(net::ip::tcp::v4(), 5678);
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
                if (auto server = std::get_if<Server>(&state)) {
                    if (!ec) {
                        server->clients.emplace_back(ctx, std::move(socket));
                        start_accept();

                        add_message("A client has connected to the server!");
                    } else {
                    }
                }
            });
    }
}
