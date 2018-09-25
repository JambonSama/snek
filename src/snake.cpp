#include "stable.hpp"
#include "snake.h"
#include "main.h"

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

SnakeGame::Player::ID SnakeGame::add_player() {
    Player::ID id = unique_player_id++;
    players.insert({id, {}});
    auto &player = players.at(id);
    player.use_ai = true;
    player.spawn_dir = Direction::Up;
    player.color = get_random_color();
    player.id = id;

    ++unique_player_id;
    return id;
}

void SnakeGame::add_player(SnakeGame::Player::ID id) {
    players.insert({id, {}});
    auto &player = players.at(id);
    player.use_ai = true;
    player.spawn_dir = Direction::Up;
    player.color = get_random_color();
    player.id = id;
}

void SnakeGame::recompute_spawn_points() {
    auto n = players.size();
    u32 i = 0;
    for (auto &p : players) {
        p.second.spawnY = gridRows - 10;
        p.second.spawnX = gridCols * i / n;
        add_message("spawn pos: %d %d", p.second.spawnX, p.second.spawnY);
        ++i;
    }
}

void SnakeGame::spawn(Player &player) {
    //    paused = true;
    player.body.resize(initialSize);
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
void SnakeGame::decompose(Player &player) {
    for (size_t i = 1; i < player.body.size(); ++i) {
        add_food(player.body[i].x, player.body[i].y);
    }

	player.input_buffer.clear();
}

void SnakeGame::add_food(int x, int y) {
    if (world_map(x, y).food == nullptr) {
        auto f = new Food{x, y};
        world_map(x, y).food = f;
        food.push_back(f);
    }
}

void SnakeGame::remove_food(int x, int y) {
    auto f = world_map(x, y).food;
    if (f != nullptr) {
        std::remove_if(food.begin(), food.end(),
                       [f](auto food_p) { return food_p == f; });
        delete f;
    }
    world_map(x, y).food = nullptr;
}

void SnakeGame::reset_map() {
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

    world_map.resize(gridCols, gridRows);
    food.clear();
    add_food(rand() % gridCols, rand() % gridRows);

    body_shape.setSize(sf::Vector2f(gridSize, gridSize));
    body_shape.setFillColor({255, 0, 0, 255});

    food_shape.setSize(sf::Vector2f(gridSize / 2, gridSize / 2));
    food_shape.setFillColor({0, 255, 0, 255});
    food_shape.setOrigin(-gridSize / 4, -gridSize / 4);

    game_status = GameStatus::MainMenu;
}

//void SnakeGame::input_key_(sf::Keyboard::Key k, bool down) {
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

void SnakeGame::update(Input& input, float dt) {

    switch (game_status) {
    case GameStatus::MainMenu: {
        main_menu(input, dt);
    } break;
    case GameStatus::SinglePlayer: {
        single_player(input, dt);
    } break;
    case GameStatus::HostLobby: {
        host_lobby(input, dt);
    } break;
    case GameStatus::GuestLobby: {
        guest_lobby(input, dt);
    } break;
    case GameStatus::HostMultiPlayer: {
		game_status = GameStatus::MainMenu;
    } break;
    case GameStatus::GuestMultiPlayer: {
		game_status = GameStatus::MainMenu;
    } break;
    }
}

sf::Color SnakeGame::get_random_color() {
    sf::Color c;
    c.r = rand() % 255;
    c.g = rand() % 255;
    c.b = 255 * 3 - c.r - c.b;
    c.a = 255;
    return c;
}

void SnakeGame::main_menu(Input& input, float dt) {
    auto [w, h] = window->getView().getSize();
    const int N = 4;
    if (ui::push_button(w / 2, 1 * h / (N + 1), "Single Player",
                        ui::Align::Center)) {
        players.clear();
        game_status = GameStatus::SinglePlayer;
        add_player(local_id);
        /*for (int i = 0; i < 20; i++)
            add_player();*/
        auto &player = players.at(local_id);
        player.use_ai = false;

        recompute_spawn_points();
        for (auto &[id, player] : players)
            spawn(player);
    } else if (ui::push_button(w / 2, 2 * h / (N + 1), "Host",
                               ui::Align::Center)) {
        network.start_server();
        game_status = GameStatus::HostLobby;
    } else if (ui::push_button(w / 2, 3 * h / (N + 1), "Connect",
                               ui::Align::Center)) {
        network.connect();
        game_status = GameStatus::GuestLobby;
    } else if (ui::push_button(w / 2, 4 * h / (N + 1), "Quit",
                               ui::Align::Center)) {
        window->close();
    }
}

void SnakeGame::host_lobby(Input& input, float dt) {
    ui::label(0, 0, "Hosting Game");
    if (ui::push_button(250, 0, "Start")) {
        game_status = GameStatus::HostMultiPlayer;
    } else if (ui::push_button(400, 0, "Quit")) {
        network.stop_server();
        game_status = GameStatus::MainMenu;
    }
}

void SnakeGame::guest_lobby(Input& input, float dt) {
    ui::label(0, 0, "Lobby");
    if (ui::push_button(200, 0, "Quit")) {
        game_status = GameStatus::MainMenu;
    }
}

void SnakeGame::single_player(Input& input, float dt) {
    network.update();

	for (auto ev : input.events) {
		auto& player = players.at(local_id);
		if (auto e = std::get_if<Input::KeyPressed>(&ev); e) {
			if (e->key == sf::Keyboard::Space) {
				player.boost = true;
			}
			else if (e->key == sf::Keyboard::P) {
				paused = !paused;
			}
			else {
				player.input_buffer.push_back(e->key);
				paused = false;
			}
		}
		else if (auto e = std::get_if<Input::KeyReleased>(&ev); e) {
			if (e->key == sf::Keyboard::Space) {
				player.boost = false;
			}
			else {
				
			}
		}
		else if (auto e = std::get_if<Input::LostFocus>(&ev); e) {
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

            foodRegrowCount++;
        }
    }

    for (auto &[id, player] : players) {
        if (player.body[0].x < 0 || player.body[0].x >= gridCols ||
            player.body[0].y < 0 || player.body[0].y >= gridRows) {
            add_message("You died! Do no try to go out of the playing field.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            player.dead = true;
        }

        for (auto f : food) {
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

        for (auto it = food.begin(); it != food.end();) {
            auto f = *it;
            if (player.body[0] == f->p) {
                for (int i = 0; i < foodGrowth; ++i) {
                    if (player.body.size() < 50) {
                        player.body.push_back(player.body.back());
                    }
                }
                world_map((*it)->p.x, (*it)->p.y).food = nullptr;
                it = food.erase(it);
                delete f;
            } else {
                ++it;
            }
        }

        if (foodRegrowCount >= foodRegrow && food.size() < 10) {
            add_food(rand() % gridCols, rand() % gridRows);
            foodRegrowCount = 0;
        }

        bool collision = false;
        for (auto &[id_test, player_test] : players) {
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

    for (auto &[id, player] : players) {
        if (player.dead) {
            decompose(player);
            spawn(player);
        }
    }

    if (paused) {
        window->draw(pause_text);
    }

    //    if (ui::push_button(0, 0, "Server")) {
    //        network.start_server();
    //    }
    //    if (ui::push_button(0, 25, "Client")) {
    //        network.connect();
    //    }

    //    auto &player = players.at(local_id);
    //    ui::label(200, 5, "Score: %4d", player.body.size() - initialSize);

    //    if(players.size() > 0)
    //    player.use_ai = ui::toggle_button(0, 50, "Snake AI");
}
