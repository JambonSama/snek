#include "snake.h"
#include "engine.h"
#include "stable_win32.hpp"

SnakeGame::Food::Food(int x, int y) : p(x, y) {}
void SnakeGame::Cell::reset() { food = nullptr; }

SnakeGame::Direction SnakeGame::SinglePlayer::set_dir(Player &player,
                                                      Direction d) {
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
    if (game.world_map(x, y).food == nullptr) {
        auto f = new Food{x, y};
        game.world_map(x, y).food = f;
        game.food.push_back(f);
    }
}

void SnakeGame::SinglePlayer::remove_food(int x, int y) {
    auto f = game.world_map(x, y).food;
    if (f != nullptr) {
        std::remove_if(game.food.begin(), game.food.end(),
                       [f](auto food_p) { return food_p == f; });
        delete f;
    }
    game.world_map(x, y).food = nullptr;
}

void SnakeGame::SinglePlayer::reset_map() {
    for (auto f : game.food) {
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

void SnakeGame::host_lobby(HostLobby &s, Input &input, float dt) {
    for (auto &[id, player] : s.players) {
        player.send_buffer.reset();
    }

    if (!s.game_running) {
        ui::label(5, 0, "Hosting Game");
        if (ui::push_button(250, 0, "HostLobby##Start")) {
            using namespace SnakeNetwork;
            s.recompute_spawn_points();
            for (auto &[id, player] : s.players) {
                Message msg;
                msg.body = SetPlayerInfo{id,
                                         player.ready,
                                         player.spawnX,
                                         player.spawnY,
                                         player.spawn_dir,
                                         player.color};
                s.send_all(msg);
            }

            for (auto &[id, player] : s.players) {
                s.spawn(player);
            }

            Message msg;
            msg.body = StartGame{};
            s.send_all(msg);
            s.game_running = true;

        } else if (ui::push_button(400, 0, "HostLobby##Quit")) {
            state.emplace<MainMenu>();
        }

        while (auto id = s.network.get_new_client()) {
            printf("Got new player: %d\n", *id);
            s.add_player(*id);
        }
    }

    for (auto it = s.players.begin(); it != s.players.end();) {
        const auto id = it->first;
        auto &player = it->second;
        using namespace SnakeNetwork;
        if (id != s.local_id) {

            printf("Receiving ... ");
            if (s.network.recv(recv_buffer, id)) {
                printf("%u bytes\n", recv_buffer.bytes.size());
                Message msg;
                while (!recv_buffer.empty()) {
                    recv_buffer.read(msg);
                    if (std::get_if<HeartBeat>(&msg.body)) {
                        msg.body.emplace<HeartBeat>();
                        player.send_buffer.write(msg);
                    } else if (std::get_if<JoinRequest>(&msg.body)) {
                        msg.body.emplace<JoinResponse>(id);
                        player.send_buffer.write(msg);
                        for (auto &[tid, tplayer] : s.players) {
                            msg.body = NewPlayer{tid, tplayer.ready};
                            player.send_buffer.write(msg);
                            printf("Sharing player %u with %u\n", tid, id);
                        }

                        for (auto &[tid, tplayer] : s.players) {
                            if (tid != id && tid != s.local_id) {
                                msg.body = NewPlayer{id, player.ready};
                                tplayer.send_buffer.write(msg);
                                printf("Sharing player %u with %u\n", id, tid);
                            }
                        }

                    } else if (auto m = std::get_if<SetReady>(&msg.body)) {
                        player.ready = m->ready;
                        msg.body = ServerSetReady{player.ready, id};
                        for (auto &[tid, tplayer] : s.players) {
                            tplayer.send_buffer.write(msg);
                        }
                    } else if (auto m = std::get_if<PlayerInput>(&msg.body)) {
                        if (m->down) {
                            player.input_buffer.push_back(m->key);
                        }
                    }
                }

                ++it;
            } else {
                auto erase_id = it->first;
                it = s.players.erase(it);
                Message msg;
                msg.body = PlayerLeft{erase_id};
                for (auto &[tid, tplayer] : s.players) {
                    tplayer.send_buffer.write(msg);
                }
            }
        } else {
            ++it;
        }
    }

    if (s.game_running) {
        s.game_tick(input, dt);
    }

    for (auto &[id, player] : s.players) {
        if (id == s.local_id)
            continue;
        printf("sending %u bytes to %u\n", player.send_buffer.bytes.size(), id);
        s.network.send(player.send_buffer, id);
    }

    int y = 50;

    if (!s.game_running) {
        for (auto &[id, player] : s.players) {
            ui::labelc(20, y,
                       player.ready ? sf::Color{100, 255, 100, 255}
                                    : sf::Color{255, 100, 100, 255},
                       "Player %u : %s", id,
                       player.ready ? "Ready" : "Not Ready");
            y += 40;
        }
    }
}

void SnakeGame::guest_lobby(GuestLobby &s, Input &input, float dt) {
    using namespace SnakeNetwork;
    Message msg;
    s.send_buffer.reset();

    if (!s.game_running) {
        ui::label(5, 0, "Lobby");

        if (bool ready; ui::toggle_button(250, 0, "Ready", &ready)) {
            msg.body = SetReady{ready};
            s.send_buffer.write(msg);
            //        printf("Sending %s\n", ready ? "Ready" : "Not Ready");
        }
        if (ui::push_button(400, 0, "GuestLobby##Quit")) {
            state.emplace<MainMenu>();
        }
    }

    if (s.network.connected()) {

        if (s.game_running) {
            s.game_tick(input, dt);
        } else {
            if (s.local_id == 0) {
                msg.body.emplace<JoinRequest>();
                s.send_buffer.write(msg);
                printf("Sending JoinRequest\n");
            } else {
                msg.body.emplace<HeartBeat>();
                s.send_buffer.write(msg);
            }

            s.network.send(s.send_buffer);

            if (s.network.recv(recv_buffer)) {
                while (!recv_buffer.empty()) {
                    recv_buffer.read(msg);
                    if (auto m = std::get_if<JoinResponse>(&msg.body)) {
                        printf("Received JoinResponse\n");
                        s.add_player(m->id);
                        s.local_id = m->id;
                    } else if (auto m = std::get_if<NewPlayer>(&msg.body)) {
                        printf("Received NewPlayer\n");
                        auto p = s.add_player(m->id);
                        p->ready = m->ready;
                    } else if (auto m =
                                   std::get_if<ServerSetReady>(&msg.body)) {
                        printf("Received ServerSetReady (v=%d id=%d)\n",
                               m->ready, m->id);
                        auto &p = s.players.at(m->id);
                        p.ready = m->ready;
                    } else if (auto m = std::get_if<PlayerLeft>(&msg.body)) {
                        s.players.erase(m->id);
                    } else if (auto m = std::get_if<SetPlayerInfo>(&msg.body)) {
                        auto &p = s.players.at(m->id);
                        p.spawnX = m->spawnX;
                        p.spawnY = m->spawnY;
                        p.spawn_dir = m->spawn_dir;
                        p.color = m->color;
                        p.ready = m->ready;
                    } else if (auto m = std::get_if<StartGame>(&msg.body)) {
                        s.game_running = true;
                    } else if (auto m = std::get_if<SpawnPlayer>(&msg.body)) {
                        auto &p = s.players.at(m->id);
                        s.spawn(p);
                    }
                }
            } else {
                state.emplace<MainMenu>();
            }
        }
    }

    if (!s.game_running) {

        int y = 50;
        for (auto &[id, player] : s.players) {
            ui::labelc(20, y,
                       player.ready ? sf::Color{100, 255, 100, 255}
                                    : sf::Color{255, 100, 100, 255},
                       "Player %u : %s", id,
                       player.ready ? "Ready" : "Not Ready");
            y += 40;
        }
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
    game.world_map.resize(game.gridCols, game.gridRows);
    add_player();
    for (int i = 0; i < 5; i++) {
        auto p = add_player();
        p->use_ai = true;
    }
    recompute_spawn_points();
    for (auto &[id, player] : players)
        spawn(player);
}

void SnakeGame::single_player(SinglePlayer &s, Input &input, float dt) {
    s.game_tick(input, dt);
}

SnakeGame::Player *SnakeGame::SinglePlayer::add_player() {
    const auto id = unique_player_id++;
    auto &player = players.emplace(id, Player{}).first->second;
    player.spawn_dir = Direction::Up;
    player.color = game.get_random_color();
    player.id = id;

    return &player;
}

void SnakeGame::SinglePlayer::game_tick(Input &input, float dt) {
    for (auto &ev : input.events) {
        auto &player = players.at(local_id);
        if (auto e = std::get_if<Input::KeyPressed>(&ev)) {
            if (e->key == sf::Keyboard::Space) {
                player.boost = true;
            } else if (e->key == sf::Keyboard::P) {
                paused = !paused;
            } else {
                player.input_buffer.push_back(e->key);
                paused = false;
            }
        } else if (auto e = std::get_if<Input::KeyReleased>(&ev)) {
            if (e->key == sf::Keyboard::Space) {
                player.boost = false;
            } else {
            }
        } else if (auto e = std::get_if<Input::LostFocus>(&ev)) {
            paused = true;
        }
    }

    for (auto &[id, player] : players) {
        int div = player.boost ? 0 : 1;
        if (!paused && player.moveCounter++ >= player.moveDelay * div) {
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
                    for (auto &[id_test, player_test] : players) {
                        if (game.on_player(player, player_test, tx, ty)) {
                            collision = true;
                            break;
                        }
                    }

                    if (tx < 0 || tx >= game.gridCols || ty < 0 ||
                        ty >= game.gridRows || collision) {
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

            game.foodRegrowCount++;
        }
    }

    for (auto &[id, player] : players) {
        if (player.body[0].x < 0 || player.body[0].x >= game.gridCols ||
            player.body[0].y < 0 || player.body[0].y >= game.gridRows) {
            add_message("You died! Do no try to go out of the playing field.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            player.dead = true;
        }

        for (auto f : game.food) {
            game.food_shape.setPosition(f->p.x * game.gridSize,
                                        f->p.y * game.gridSize);
            window->draw(game.food_shape);
        }

        int n = 0;
        for (auto [x, y] : player.body) {

            game.body_shape.setPosition(x * game.gridSize, y * game.gridSize);
            auto color = player.color;
            color.a = 255 - n * 64 / player.body.size();
            game.body_shape.setFillColor(color);
            if (id == local_id) {
                game.body_shape.setOutlineColor({255, 255, 255, 255});
                game.body_shape.setOutlineThickness(1);
            } else {
                game.body_shape.setOutlineColor({0, 0, 0, 0});
                game.body_shape.setOutlineThickness(0);
            }
            window->draw(game.body_shape);
            n++;
        }

        for (auto it = game.food.begin(); it != game.food.end();) {
            auto f = *it;
            if (player.body[0] == f->p) {
                for (int i = 0; i < game.foodGrowth; ++i) {
                    if (player.body.size() < 50) {
                        player.body.push_back(player.body.back());
                    }
                }
                game.world_map((*it)->p.x, (*it)->p.y).food = nullptr;
                it = game.food.erase(it);
                delete f;
            } else {
                ++it;
            }
        }

        if (game.foodRegrowCount >= game.foodRegrow && game.food.size() < 10) {
            add_food(rand() % game.gridCols, rand() % game.gridRows);
            game.foodRegrowCount = 0;
        }

        bool collision = false;
        for (auto &[id_test, player_test] : players) {
            if (game.on_player(player, player_test)) {
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

    for (auto &[id, player] : players) {
        if (player.dead) {
            decompose(player);
            spawn(player);
        }
    }

    if (paused) {
        window->draw(game.pause_text);
    }
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
    game.world_map.resize(game.gridCols, game.gridRows);
    network.start_server();
}

void SnakeGame::HostLobby::recompute_spawn_points() {
    auto n = players.size();
    u32 i = 0;
    for (auto &[id, p] : players) {
        p.spawnY = game.gridRows - 10;
        p.spawnX = game.gridCols * (i + 1) / (n + 1);
        // add_message("spawn pos: %d %d", p.spawnX, p.spawnY);
        ++i;
    }
}

void SnakeGame::HostLobby::spawn(SnakeGame::Player &player) {
    player.body.resize(player.initialSize);
    player.body[0] = {static_cast<int>(player.spawnX),
                      static_cast<int>(player.spawnY)};

    for (size_t i = 1; i < player.body.size(); ++i) {
        player.body[i].x = player.body[i - 1].x;
        player.body[i].y = player.body[i - 1].y + 1;
    }

    player.dir = player.spawn_dir;
    player.dead = false;

    using namespace SnakeNetwork;
    Message msg;
    msg.body = SpawnPlayer{player.id};
    send_all(msg);
}

void SnakeGame::HostLobby::decompose(Player &player) {
    for (size_t i = 1; i < player.body.size(); ++i) {
        add_food(player.body[i].x, player.body[i].y);
    }

    player.input_buffer.clear();
}

void SnakeGame::HostLobby::send_all(SnakeNetwork::Message &msg) {
    for (auto &[id, player] : players) {
        if (id == local_id)
            continue;
        player.send_buffer.write(msg);
    }
}

void SnakeGame::HostLobby::add_food(int x, int y) {
    if (game.world_map(x, y).food == nullptr) {
        auto f = new Food{x, y};
        game.world_map(x, y).food = f;
        game.food.push_back(f);
    }
}

SnakeGame::GuestLobby::GuestLobby(SnakeGame &game) : game(game) {
    game.world_map.resize(game.gridCols, game.gridRows);
    network.connect();
}

SnakeGame::Player *SnakeGame::GuestLobby::add_player(Network::ClientID id) {
    if (auto it = players.find(id); it != players.end()) {
        return &it->second;
    } else {
        auto &player = players.emplace(id, Player{}).first->second;
        player.spawn_dir = Direction::Up;
        player.color = game.get_random_color();
        player.id = id;

        return &player;
    }
}

void SnakeGame::GuestLobby::spawn(SnakeGame::Player &player) {
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

void SnakeGame::HostLobby::game_tick(Input &input, float dt) {
    using namespace SnakeNetwork;
    Message msg;

    for (auto &ev : input.events) {
        auto &player = players.at(local_id);
        if (auto e = std::get_if<Input::KeyPressed>(&ev)) {
            player.input_buffer.push_back(e->key);
        } else if (auto e = std::get_if<Input::KeyReleased>(&ev)) {
        }
    }

    for (auto &[id, player] : players) {
        int div = player.boost ? 0 : 1;
        if (player.moveCounter++ >= player.moveDelay * div) {
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
            msg.body = MovePlayer{player.id, player.dir};
            send_all(msg);
            player.moveCounter = 0;
        }
    }

    for (auto &[id, player] : players) {
        if (player.body[0].x < 0 || player.body[0].x >= game.gridCols ||
            player.body[0].y < 0 || player.body[0].y >= game.gridRows) {
            //            add_message("You died! Do no try to go out of the
            //            playingfield.\n "
            //                        "Final score: %d",
            //                        player.body.size() - 3);
            player.dead = true;
        }

        for (auto f : game.food) {
            game.food_shape.setPosition(f->p.x * game.gridSize,
                                        f->p.y * game.gridSize);
            window->draw(game.food_shape);
        }

        int n = 0;
        for (auto [x, y] : player.body) {

            game.body_shape.setPosition(x * game.gridSize, y * game.gridSize);
            auto color = player.color;
            color.a = 255 - n * 64 / player.body.size();
            game.body_shape.setFillColor(color);
            if (id == local_id) {
                game.body_shape.setOutlineColor({255, 255, 255, 255});
                game.body_shape.setOutlineThickness(2);
            } else {
                game.body_shape.setOutlineColor({0, 0, 0, 0});
                game.body_shape.setOutlineThickness(0);
            }
            window->draw(game.body_shape);
            n++;
        }

        //        for (auto it = s.food.begin(); it != s.food.end();) {
        //            auto f = *it;
        //            if (player.body[0] == f->p) {
        //                for (int i = 0; i < s.foodGrowth; ++i) {
        //                    if (player.body.size() < 50) {
        //                        player.body.push_back(player.body.back());
        //                    }
        //                }
        //                s.world_map((*it)->p.x, (*it)->p.y).food = nullptr;
        //                it = s.food.erase(it);
        //                delete f;
        //            } else {
        //                ++it;
        //            }
        //        }

        //        if (s.foodRegrowCount >= s.foodRegrow && s.food.size() < 10) {
        //            s.add_food(rand() % gridCols, rand() % gridRows);
        //            s.foodRegrowCount = 0;
        //        }

        //        bool collision = false;
        //        for (auto &[id_test, player_test] : s.players) {
        //            if (on_player(player, player_test)) {
        //                collision = true;
        //                break;
        //            }
        //        }
        //        if (collision) {
        //            add_message("You died! Do no eat snakes.\n"
        //                        "Final score: %d",
        //                        player.body.size() - 3);
        //            player.dead = true;
        //        }
    }

    for (auto &[id, player] : players) {
        if (player.dead) {
            decompose(player);
            spawn(player);
        }
    }

    //    if (s.paused) {
    //        window->draw(pause_text);
    //    }
}

SnakeGame::Direction SnakeGame::HostLobby::set_dir(SnakeGame::Player &player,
                                                   SnakeGame::Direction d) {
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

void SnakeGame::GuestLobby::game_tick(Input &input, float dt) {
    using namespace SnakeNetwork;
    Message msg;
    printf("%d input events\n", input.events.size());
    for (auto &ev : input.events) {
        auto &player = players.at(local_id);
        if (auto e = std::get_if<Input::KeyPressed>(&ev)) {
            msg.body = PlayerInput{e->key, true};
            // send_buffer.write(msg);
        } else if (auto e = std::get_if<Input::KeyReleased>(&ev)) {
            msg.body = PlayerInput{e->key, false};
            // send_buffer.write(msg);
        }
    }

    if (send_buffer.empty()) {
        msg.body.emplace<HeartBeat>();
        send_buffer.write(msg);
    }
    printf("Sending %d bytes\n", send_buffer.bytes.size());
    network.send(send_buffer);

    printf("Receiving ... ");
    if (network.recv(game.recv_buffer)) {
        printf("%d bytes\n", game.recv_buffer.bytes.size());
        while (!game.recv_buffer.empty()) {
            game.recv_buffer.read(msg);
            if (auto m = std::get_if<MovePlayer>(&msg.body)) {
                auto &player = players.at(m->id);
                player.dir = m->dir;
                for (int i = player.body.size() - 1; i > 0; --i) {
                    player.body[i].x = player.body[i - 1].x;
                    player.body[i].y = player.body[i - 1].y;
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
            } else if (auto m = std::get_if<KillPlayer>(&msg.body)) {
                auto &player = players.at(m->id);
                decompose(player);
            }
        }
    }

    //    for (auto &[id, player] : s.players) {
    //        int div = player.boost ? 0 : 1;
    //        if (!s.paused && player.moveCounter++ >= player.moveDelay * div) {
    //            if (!player.input_buffer.empty()) {
    //                auto k = player.input_buffer.front();
    //                player.input_buffer.pop_front();
    //                if (k == sf::Keyboard::Left) {
    //                    player.dir = set_dir(player, Direction::Left);
    //                }

    //                else if (k == sf::Keyboard::Right) {
    //                    player.dir = set_dir(player, Direction::Right);
    //                }

    //                else if (k == sf::Keyboard::Down) {
    //                    player.dir = set_dir(player, Direction::Down);
    //                }

    //                else if (k == sf::Keyboard::Up) {
    //                    player.dir = set_dir(player, Direction::Up);
    //                }
    //            }

    //            for (int i = player.body.size() - 1; i > 0; --i) {
    //                player.body[i].x = player.body[i - 1].x;
    //                player.body[i].y = player.body[i - 1].y;
    //            }

    //            if (player.use_ai) {
    //                //                player.dir =
    //                //                next_left[static_cast<int>(player.dir)];
    //                Direction test_dir = player.dir;
    //                int n = 0;
    //                bool found = false;
    //                do {
    //                    int ty = player.body[0].y;
    //                    int tx = player.body[0].x;

    //                    switch (test_dir) {
    //                    case Direction::Down:
    //                        ty++;
    //                        break;
    //                    case Direction::Up:
    //                        ty--;
    //                        break;
    //                    case Direction::Left:
    //                        tx--;
    //                        break;
    //                    case Direction::Right:
    //                        tx++;
    //                        break;
    //                    }

    //                    bool collision = false;
    //                    for (auto &[id_test, player_test] : s.players) {
    //                        if (on_player(player, player_test, tx, ty)) {
    //                            collision = true;
    //                            break;
    //                        }
    //                    }

    //                    if (tx < 0 || tx >= gridCols || ty < 0 || ty >=
    //                    gridRows ||
    //                        collision) {
    //                        test_dir = next_right[static_cast<int>(test_dir)];
    //                    } else {
    //                        found = true;
    //                    }
    //                } while (!found && ++n < 4);
    //                player.dir = test_dir;
    //            }

    //            switch (player.dir) {
    //            case Direction::Down:
    //                player.body[0].y++;
    //                break;
    //            case Direction::Up:
    //                player.body[0].y--;
    //                break;
    //            case Direction::Left:
    //                player.body[0].x--;
    //                break;
    //            case Direction::Right:
    //                player.body[0].x++;
    //                break;
    //            }
    //            player.moveCounter = 0;

    //            s.foodRegrowCount++;
    //        }
    //    }

    for (auto &[id, player] : players) {
        //        if (player.body[0].x < 0 || player.body[0].x >= gridCols ||
        //            player.body[0].y < 0 || player.body[0].y >= gridRows) {
        //            add_message("You died! Do no try to go out of the playing
        //            field.\n"
        //                        "Final score: %d",
        //                        player.body.size() - 3);
        //            player.dead = true;
        //        }

        //        for (auto f : s.food) {
        //            food_shape.setPosition(f->p.x * gridSize, f->p.y *
        //            gridSize); window->draw(food_shape);
        //        }

        int n = 0;
        for (auto [x, y] : player.body) {

            game.body_shape.setPosition(x * game.gridSize, y * game.gridSize);
            auto color = player.color;
            color.a = 255 - n * 64 / player.body.size();
            game.body_shape.setFillColor(color);
            if (id == local_id) {
                game.body_shape.setOutlineColor({255, 255, 255, 255});
                game.body_shape.setOutlineThickness(2);
            } else {
                game.body_shape.setOutlineColor({0, 0, 0, 0});
                game.body_shape.setOutlineThickness(0);
            }
            window->draw(game.body_shape);
            n++;
        }

        //        for (auto it = s.food.begin(); it != s.food.end();) {
        //            auto f = *it;
        //            if (player.body[0] == f->p) {
        //                for (int i = 0; i < s.foodGrowth; ++i) {
        //                    if (player.body.size() < 50) {
        //                        player.body.push_back(player.body.back());
        //                    }
        //                }
        //                s.world_map((*it)->p.x, (*it)->p.y).food = nullptr;
        //                it = s.food.erase(it);
        //                delete f;
        //            } else {
        //                ++it;
        //            }
        //        }

        //        if (s.foodRegrowCount >= s.foodRegrow && s.food.size() < 10) {
        //            s.add_food(rand() % gridCols, rand() % gridRows);
        //            s.foodRegrowCount = 0;
        //        }

        //        bool collision = false;
        //        for (auto &[id_test, player_test] : s.players) {
        //            if (on_player(player, player_test)) {
        //                collision = true;
        //                break;
        //            }
        //        }
        //        if (collision) {
        //            add_message("You died! Do no eat snakes.\n"
        //                        "Final score: %d",
        //                        player.body.size() - 3);
        //            player.dead = true;
        //        }
    }

    //    for (auto &[id, player] : s.players) {
    //        if (player.dead) {
    //            s.decompose(player);
    //            s.spawn(player);
    //        }
    //    }

    //    if (s.paused) {
    //        window->draw(pause_text);
    //    }
}

void SnakeGame::GuestLobby::decompose(SnakeGame::Player &player) {
    for (size_t i = 1; i < player.body.size(); ++i) {
        add_food(player.body[i].x, player.body[i].y);
    }

    player.input_buffer.clear();
}

void SnakeGame::GuestLobby::add_food(int x, int y) {
    if (game.world_map(x, y).food == nullptr) {
        auto f = new Food{x, y};
        game.world_map(x, y).food = f;
        game.food.push_back(f);
    }
}
