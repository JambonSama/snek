#include "snake.h"
#include "engine.h"
#include "stable_win32.hpp"

SnakeGame::Food::Food(int x, int y) : p(x, y) {}
void SnakeGame::Cell::reset() { food = nullptr; }

SnakeGame::Direction SnakeGame::set_dir(Player &player, Direction d) {
    if (player.dir == Direction::Down && d == Direction::Up)
        return player.dir;

    if (player.dir == Direction::Left && d == Direction::Right)
        return player.dir;

    if (player.dir == Direction::Right && d == Direction::Left)
        return player.dir;

    if (player.dir == Direction::Up && d == Direction::Down)
        return player.dir;

    return d;
}

void SnakeGame::SinglePlayer::recompute_spawn_points() {
    auto n = players.size();
    u32 i = 0;
    for (auto &p : players) {
        p.second.spawnY = game.gridRows - 10;
        p.second.spawnX = game.gridCols * (i + 1) / (n + 1);
        add_message("spawn pos: %d %d", p.second.spawnX, p.second.spawnY);
        ++i;
    }
}

void SnakeGame::SinglePlayer::spawn(Player &player) {

    player.body.resize(player.initialSize);
    player.body[0] = {static_cast<int>(player.spawnX),
                      static_cast<int>(player.spawnY)};

    for (size_t i = 1; i < player.body.size(); ++i) {
        player.body[i].x = player.body[i - 1].x;
        player.body[i].y = player.body[i - 1].y + 1;
    }

    player.dir = player.spawn_dir;
    player.dead = false;
}

// leave food behind where the body was
void SnakeGame::SinglePlayer::decompose(Player &player) {
    for (size_t i = 1; i < player.body.size(); ++i) {
        add_food(player.body[i].x, player.body[i].y);
    }

    player.input_buffer.clear();
}

void SnakeGame::SinglePlayer::add_food(int x, int y) {
    if (world_map(x, y).food == nullptr) {
        auto f = new Food{x, y};
        world_map(x, y).food = f;
        food.push_back(f);
    }
}

void SnakeGame::SinglePlayer::remove_food(int x, int y) {
    auto f = world_map(x, y).food;
    if (f != nullptr) {
        std::remove_if(food.begin(), food.end(),
                       [f](auto food_p) { return food_p == f; });
        delete f;
    }
    world_map(x, y).food = nullptr;
}

void SnakeGame::SinglePlayer::reset_map() {
    for (auto f : food) {
        remove_food(f->p.x, f->p.y);
    }
    //    for (int y = 0; y < world_map.h(); ++y) {
    //        for (int x = 0; x < world_map.w(); ++x) {
    //            world_map(x, y).reset();
    //        }
    //    }
}

bool SnakeGame::on_player(const Player &p1, const Player &p2) {
    return on_player(p1, p2, p1.body[0].x, p1.body[0].y);
}

bool SnakeGame::on_player(const Player &p1, const Player &p2, int x, int y) {

    const size_t i0 = p1.id == p2.id ? 1 : 0;
    for (size_t i = i0; i < p2.body.size(); ++i) {
        if (p2.body[i].x == x && p2.body[i].y == y) {
            return true;
        }
    }
    return false;
}

void SnakeGame::init() {
    hand_font.loadFromFile("resources/fonts/act.ttf");
    pause_text.setFont(hand_font);
    pause_text.setCharacterSize(200);
    pause_text.setString("Paused");
    sf::FloatRect BB = pause_text.getLocalBounds();
    pause_text.setOrigin(BB.width / 2, BB.height);

    gridCols = window->getSize().x / gridSize;
    gridRows = window->getSize().y / gridSize;
    sf::Vector2u temp(gridCols * gridSize, gridRows * gridSize);
    window->setSize(temp);

    sf::View view(
        sf::FloatRect(0, 0, gridCols * gridSize, gridRows * gridSize));
    window->setView(view);

    pause_text.setPosition(window->getSize().x / 2, window->getSize().y / 2);

    body_shape.setSize(sf::Vector2f(gridSize, gridSize));
    body_shape.setFillColor({255, 0, 0, 255});

    food_shape.setSize(sf::Vector2f(gridSize / 2, gridSize / 2));
    food_shape.setFillColor({0, 255, 0, 255});
    food_shape.setOrigin(-(int)gridSize / 4, -(int)gridSize / 4);
}

// void SnakeGame::input_key_(sf::Keyboard::Key k, bool down) {
//    if (game_status == GameStatus::SinglePlayer) {
//        auto &player = players.at(local_id);
//        if (k == sf::Keyboard::P && down) {
//            paused = !paused;
//        } else if (k == sf::Keyboard::Space && down) {
//            player.boost = true;
//        } else if (k == sf::Keyboard::Space && !down) {
//            player.boost = false;
//        } else if (down) {
//            paused = false;
//            player.input_buffer.emplace_back(k);
//        }
//    }
//}

