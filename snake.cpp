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
}

// leave food behind where the body was
void SnakeGame::decompose(Player &player) {
    for (size_t i = 1; i < player.body.size(); ++i) {
        add_food(player.body[i].x, player.body[i].y);
    }
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

bool SnakeGame::on_player(Player &player, int x, int y) {

    for (size_t i = 1; i < player.body.size(); ++i) {
        if (player.body[i].x == x && player.body[i].y == y) {
            return true;
        }
    }
    return false;
}

void SnakeGame::init() {
    players[local_id] = {};
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

    auto get_color = []() -> sf::Color {
        sf::Color c;
        c.r = rand() % 255;
        c.g = rand() % 255;
        c.b = 255 * 3 - c.r - c.b;
        c.a = 255;
        return c;
    };

    {
        auto &player = players.at(local_id);
        player.spawnX = gridCols / 2;
        player.spawnY = gridRows / 2;
        player.color = get_color();
        spawn(player);
    }
    for (Player::ID id = 1; id < 10; id++) {
        players[id] = {};
        auto &player = players.at(id);
        player.spawnX = rand() % (gridCols - 5) + 5;
        player.spawnY = rand() % (gridRows - 5) + 5;
        player.use_ai = true;
        player.spawn_dir = static_cast<Direction>(rand() % 4);
        player.color = get_color();
        spawn(player);
    }

    //    {
    //        players[2] = {};
    //        auto &player = players.at(2);
    //        player.spawnX = gridCols - 10;
    //        player.spawnY = gridRows - 20;
    //        player.use_ai = true;
    //        player.color = get_color();
    //        spawn(player);
    //    }

    add_message("Started Snake game with grid: %dx%d (%d)", gridCols, gridRows,
                gridSize);
}

void SnakeGame::input_key(sf::Keyboard::Key k, bool down) {
    if (k == sf::Keyboard::P && down) {
        paused = !paused;
    } else if (k == sf::Keyboard::Space && down) {
        boost = true;
    } else if (k == sf::Keyboard::Space && !down) {
        boost = false;
    } else if (down) {
        paused = false;
        auto &player = players.at(local_id);
        player.input_buffer.emplace_back(k);
    }
}

void SnakeGame::update(float dt) {

    network.update();

    int div = boost ? 0 : 1;
    if (!paused && moveCounter++ >= moveDelay * div) {
        for (auto &[id, player] : players) {
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
                        if (on_player(player_test, tx, ty)) {
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
            moveCounter = 0;

            foodRegrowCount++;
        }
    }

    for (auto &[id, player] : players) {
        if (player.body[0].x < 0 || player.body[0].x >= gridCols ||
            player.body[0].y < 0 || player.body[0].y >= gridRows) {
            add_message("You died! Do no try to go out of the playing field.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            decompose(player);
            spawn(player);
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
            if (on_player(player_test, player.body[0].x, player.body[0].y)) {
                collision = true;
                break;
            }
        }
        if (collision) {
            add_message("You died! Do no eat snakes.\n"
                        "Final score: %d",
                        player.body.size() - 3);
            decompose(player);
            spawn(player);
        }
    }

    if (paused) {
        window->draw(pause_text);
    }

    if (ui::push_button(0, 0, "Server")) {
        network.start_server();
    }
    if (ui::push_button(0, 25, "Client")) {
        network.connect();
    }

    auto &player = players.at(local_id);
    ui::label(200, 5, "Score: %4d", player.body.size() - initialSize);

    player.use_ai = ui::toggle_button(0, 50, "Snake AI");
}
