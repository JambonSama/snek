#pragma once
#include "engine.h"
#include "network.h"
#include "stable_win32.hpp"


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

		using ID = uint8_t;
		ID id;

		u32 initialSize = 3;
	};

    struct MainMenu {
       
    };

	using PlayerList = std::unordered_map<Player::ID, Player>;
	using WorldMap = Array2D<Cell>;

    struct SinglePlayer {
		SinglePlayer(SnakeGame& game);
		void add_food(int x, int y);
		void remove_food(int x, int y);
		void reset_map();
		void decompose(Player& player);
		void spawn(Player& player);
		void recompute_spawn_points();

		Player* add_player();

		Player::ID local_id = 0;
		Player::ID unique_player_id = 0;

		PlayerList players;
		std::vector<Food *> food;
		int foodRegrow = 20;
		int foodRegrowCount = 0;

		int foodGrowth = 1;
		WorldMap world_map;
		

		bool paused = false;
		SnakeGame& game;
    };

	struct HostLobby {
		HostLobby(SnakeGame& game);
		Network network;
		SnakeGame& game;

		Player* add_player(Network::ClientID id);
		std::unordered_map<Network::ClientID, Player> players;

		Player::ID local_id = 0;
		Player::ID unique_player_id = 1;
	};

	struct GuestLobby {
		GuestLobby(SnakeGame& game);
		Network network;
		SnakeGame& game;

		struct None {};
		struct WaitingForId {};
		struct Ready {
			Ready(Player::ID id) : id(id) {}
			const Player::ID id;
		};
		std::variant<None, WaitingForId, Ready> state;
	};

    using GameState = std::variant<MainMenu, SinglePlayer, HostLobby, GuestLobby>;
    GameState state;
	
    struct Message {
        enum class Type: u8 { RequestJoin, SetNewPlayer };

        struct HeartBeat {};

        struct RequestJoin {};

        struct SetNewPlayer {
            Player::ID new_id;
        };

        struct PlayerJoined {
            Player::ID new_id;
            sf::Color color;
        };

        Type header;

		union {
			RequestJoin request_join;
			SetNewPlayer set_new_player;
		} body;
    };


    u32 gridRows = 30;
    u32 gridCols = 30;

	Network::Buffer buffer;
    Direction set_dir(Player &player, Direction d);

    bool on_player(const Player &p1, const Player &p2);
    bool on_player(const Player &p1, const Player &p2, int x, int y);

    void init();

    void update(Input &input, float dt);

     sf::Color get_random_color();

     void main_menu(MainMenu& s, Input &input, float dt);
     void host_lobby(HostLobby& s, Input &input, float dt);
     void guest_lobby(GuestLobby& s, Input &input, float dt);
     void single_player(SinglePlayer& s, Input &input, float);
};