void SnakeGame::update(Input &input, float dt) {

    if (auto s = std::get_if<MainMenu>(&state)) {
        main_menu(*s, input, dt);
    } else if (auto s = std::get_if<SinglePlayer>(&state)) {
        single_player(*s, input, dt);
    } else if (auto s = std::get_if<HostLobby>(&state)) {
        host_lobby(*s, input, dt);
    } else if (auto s = std::get_if<GuestLobby>(&state)) {
        guest_lobby(*s, input, dt);
    }
}

sf::Color SnakeGame::get_random_color() {
    sf::Color color;
    color.r = rand() % 255;
    color.g = rand() % 255;
    color.b = 255 * 2 - color.r - color.g;
    return color;
}

// void SnakeGame::main_menu(Input &input, float dt) {
//    auto [w, h] = window->getView().getSize();
//    const int N = 4;
//    if (ui::push_button(w / 2, 1 * h / (N + 1), "MainMenu##Single Player",
//                        ui::Align::Center)) {
//        players.clear();
//        game_status = GameStatus::SinglePlayer;
//        add_player(local_id);
//        /*for (int i = 0; i < 20; i++)
//            add_player();*/
//        auto &player = players.at(local_id);
//        player.use_ai = false;
//
//        recompute_spawn_points();
//        for (auto &[id, player] : players)
//            spawn(player);
//    } else if (ui::push_button(w / 2, 2 * h / (N + 1), "MainMenu##Host",
//                               ui::Align::Center)) {
//        network.start_server();
//        game_status = GameStatus::HostLobby;
//    } else if (ui::push_button(w / 2, 3 * h / (N + 1), "MainMenu##Connect",
//                               ui::Align::Center)) {
//        network.connect();
//        game_status = GameStatus::GuestLobby;
//    } else if (ui::push_button(w / 2, 4 * h / (N + 1), "MainMenu##Quit",
//                               ui::Align::Center)) {
//        window->close();
//    }
//}
//
void SnakeGame::host_lobby(HostLobby &s, Input &input, float dt) {
    ui::label(5, 0, "Hosting Game");
    if (ui::push_button(250, 0, "HostLobby##Start")) {

    } else if (ui::push_button(400, 0, "HostLobby##Quit")) {
        state.emplace<MainMenu>();
    }

    while (auto id = s.network.get_new_client()) {
        printf("Got new player: %d\n", *id);
        s.add_player(*id);
    }

    for (auto &[id, player] : s.players) {
        player.send_buffer.reset();
    }

    for (auto it = s.players.begin(); it != s.players.end();) {
        const auto id = it->first;
        auto &player = it->second;
        using namespace SnakeNetwork;
        if (id != s.local_id) {

            if (s.network.recv(recv_buffer, id)) {
                Message msg;
                while (!recv_buffer.empty()) {
                    recv_buffer.read(msg);
                    if (std::get_if<HeartBeat>(&msg)) {
                        msg.emplace<HeartBeat>();
                        player.send_buffer.write(msg);
                    } else if (std::get_if<JoinRequest>(&msg)) {
                        msg.emplace<JoinResponse>(id);
                        player.send_buffer.write(msg);

                    } else if (auto m = std::get_if<SetReady>(&msg)) {
                        player.ready = m->ready;
                        bool ready = m->ready;
                        msg = ServerSetReady{ready, id};
                        for (auto &[tid, tplayer] : s.players) {
                            tplayer.send_buffer.write(msg);
                        }
                    }
                }
                if (player.known_ids.size() != s.players.size()) {
                    for (auto &[tid, tplayer] : s.players) {
                        if (std::find(player.known_ids.begin(),
                                      player.known_ids.end(),
                                      tid) == player.known_ids.end()) {
                            msg = NewPlayer{tid, tplayer.ready};
                            player.known_ids.push_back(tid);
                            player.send_buffer.write(msg);
                            printf("Sharing player id %u with %u\n", tid, id);
                        }
                    }
                }
                s.network.send(player.send_buffer, id);
                ++it;
            } else {
                it = s.players.erase(it);
            }
        } else {
            ++it;
        }
    }

    int y = 50;

    for (auto &[id, player] : s.players) {
        ui::labelc(20, y,
                   player.ready ? sf::Color{100, 255, 100, 255}
                                : sf::Color{255, 100, 100, 255},
                   "Player %u : %s", id, player.ready ? "Ready" : "Not Ready");
        y += 40;
    }
}

