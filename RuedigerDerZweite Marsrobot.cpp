#include <iostream> 
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <array>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <queue>
#include <random>

#define ROBOT_DEBUG_MAPPRINT
#define ROBOT_DEBUG_INFORMATION

struct Cell
{
	char type = '?';
	bool visited = false;
};

struct Node{
	int x, y; // Coordinates
	int g, h; // g: cost to reach, h: heuristic estimate
	Node* parent; // Parent node

	Node(int x, int y, int g, int h, Node* parent)
		: x(x), y(y), g(g), h(h), parent(parent) {}

	int f() const { return g + h; } // Total cost estimate
};

class CompareNodes {
public:
	bool operator()(const Node* a, const Node* b) {
		return a->f() > b->f();
	}
};

bool isValid(int x, int y, const std::vector<std::vector<Cell>>& map) {
	return x >= 0 && x < map.size() && y >= 0 && y < map[0].size() && map[y][x].type != 'B';
}

std::vector<std::pair<int, int>> getPath(Node* endNode) {
	std::vector<std::pair<int, int>> path;
	Node* current = endNode;
	while (current) {
		path.emplace_back(current->x, current->y);
		current = current->parent;
	}
	std::reverse(path.begin(), path.end());
	return path;
}

std::vector<std::pair<int, int>> findPath(int startX, int startY, int goalX, int goalY, const std::vector<std::vector<Cell>>& map) {
	std::priority_queue<Node*, std::vector<Node*>, CompareNodes> openSet;
	std::vector<std::vector<bool>> closedSet(map.size(), std::vector<bool>(map[0].size(), false));

	Node* startNode = new Node(startX, startY, 0, abs(goalX - startX) + abs(goalY - startY), nullptr);
	openSet.push(startNode);

	while (!openSet.empty()) {
		Node* current = openSet.top();
		openSet.pop();

		if (current->x == goalX && current->y == goalY) {
			std::vector<std::pair<int, int>> path = getPath(current);
			while (!openSet.empty()) {
				delete openSet.top();
				openSet.pop();
			}
			return path;
		}

		closedSet[current->x][current->y] = true;

		int dx[] = { -1, 1, 0, 0 };
		int dy[] = { 0, 0, -1, 1 };

		for (int i = 0; i < 4; ++i) {
			int newX = current->x + dx[i];
			int newY = current->y + dy[i];

			if (isValid(newX, newY, map) && !closedSet[newX][newY]) {
				int newG = current->g + 1;
				int newH = abs(goalX - newX) + abs(goalY - newY);
				Node* newNode = new Node(newX, newY, newG, newH, current);
				openSet.push(newNode);
				closedSet[newX][newY] = true;
			}
		}
	}

	return {}; // No path found
}

enum class Move
{
	UP,
	DOWN,
	LEFT,
	RIGHT,

	STAY
};

Move calculateDirection(int startX, int startY, int goalX, int goalY)
{
	if (startX < goalX) {
		return Move::RIGHT;
	}
	else if (startX > goalX) {
		return Move::LEFT;
	}
	else if (startY < goalY) {
		return Move::DOWN;
	}
	else if (startY > goalY) {
		return Move::UP;
	}
	// points are the same or error has occured
	return Move::STAY;
}

enum class Action
{
	MINE_UP,
	MINE_DOWN,
	MINE_LEFT,
	MINE_RIGHT,

	ATTACK_UP,
	ATTACK_DOWN,
	ATTACK_LEFT,
	ATTACK_RIGHT,

	SCAN_UP,
	SCAN_DOWN,
	SCAN_LEFT,
	SCAN_RIGHT,

	PLACE_UP,
	PLACE_DOWN,
	PLACE_LEFT,
	PLACE_RIGHT
};

enum class Buy
{
	BUY_SIGHT,
	BUY_ATTACK,
	BUY_DRILL,
	BUY_ANTENNA,
	BUY_BATTERY,
	BUY_HEAL
};

enum class Goal
{
	EXPLORE,
	MINE_NEAREST_ORE,
	RETURN_HOME,
	KILL_NEAREST_PLAYER,
	RUN_FROM_NEAREST_PLAYER,
	RUN_FROM_ZONE,

	ACTION_FAILURE
};

class Rover
{
public:
	Rover()
	{
	}
	int spawn_x{}, spawn_y{};
	int newrandx{5}, newrandy{5};
	int rows{}, cols{};
	std::vector<std::vector<Cell>> map{};
	int pos_x{}, pos_y{};
	int hp{}, dig_lvl{}, atk_lvl{}, move_lvl{}, sight_lvl{}, has_antenna{}, has_battery{};
	int stone{}, iron{}, osmium{};
};

