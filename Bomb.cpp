#include "Bomb.h"
#include "Board.h"

Bomb::Bomb()
{
    x = 0;
    y = 0;
    timer = 0;
    active = false;
}

void Bomb::activate(int posX, int posY)
{
    x = posX;
    y = posY;
    timer = BOMB_TIMER_START;
    active = true;
}

// returns true if explosion happened this tick
bool Bomb::tick(Board& board)
{
    if (!active)
        return false;

    timer--;

    if (timer > 0)
        return false;

    // timer reached 0 - explode!
    board.setChar(y, x, NOTHING_SYMBOL);

    // destroy in all 8 directions
    const int dx[8] = { 0, 0, -1, 1, -1, -1, 1, 1 };
    const int dy[8] = { -1, 1, 0, 0, -1, 1, -1, 1 };

    for (int dir = 0; dir < 8; dir++)
    {
        for (int dist = 1; dist <= EXPLOSION_RADIUS; dist++)
        {
            int checkX = x + dx[dir] * dist;
            int checkY = y + dy[dir] * dist;

            char c = board.getChar(checkY, checkX);

            // don't destroy doors
            bool isDoor = (c >= DOOR1_SYMBOL && c <= DOOR9_SYMBOL);

            // wall stops explosion in this direction but gets destroyed
            if (c == WALL_SYMBOL)
            {
                board.setChar(checkY, checkX, NOTHING_SYMBOL);
                break;
            }

            if (c != NOTHING_SYMBOL && !isDoor)
            {
                board.setChar(checkY, checkX, NOTHING_SYMBOL);
            }
        }
    }

    active = false;
    return true;
}

bool Bomb::isActive() const { return active; }
int Bomb::getX() const { return x; }
int Bomb::getY() const { return y; }