void SnakeGame::guest_lobby(GuestLobby &s, Input &input, float dt) {
    using namespace SnakeNetwork;
    Message msg;
    Network::Buffer send_buffer;

    ui::label(5, 0, "Lobby");

    if (bool ready; ui::toggle_button(250, 0, "Ready", &ready)) {
        msg = SetReady{ready};
        send_buffer.write(msg);
        printf("Sending %s\n", ready ? "Ready" : "Not Ready");
    }
    if (ui::push_button(400, 0, "GuestLobby##Quit")) {
        state.emplace<MainMenu>();
    }

    if (s.network.connected()) {

        if (s.local_id == 0) {
            msg.emplace<JoinRequest>();
            send_buffer.write(msg);
            printf("Sending JoinRequest\n");
        } else {
            msg.emplace<HeartBeat>();
            send_buffer.write(msg);
        }

        s.network.send(send_buffer);

        if (s.network.recv(recv_buffer)) {
            while (!recv_buffer.empty()) {
                recv_buffer.read(msg);
                if (auto m = std::get_if<JoinResponse>(&msg)) {
                    printf("Received JoinResponse\n");
                    s.add_player(m->id);
                    s.local_id = m->id;
                } else if (auto m = std::get_if<NewPlayer>(&msg)) {
                    printf("Received NewPlayer\n");
                    auto p = s.add_player(m->id);
                    p->ready = m->ready;
                } else if (auto m = std::get_if<ServerSetReady>(&msg)) {
                    printf("Received ServerSetReady\n");
                    auto &p = s.players.at(m->id);
                    p.ready = m->ready;
                }
            }
        } else {
            state.emplace<MainMenu>();
        }
    }
    int y = 50;
    for (auto &[id, player] : s.players) {
        ui::labelc(20, y,
                   player.ready ? sf::Color{100, 255, 100, 255}
                                : sf::Color{255, 100, 100, 255},
                   "Player %u : %s", id, player.ready ? "Ready" : "Not Ready");
        y += 40;
    }
}

//
void SnakeGame::main_menu(MainMenu &s, Input &input, float dt) {
    auto [w, h] = window->getView().getSize();
    const int N = 4;
    if (ui::push_button(w / 2, 1 * h / (N + 1), "MainMenu##Single Player",
                        ui::Align::Center)) {
        state.emplace<SinglePlayer>(*this);
    } else if (ui::push_button(w / 2, 2 * h / (N + 1), "MainMenu##Host",
                               ui::Align::Center)) {
        state.emplace<HostLobby>(*this);
    } else if (ui::push_button(w / 2, 3 * h / (N + 1), "MainMenu##Connect",
                               ui::Align::Center)) {
        state.emplace<GuestLobby>(*this);
    } else if (ui::push_button(w / 2, 4 * h / (N + 1), "MainMenu##Quit",
                               ui::Align::Center)) {
        window->close();
    }
}

SnakeGame::SinglePlayer::SinglePlayer(SnakeGame &game) : game(game) {
    world_map.resize(game.gridCols, game.gridRows);
    add_player();
    recompute_spawn_points();
    for (auto &[id, player] : players)
        spawn(player);
}