// TODO
int distance_from_zone(Rover& rover)
{
	return 999;
}

// TODO
bool player_visible(Rover& rover)
{
	return false;
}

// TODO
std::pair<int, int> ore_visible(Rover& rover)
{
	for (int i = 0; i < rover.rows; ++i) {
		for (int j = 0; j < rover.cols; ++j) {
			if (rover.map[i][j].type == 'D' || rover.map[i][j].type == 'C')
				return { i, j };
		}
	}
	return {-1, -1};
}

// please don't ask me about this i have no idea what i'm doing 
Goal decideOnNextGoal(Rover& rover)
{
	if (distance_from_zone(rover) <= 3)
		Goal::RUN_FROM_ZONE;

	// if there's a player and you're healthy: fight, else run
	else if (player_visible(rover) && rover.hp >= 10)
		return Goal::KILL_NEAREST_PLAYER;
	else if (player_visible(rover))
		return Goal::RUN_FROM_NEAREST_PLAYER;

	else if (ore_visible(rover).first > 0 && ore_visible(rover).second > 0)
		return Goal::MINE_NEAREST_ORE;

	// tweak these 
	else if (rover.iron >= 1 && rover.osmium >= 1 && !rover.has_battery)
		return Goal::RETURN_HOME;

	else
		return Goal::EXPLORE;

	return Goal::ACTION_FAILURE;
}

int getID()
{
	int id = 0;
	std::cout << "enter id: ";
	std::cin >> id;
	std::cout << "id read" << std::endl;
	return id;
}

std::string readServerFile(std::ifstream& input)
{
	std::stringstream buffer;
	buffer << input.rdbuf();
	input.close();
	return buffer.str();
}

std::string get_movement_command(Move move)
{
	std::string command{};

	switch (move)
	{
	case (Move::UP): {
		command += "U ";
		break;
	}
	case (Move::DOWN): {
		command += "D ";
		break;
	}
	case (Move::LEFT): {
		command += "L ";
		break;
	}
	case (Move::RIGHT): {
		command += "R ";
		break;
	}
	case (Move::STAY): {
		break;
	}
	}
	
	return command;
}

std::vector<std::pair<int, int>> exploreUnexplored(int startX, int startY, Rover& rover)
{
	if ((std::abs(rover.pos_x - rover.newrandx) + std::abs(rover.pos_y - rover.newrandy)) <= 5) {
		std::random_device seeder;
		std::mt19937 engine(seeder());
		std::uniform_int_distribution<int> dist(0, rover.map.size());
		rover.newrandx = dist(engine);
		rover.newrandy = dist(engine);
		std::cout << "\n\n\n\n\n\n\n\n";
	}
	std::cout << rover.newrandx << " " << rover.newrandy << std::endl;

	std::vector<std::pair<int, int>> path = findPath(rover.pos_x, rover.pos_y, rover.newrandx, rover.newrandy, rover.map);

	return path;
}

