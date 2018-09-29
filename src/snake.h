#pragma once
#include "engine.h"
#include "network.h"
#include "stable_win32.hpp"

namespace SnakeNetwork {
struct Message;
}

struct SnakeGame {
    u32 gridSize = 16;
    sf::RectangleShape body_shape;
    sf::RectangleShape food_shape;

    sf::Font hand_font;
    sf::Text pause_text;

    struct Food {
        sf::Vector2i p;
        Food(int x, int y);
    };

    struct Cell {
        Food *food = nullptr;
        void reset();
    };

    enum class Direction { Up, Right, Down, Left };
    static constexpr Direction next_right[4] = {
        Direction::Right, Direction::Down, Direction::Left, Direction::Up};

    static constexpr Direction next_left[4] = {
        Direction::Left, Direction::Up, Direction::Right, Direction::Down};

    struct Player {
        sf::Color color;
        std::vector<sf::Vector2i> body;
        Direction dir;
        std::list<sf::Keyboard::Key> input_buffer;
        bool use_ai = false;
        bool boost = false;
        bool dead = false;

        int moveDelay = 2;
        int moveCounter = 0;

        u32 spawnX;
        u32 spawnY;
        Direction spawn_dir;

        Network::ClientID id;
        std::vector<Network::ClientID> known_ids;

        bool ready = false;

        u32 initialSize = 3;

        Network::Buffer send_buffer;

        // Player(const Player &) = delete;
        void operator=(const Player &) = delete;
    };

    struct MainMenu {};

    using PlayerList = std::unordered_map<Network::ClientID, Player>;
    using WorldMap = Array2D<Cell>;

    std::vector<Food *> food;
    int foodRegrow = 20;
    int foodRegrowCount = 0;

    int foodGrowth = 1;
    WorldMap world_map;

    struct SinglePlayer {
        SinglePlayer(SnakeGame &game);
        void add_food(int x, int y);
        void remove_food(int x, int y);
        void reset_map();
        void decompose(Player &player);
        void spawn(Player &player);
        void recompute_spawn_points();

        Player *add_player();

        Network::ClientID local_id = 0;
        Network::ClientID unique_player_id = 0;

        PlayerList players;

        bool paused = false;
        SnakeGame &game;

        Direction set_dir(Player &player, Direction d);
        void game_tick(Input &input, float dt);
    };

    struct HostLobby {
        HostLobby(SnakeGame &game);
        Network network;
        SnakeGame &game;

        Player *add_player(Network::ClientID id);
        std::unordered_map<Network::ClientID, Player> players;

        const Network::ClientID local_id = 0;
        Network::ClientID unique_player_id = 1;

        void recompute_spawn_points();
        void spawn(Player &player);
        void decompose(Player &player);
        void send_all(SnakeNetwork::Message &msg);
        void add_food(int x, int y);
        void grow_player(Player &player);

        void game_tick(Input &input, float dt);
        Direction set_dir(Player &player, Direction d);

        bool game_running = false;
    };

    struct GuestLobby {
        GuestLobby(SnakeGame &game);
        Network network;
        SnakeGame &game;

        Player *add_player(Network::ClientID id);
        std::unordered_map<Network::ClientID, Player> players;
        std::list<SnakeNetwork::Message> msgs;

        Network::ClientID local_id = 0;
        bool game_running = false;
        Network::Buffer send_buffer;

        void spawn(Player &player);
        void game_tick(Input &input, float dt);
        void add_food(int x, int y);
        void remove_food(int x, int y);
        void grow_player(Player &player);
    };

    using GameState =
        std::variant<MainMenu, SinglePlayer, HostLobby, GuestLobby>;
    GameState state;

    u32 gridRows = 30;
    u32 gridCols = 30;

    Network::Buffer recv_buffer;

    bool on_player(const Player &p1, const Player &p2);
    bool on_player(const Player &p1, const Player &p2, int x, int y);

    void init();

    void update(Input &input, float dt);

    sf::Color get_random_color();

    void main_menu(MainMenu &s, Input &input, float dt);
    void host_lobby(HostLobby &s, Input &input, float dt);
    void guest_lobby(GuestLobby &s, Input &input, float dt);
    void single_player(SinglePlayer &s, Input &input, float);
};

namespace SnakeNetwork {

struct HeartBeat {};

struct JoinRequest {};

struct JoinResponse {
    JoinResponse(Network::ClientID id) : id(id) {}
    const Network::ClientID id;
};

struct SetReady {
    bool ready;
};

struct ServerSetReady {
    bool ready;
    Network::ClientID id;
};

struct NewPlayer {
    Network::ClientID id;
    bool ready;
};

struct PlayerLeft {
    Network::ClientID id;
};

struct SetPlayerInfo {
    Network::ClientID id;
    bool ready;
    u32 spawnX;
    u32 spawnY;
    SnakeGame::Direction spawn_dir;
    sf::Color color;
};

struct StartGame {};

struct SpawnPlayer {
    Network::ClientID id;
};

struct MovePlayer {
    Network::ClientID id;
    SnakeGame::Direction dir;
};

struct SpawnFood {
    int x;
    int y;
};

struct DestroyFood {
    int x;
    int y;
};

struct PlayerGrow {
    Network::ClientID id;
};

struct PlayerInput {
    sf::Keyboard::Key key;
    bool down;
};

struct Message {

    std::variant<HeartBeat, JoinRequest, JoinResponse, SetReady, ServerSetReady,
                 NewPlayer, PlayerLeft, SetPlayerInfo, StartGame, PlayerInput,
                 SpawnPlayer, MovePlayer, SpawnFood, DestroyFood, PlayerGrow>
        body;
};

}; // namespace SnakeNetwork
