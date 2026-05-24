#pragma once
#include <vector>
#include <string>
#include <functional>


struct Position 
{
	int x;
	int y;
};

enum
{
	SWITCH_BITS = 4
};

class SwitchAndGate
{
private:

	struct GateSegment
	{
		int gateNumber;
		int row;
		int colStart;
		int colEnd;
	};

	//all switches positions
	std::vector<Position>switchesLocations;

	//all gates positions
	std::vector<GateSegment>gates;

	//the required state for each switch to open the gate
	bool requiredStateToOpen[SWITCH_BITS];

	//current state of the switches
	bool curSwitchesState[SWITCH_BITS];

	//int lastSwitchIndex;
	int lastSwitchIndexP1;
	int lastSwitchIndexP2;
public:
	// Tile representations
	static constexpr char openSwitchChar = '/';
	static constexpr char closedSwitchChar = '\\';
	static constexpr char closedGateChar = '=';
	static constexpr char openGateChar = ' ';

	SwitchAndGate();

	//get the switches positions from the room
	void LoadSwitchesFromRoom(int height, int width, const std::function<char(int, int)>& getAt);
	// Load gates from separate file AND draw them closed onto the board
	bool LoadGatesFromFile(const std::string& gateFilename,const std::function<void(int, int, char)>& setAt);

	void updateSwitchState(int playerX, int playerY, int playerId,const std::function<void(int, int, char)>& setAt);
	int calcSwitchValue() const;
	void updateGateState(const std::function<void(int, int, char)>& setAt);
	
};
