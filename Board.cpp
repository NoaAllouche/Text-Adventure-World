#include "Board.h"
#include "SwitchAndGate.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>

void Board::resetRoomState()
{
    // Reset switches
    switchCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        switchRow[i] = -1;
        switchCol[i] = -1;
    }

    // Reset gates
    gateCount = 0;
    for (int i = 0; i < 9; ++i)
    {
        gates[i].number = 0;
        gates[i].row = -1;
        gates[i].colStart = -1;
        gates[i].colEnd = -1;
        gates[i].isOpen = false;
    }
}

// Constructor: initialize board and reset switch/gate system
Board::Board()
{
    // initialize switch , gate tracking
    resetRoomState();

    //initialize board
    for (int r = 0; r < BOARD_HEIGHT; ++r)
        for (int c = 0; c < BOARD_WIDTH; ++c)
            board[r][c] = ' ';

}

// Print the board to console using std::cout
void Board::printBoard()
{
    
    for (int row = 0; row < BOARD_HEIGHT; ++row)
    {
        for (int col = 0; col < BOARD_WIDTH; ++col)
            std::cout << board[row][col];
        std::cout << std::endl;
    }
}

// Print the board with darkness support
void Board::printBoardMasked(bool illuminate)
{
    for (int row = 0; row < BOARD_HEIGHT; ++row)
    {
        for (int col = 0; col < BOARD_WIDTH; ++col)
        {
            char c = board[row][col];

            if (isDarkRoom && !illuminate)
            {
                ////always show the frame
                bool isFrame = (row == 0 || row == BOARD_HEIGHT - 1 || col == 0 || col == BOARD_WIDTH - 1);
                if (isFrame)
                {
                    std::cout << c;
                    continue;
                }

                //hide gates in the dark
                if (c == GATE_SYMBOL)
                {
                    std::cout << ' ';
                    continue;
                }
                //always show the torch so it can be found and the players
                bool isAlwaysVisible = (c == TORCH_SYMBOL || c==P1_SYMBOL ||c==P2_SYMBOL);

                if (!isFrame && !isAlwaysVisible)
                    c = ' ';
            }

            std::cout << c;
        }
        std::cout << std::endl;
    }
}


// Set board cell at (row, col) to character c
void Board::setChar(int row, int col, char c)
{
    if (row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH)
        board[row][col] = c;
}

// Get character at (row, col)
char Board::getChar(int row, int col) const
{
    if (row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH)
        return board[row][col];
    return '\0';
}

void Board::updateSwitchesAndGates(int p1x, int p1y, int p2x, int p2y)
{
    auto setTheChar = [&](int r, int c, char ch) { setChar(r, c, ch); };

    m_switchAndGate.updateSwitchState(p1x, p1y, 1, setTheChar);
    m_switchAndGate.updateSwitchState(p2x, p2y, 2, setTheChar);

    m_switchAndGate.updateGateState(setTheChar);
}

//Load room layout from a file 
bool Board::loadRoomFromFile(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in.is_open())
        return false;

    resetRoomState();

    std::string line;

    // Skip everything until we find the top border line
    while (std::getline(in, line))
    {
        //detect darkness marker in header area
        if (line.find("DARK") != std::string::npos || line.find("dark") != std::string::npos)
            isDarkRoom = true;

        if (!line.empty() && line[0] == '+') // the "+====...+"
            break;
    }

    if (in.fail())  // never found the frame
        return false;

    auto normalizeToWidth = [&](std::string& s) //s is a line in the file
        {
            if ((int)s.size() < BOARD_WIDTH)
                s.append(BOARD_WIDTH - (int)s.size(), ' ');
            else if ((int)s.size() > BOARD_WIDTH)
                s.resize(BOARD_WIDTH);
        };

    // write row 0 (already read)
    normalizeToWidth(line);
    for (int col = 0; col < BOARD_WIDTH; ++col)
        setChar(0, col, line[col]);

    // write remaining rows 1..BOARD_HEIGHT-1
    for (int row = 1; row < BOARD_HEIGHT; ++row)
    {
        if (!std::getline(in, line))
            return false;

        normalizeToWidth(line);

        for (int col = 0; col < BOARD_WIDTH; ++col)
            setChar(row, col, line[col]);
    }

    //build switches and gates after the board is fully loaded
    m_switchAndGate.LoadSwitchesFromRoom(BOARD_HEIGHT,BOARD_WIDTH,[&](int r, int c) { return getChar(r, c); });
    std::string gatesFile = filename;
    auto pos = gatesFile.rfind(".screen.txt");
    if (pos != std::string::npos)
        gatesFile.replace(pos, std::string(".screen.txt").size(), ".gates.txt");
    else
        gatesFile += ".gates.txt";

    bool ok = m_switchAndGate.LoadGatesFromFile(gatesFile,[&](int r, int c, char ch) { setChar(r, c, ch); }
);
    bool room3 = filename[9] == '3';
    if (!ok && !room3)
    {
        std::cout << "Failed to load gates file: " << gatesFile << "\n";
    }
    return true;
} 

