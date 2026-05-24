#pragma once

class Board;

// Bomb class - handles deployed bomb countdown and explosion
class Bomb {

    int x, y;           // position of deployed bomb
    int timer;          // countdown (starts at 5)
    bool active;        // is this bomb currently ticking?

    enum {
        BOMB_TIMER_START = 5,
        EXPLOSION_RADIUS = 3
    };

public:
    Bomb();

    // deploy bomb at position, starts countdown
    void activate(int posX, int posY);

    // called every game cycle - counts down and explodes when ready
    // returns true if explosion happened this tick
    bool tick(Board& board);

    // getters
    bool isActive() const;
    int getX() const;
    int getY() const;
};