int main()
{
	// ID
	int id = getID();

	// keep track of round
	int round = 0;

	// rover
	Rover rover{ };

	// main loop
	while (true)
	{
		// get time
		auto begin = std::chrono::high_resolution_clock::now();
		// look for existence of server file corresponding to the current round
		std::string serverFileName = "game/s" + std::to_string(id) + 
			"_" + std::to_string(round) + ".txt";
		std::ifstream input(serverFileName);
		// if the file is found:
		if (input)
		{
			std::cout << "Server file (round " << round << ") found!\n";

			// sleep a little to make sure that the file was written to by the server
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

// updates the map according to movement
#pragma region map_updating

			if (round == 0) {
				// read the size of the map
				input >> rover.rows >> rover.cols;

				// eat the first line
				std::string line;
				std::getline(input, line);

				// setup the map the first time
				rover.map.resize(rover.cols);
				for (auto& row : rover.map) {
					row.resize(rover.rows);
				}
				// fill it
				for (int i = 0; i < rover.rows; i++) {
					std::getline(input, line);
					std::istringstream lineStream(line);
					for (int j = 0; j < rover.cols; ++j) {
						char type;
						lineStream >> type;
						rover.map[i][j].type = type;
						if (type != '?')
							rover.map[i][j].visited = true;
					}
				}
			}
			// round > 0
			else
			{
				// eat the first line
				std::string line;
				std::getline(input, line);

				// fill new map spaces
				for (int i = 0; i < rover.rows; i++) {
					std::getline(input, line);
					std::istringstream lineStream(line);
					for (int j = 0; j < rover.cols; j++) {
						char type;
						lineStream >> type;
						if (type != '?') {
							rover.map[i][j].type = type;
							rover.map[i][j].visited = true;
						}
					}
				}
			}

#ifdef ROBOT_DEBUG_MAPPRINT
			for (int i = 0; i < rover.rows; ++i) {
				for (int j = 0; j < rover.cols; ++j) {
					std::cout << rover.map[i][j].type << " ";
				}
				std::cout << std::endl;
			}
#endif

#pragma endregion

// reads the robots data from the file
#pragma region info

			std::string line;
			std::getline(input, line);
			std::istringstream poslineStream(line);
			poslineStream >> rover.pos_x >> rover.pos_y;
			if (!round) {
				rover.spawn_x = rover.pos_x;
				rover.spawn_y = rover.pos_y;
			}

			std::getline(input, line);
			std::istringstream statslineStream(line);
			statslineStream >> rover.hp >> rover.dig_lvl >> rover.atk_lvl
				>> rover.move_lvl >> rover.sight_lvl >> rover.has_antenna >> rover.has_battery;

			std::getline(input, line);
			std::istringstream resourcelineStream(line);
			resourcelineStream >> rover.stone >> rover.iron >> rover.osmium;

#ifdef ROBOT_DEBUG_INFORMATION
			std::cout << "x: " << rover.pos_x << ", y: " << rover.pos_y << std::endl;
			std::cout << "hp: " << rover.hp << ", dig: " << rover.dig_lvl << ", attack: " << rover.atk_lvl << ", move: " << rover.move_lvl << ", sight: "
				<< rover.sight_lvl << ", antenna: " << rover.has_antenna << ", has battery: " << rover.has_battery << std::endl;
			std::cout << "stone: " << rover.stone << ", iron: " << rover.iron << ", osmium: " << rover.osmium << std::endl;
#endif

#pragma endregion

// decide on the action to take this round (buying is seperate)
#pragma region decision_making

			// store the command string
			std::string move_cmd = "", action_cmd = "", buy_cmd = "";

			Goal nextGoal = decideOnNextGoal(rover);
			
			std::cout << static_cast<int>(nextGoal) << "\n";

			switch (nextGoal)
			{
			case (Goal::EXPLORE): {
				// find a path to the nearest unvisited cell
				std::cout << "begin exploration...\n";
				std::vector<std::pair<int, int>> path = exploreUnexplored(rover.pos_x, rover.pos_y, rover);

				for (auto pair : path) {
					std::cout << "x: " << pair.first << " y: " << pair.second << std::endl;
				}

				for (int i = 1; i <= rover.dig_lvl && i <= path.size(); i++) {
					Move move = calculateDirection(path.at(i - 1).first, path.at(i - 1).second, path.at(i).first, path.at(i).second);
					move_cmd += get_movement_command(move);
				}
				if (path.size() >= 2) {
					Move move = calculateDirection(path.at(0).first, path.at(0).second, path.at(1).first, path.at(1).second);
					action_cmd += "M " + get_movement_command(move);
				}
				std::cout << "successful exploration...\n";
				break;
			}
			case (Goal::MINE_NEAREST_ORE): {
				auto [y, x] = ore_visible(rover);
				auto path = findPath(rover.pos_x, rover.pos_y, x, y, rover.map);
				for (auto pair : path) {
					std::cout << "x: " << pair.first << " y: " << pair.second << std::endl;
				}

				for (int i = 1; i <= rover.dig_lvl && i <= path.size(); i++) {
					Move move = calculateDirection(path.at(i - 1).first, path.at(i - 1).second, path.at(i).first, path.at(i).second);
					move_cmd += get_movement_command(move);
				}
				if (path.size() >= 2) {
					Move move = calculateDirection(path.at(0).first, path.at(0).second, path.at(1).first, path.at(1).second);
					action_cmd += "M " + get_movement_command(move);
				}
				break;
			}
			case (Goal::KILL_NEAREST_PLAYER): {

				break;
			}
			case (Goal::RUN_FROM_NEAREST_PLAYER): {

				break;
			}
			case (Goal::RUN_FROM_ZONE): {

				break;
			}
			case (Goal::RETURN_HOME): {

				break;
			}
			default: {
				std::cout << "A decision making Error has occured." << std::endl;
			}
			}

			// output string
			std::string out_cmd = (move_cmd + action_cmd + buy_cmd);

#pragma endregion

			// write the response back
			std::string ourFileName = "game/c" + std::to_string(id) + "_" + std::to_string(round) +
				".txt";
			std::ofstream response(ourFileName);

			response << out_cmd;
			response.close();

			// increment the round
			round++;

			// time measurement
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_taken = end - begin;
			std::cout << "Time taken to calculate move: " << time_taken.count() << std::endl;
		}
		else
		{
			//waiting
		}
	}
	return 0;
}