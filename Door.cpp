#include "Door.h"

Door::Door()
{
    positionCount = 0;
    targetRoom = -1;
    isOpen = false;
    for (int i = 0; i < 3; i++)
        positions[i].used = false;
}

void Door::addPosition(int room, int x, int y)
{
    if (positionCount >= 3)
        return;
    positions[positionCount].room = room;
    positions[positionCount].x = x;
    positions[positionCount].y = y;
    positions[positionCount].used = true;
    positionCount++;
}

void Door::setTargetRoom(int room)
{
    targetRoom = room;
}

bool Door::isAtPosition(int room, int x, int y) const
{
    for (int i = 0; i < positionCount; i++)
    {
        if (positions[i].room == room &&
            positions[i].x == x &&
            positions[i].y == y)
            return true;
    }
    return false;
}

int Door::getTargetRoom() const
{
    return targetRoom;
}

bool Door::getIsOpen() const
{
    return isOpen;
}

void Door::open()
{
    isOpen = true;
}

void Door::reset()
{
    isOpen = false;
}