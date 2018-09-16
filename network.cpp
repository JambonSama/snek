#include "network.h"
#include "main.h"

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
    add_message("Received %d bytes", size);
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

void Network::update() {
    switch (status()) {
    case Status::Client: {
        //        auto &client = std::get<Client>(state);
    } break;
    case Status::Server: {
        auto &server = std::get<Server>(state);

    } break;
    case Status::None: {

    } break;
    }
}

Network::Network()
    : work(net::make_work_guard(ctx)), work_thread([this]() { ctx.run(); }) {
    state.emplace<None>();
}

void Network::connect() {
    add_message("connecting to server");
    {
        state.emplace<Client>(ctx);
        auto &client = std::get<Client>(state);
        try {
            net::connect(client.socket,
                         client.resolver.resolve("localhost", "5678"));
            add_message("connected to localhost");
            std::string msg = "Hello Snake! Kacer kac kac FF";

            //            net::async_write(client.socket, net::buffer(msg),
            //                             [](auto ec, auto size) {});
            auto n = net::write(client.socket, net::buffer(msg));
            add_message("sent message, n = %d", n);
        } catch (std::exception &e) {
            add_message("Failed to connect: %s", e.what());
            state.emplace<None>();
        }
    }
}

void Network::start_server() {
    add_message("starting server...");
    try {
        state.emplace<Server>(ctx);
        auto &server = std::get<Server>(state);
        server.endpoint = net::ip::tcp::endpoint(net::ip::tcp::v4(), 5678);
        server.acceptor.open(server.endpoint.protocol());
        server.acceptor.bind(server.endpoint);
        server.acceptor.listen();
        start_accept();
    } catch (std::exception &e) {
        add_message("Failed to start server: %s", e.what());
        state.emplace<None>();
    }
}

void Network::start_accept() {
    auto &server = std::get<Server>(state);
    //        auto new_socket = server.acceptor.accept();
    //        add_message("A client has connected to the server!");
    server.acceptor.async_accept(
        [this](std::error_code, net::ip::tcp::socket socket) {
            add_message("A client has connected to the server!");
            auto &server = std::get<Server>(this->state);
            server.clients.emplace_back(ctx, std::move(socket));
            start_accept();
        });
}
