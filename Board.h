#pragma once
#include <string>
#include "SwitchAndGate.h"

// Constants that are associated with the board
enum {
    BOARD_HEIGHT = 22,
    BOARD_WIDTH = 80,
    BOARD_TOP=3,
    ROOM_INFO12 = BOARD_HEIGHT+1,
    ROOM_INFO = 25,
    RIDDLE_LINE12 = BOARD_TOP + 1+ ROOM_INFO,
    RIDDLE_LINE = 30,

    // Player symbols and controls
    P1_SYMBOL = '$',
    P2_SYMBOL = '&',

    // Game element symbols
    TORCH_SYMBOL = '!',
    BOMB_SYMBOL = '@',
    WALL_SYMBOL = 'W',
    OBSTACLE_SYMBOL = '*',
    SPRING_SYMBOL = '#',
    SWITCH_ON_SYMBOL = '/',
    SWITCH_OFF_SYMBOL = '\\',
    GATE_SYMBOL = '=',
    KEY_SYMBOL = 'K',
    NOTHING_SYMBOL = ' ',
    RIDDLE_SYMBOL = '?',

    // Door symbols (1-9)
    DOOR1_SYMBOL = '1',
    DOOR2_SYMBOL = '2',
    DOOR3_SYMBOL = '3',
    DOOR4_SYMBOL = '4',
    DOOR5_SYMBOL = '5',
    DOOR6_SYMBOL = '6',
    DOOR7_SYMBOL = '7',
    DOOR8_SYMBOL = '8',
    DOOR9_SYMBOL = '9',

    // Playable area boundaries (inside the frame)
    Room_MIN_X = 1,
    Room_MAX_X = 78,
    Room_MIN_Y = 5,
    Room_MAX_Y = 23
};

class Board {

    // The board grid
    char board[BOARD_HEIGHT][BOARD_WIDTH];

    // switches and gates SYSTEM (object)
    SwitchAndGate m_switchAndGate;

    // Switch system: 4 switches with bit values 1,2,4,8
    // positions stored as row,col pairs
    int switchRow[4];
    int switchCol[4];
    int switchCount;

    // Gate positions: each gate has a number (1-9) and position range
    struct Gate {
        int number;      // gate number (matches door number)
        int row;         // row of gate
        int colStart;    // starting column
        int colEnd;      // ending column
        bool isOpen;
    };
    Gate gates[9];
    int gateCount;
    void resetRoomState();

    //dark room
    bool isDarkRoom = false;

public:
    // empty constructor: a board with only frame
    Board();
    // Print the board to console using std::cout
    void printBoard();
    void printBoardMasked(bool illuminate);

    // Set board cell at (row, col) to character c
    void setChar(int row, int col, char c);
    // Get character at (row, col)
    char getChar(int row, int col) const;

    // Switch system
    void updateSwitchesAndGates(int p1x, int p1y, int p2x, int p2y);
    //load rooms from files
    bool loadRoomFromFile(const std::string& filename);
    //dark room
    bool getIsDarkRoom() const { return isDarkRoom; }
    void setIsDarkRoom(bool v) { isDarkRoom = v; }
};
