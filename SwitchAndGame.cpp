#include "SwitchAndGate.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

SwitchAndGate::SwitchAndGate() : lastSwitchIndexP1(-1), lastSwitchIndexP2(-1)
{
    // safe defaults (until room is loaded and scanned)
    for (int i = 0; i < SWITCH_BITS; ++i)
    {
        requiredStateToOpen[i] = true;
        curSwitchesState[i] = false;
    }
}

void SwitchAndGate::LoadSwitchesFromRoom(int height, int width, const std::function<char(int, int)>& getAt)
{
    switchesLocations.clear();

    // Skip the frame so border '=' doesn't become "gate tiles"
    for (int r = 1; r < height-1; ++r)
    {
        for (int c = 1; c < width-1; ++c)
        {
            char ch = getAt(r, c);

            //if the char is a switch
            if (ch == openSwitchChar || ch == closedSwitchChar)
            {
                switchesLocations.push_back({c,r}); //{ r, c }
            }
        }
    }
    
    //initiallize states
    for (int i = 0; i < SWITCH_BITS; ++i)
    {
        requiredStateToOpen[i] = true;
        curSwitchesState[i] = false;
    }

    lastSwitchIndexP1 = -1;
    lastSwitchIndexP2 = -1;

    //make sure the switches are read in the right order to open the gate
    std::sort(switchesLocations.begin(), switchesLocations.end(),[](const Position& a, const Position& b)
    {
        if (a.y != b.y) return a.y < b.y; // top to bottom
        return a.x < b.x;                 // left to right
    });
}


void SwitchAndGate::updateSwitchState(int playerX, int playerY, int playerId,const std::function<void(int, int, char)>& setAt)
{
    int& last = (playerId == 1) ? lastSwitchIndexP1 : lastSwitchIndexP2;
    int currentSwitchIndex = -1;
    
    int count = (int)switchesLocations.size();
    if (count > SWITCH_BITS) count = SWITCH_BITS;
    //find which switch the player is standing on
    for (int i = 0;i < count; ++i)
    {
        const Position& switchPos = switchesLocations[i];
        //check if the players location is the same as switch
        if (switchPos.x == playerX && switchPos.y == playerY)
        {
            currentSwitchIndex = i;
            break;
        }
    }
    // Toggle only when stepping ONTO a switch
    if (currentSwitchIndex != -1 && currentSwitchIndex != last)
    {
        curSwitchesState[currentSwitchIndex] = !curSwitchesState[currentSwitchIndex];
        // update the actual tile on the board
        const Position& p = switchesLocations[currentSwitchIndex];
        //replace with the required switch
        char c;
        if (curSwitchesState[currentSwitchIndex])
            c = openSwitchChar;
        else
            c = closedSwitchChar;
        setAt(p.y, p.x,c);
    }
    // Remember for next time
    last = currentSwitchIndex;
}

//check if switch is on or off and calculate the value
int SwitchAndGate::calcSwitchValue()const
{
    int value = 0;
    for (int i = 0; i < SWITCH_BITS; ++i)
    {
        //if the switch is on
        if (curSwitchesState[i])
        {
            int bit = (SWITCH_BITS - 1 - i);
            value += (1 << bit);
        }
    }
    return value;
}

void SwitchAndGate::updateGateState(const std::function<void(int, int, char)>& setAt)
{
    int val = calcSwitchValue();

    for (const auto& g : gates)
    {
        bool open;
        if (g.gateNumber == val)
            open = true;
        else
            open = false;
        char ch;
        if (open)
            ch = openGateChar;
        else
            ch = closedGateChar;

        for (int c = g.colStart; c <= g.colEnd; ++c)
            setAt(g.row, c, ch);
    }
    
}

bool SwitchAndGate::LoadGatesFromFile(const std::string& gateFilename,const std::function<void(int, int, char)>& setAt)
{
    gates.clear();

    std::ifstream in(gateFilename);
    if (!in.is_open())
        return false;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string tag;
        GateSegment g{};

        if (!(iss >> tag >> g.gateNumber >> g.row >> g.colStart >> g.colEnd))
            continue;

        if (tag != "GATE")
            continue;

        if (g.gateNumber < 0 || g.gateNumber > 15) continue;
        if (g.colStart > g.colEnd) continue;

        gates.push_back(g);

        // Draw closed gate
        for (int c = g.colStart; c <= g.colEnd; ++c)
            setAt(g.row, c, closedGateChar);
    }
    return true;
}