void SnakeGame::single_player(SinglePlayer &s, Input &input, float dt) {

    for (auto &ev : input.events) {
        auto &player = s.players.at(s.local_id);
        if (auto e = std::get_if<Input::KeyPressed>(&ev)) {
            if (e->key == sf::Keyboard::Space) {
                player.boost = true;
            } else if (e->key == sf::Keyboard::P) {
                s.paused = !s.paused;
            } else {
                player.input_buffer.push_back(e->key);
                s.paused = false;
            }
        } else if (auto e = std::get_if<Input::KeyReleased>(&ev)) {
            if (e->key == sf::Keyboard::Space) {
                player.boost = false;
            } else {
            }
        } else if (auto e = std::get_if<Input::LostFocus>(&ev)) {
            s.paused = true;
        }
    }

    for (auto &[id, player] : s.players) {
        int div = player.boost ? 0 : 1;
        if (!s.paused && player.moveCounter++ >= player.moveDelay * div) {
            if (!player.input_buffer.empty()) {
                auto k = player.input_buffer.front();
                player.input_buffer.pop_front();
                if (k == sf::Keyboard::Left) {
                    player.dir = set_dir(player, Direction::Left);
                }

                else if (k == sf::Keyboard::Right) {
                    player.dir = set_dir(player, Direction::Right);
                }

                else if (k == sf::Keyboard::Down) {
                    player.dir = set_dir(player, Direction::Down);
                }

                else if (k == sf::Keyboard::Up) {
                    player.dir = set_dir(player, Direction::Up);
                }
            }

            for (int i = player.body.size() - 1; i > 0; --i) {
                player.body[i].x = player.body[i - 1].x;
                player.body[i].y = player.body[i - 1].y;
            }

            if (player.use_ai) {
                //                player.dir =
                //                next_left[static_cast<int>(player.dir)];
                Direction test_dir = player.dir;
                int n = 0;
                bool found = false;
                do {
                    int ty = player.body[0].y;
                    int tx = player.body[0].x;

                    switch (test_dir) {
                    case Direction::Down:
                        ty++;
                        break;
                    case Direction::Up:
                        ty--;
                        break;
                    case Direction::Left:
                        tx--;
                        break;
                    case Direction::Right:
                        tx++;
                        break;
                    }

                    bool collision = false;
                    for (auto &[id_test, player_test] : s.players) {
                        if (on_player(player, player_test, tx, ty)) {
                            collision = true;
                            break;
                        }
                    }

                    if (tx < 0 || tx >= gridCols || ty < 0 || ty >= gridRows ||
                        collision) {
                        test_dir = next_right[static_cast<int>(test_dir)];
                    } else {
                        found = true;
                    }
                } while (!found && ++n < 4);
                player.dir = test_dir;
            }

            switch (player.dir) {
            case Direction::Down:
                player.body[0].y++;
                break;
            case Direction::Up:
                player.body[0].y--;
                break;
            case Direction::Left:
                player.body[0].x--;
                break;
            case Direction::Right:
                player.body[0].x++;
                break;
            }
            player.moveCounter = 0;

            s.foodRegrowCount++;
        }
    }

    for (auto &[id, player] : s.players) {
        if (player.body[0].x < 0 || player.body[0].x >= gridCols ||
            player.body[0].y < 0 || player.body[0].y >= gridRows) {
            add_message("You died! Do no try to go out of the playing field.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            player.dead = true;
        }

        for (auto f : s.food) {
            food_shape.setPosition(f->p.x * gridSize, f->p.y * gridSize);
            window->draw(food_shape);
        }

        int n = 0;
        for (auto [x, y] : player.body) {

            body_shape.setPosition(x * gridSize, y * gridSize);
            auto color = player.color;
            color.a = 255 - n * 64 / player.body.size();
            body_shape.setFillColor(color);
            window->draw(body_shape);
            n++;
        }

        for (auto it = s.food.begin(); it != s.food.end();) {
            auto f = *it;
            if (player.body[0] == f->p) {
                for (int i = 0; i < s.foodGrowth; ++i) {
                    if (player.body.size() < 50) {
                        player.body.push_back(player.body.back());
                    }
                }
                s.world_map((*it)->p.x, (*it)->p.y).food = nullptr;
                it = s.food.erase(it);
                delete f;
            } else {
                ++it;
            }
        }

        if (s.foodRegrowCount >= s.foodRegrow && s.food.size() < 10) {
            s.add_food(rand() % gridCols, rand() % gridRows);
            s.foodRegrowCount = 0;
        }

        bool collision = false;
        for (auto &[id_test, player_test] : s.players) {
            if (on_player(player, player_test)) {
                collision = true;
                break;
            }
        }
        if (collision) {
            add_message("You died! Do no eat snakes.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            player.dead = true;
        }
    }

    for (auto &[id, player] : s.players) {
        if (player.dead) {
            s.decompose(player);
            s.spawn(player);
        }
    }

    if (s.paused) {
        window->draw(pause_text);
    }
}

SnakeGame::Player *SnakeGame::SinglePlayer::add_player() {
    const auto id = unique_player_id++;
    auto &player = players.emplace(id, Player{}).first->second;
    player.spawn_dir = Direction::Up;
    player.color = game.get_random_color();
    player.id = id;

    return &player;
}

SnakeGame::Player *SnakeGame::HostLobby::add_player(Network::ClientID id) {
    auto &player = players.emplace(id, Player{}).first->second;
    player.spawn_dir = Direction::Up;
    player.color = game.get_random_color();
    player.id = id;

    return &player;
}

SnakeGame::HostLobby::HostLobby(SnakeGame &game) : game(game) {
    auto player = add_player(local_id);
    player->ready = true;
    network.start_server();
}

SnakeGame::GuestLobby::GuestLobby(SnakeGame &game) : game(game) {
    network.connect();
}

SnakeGame::Player *SnakeGame::GuestLobby::add_player(Network::ClientID id) {
    auto &player = players.emplace(id, Player{}).first->second;
    player.spawn_dir = Direction::Up;
    player.color = game.get_random_color();
    player.id = id;

    return &player;
}
