#pragma once
#include "Board.h"
class Board;

class Player {

    int life = 5;
    
    // symbol is either '$' or '&'
    char symbol;

    // currently held item (if any)
    char item;

    // current position on board
    int x, y;
    // previous position
    int prevX, prevY;

    // movement state
    int speed; // speed: 0 = stopped, 1 = normal, >1 = boosted
    char direction; // movement direction either: none, up, down, left, right

    //handlling obstacle pushing
    int m_pushesLeftInBoost = 0;

    // spring and boost ticks (ticks as in how many game cycles remaining)
    int boostTicks;
    int boostSpeed_; //speed while boosted
    bool compressingSpring;  // if player currently compressing a spring
    int springCompressed; //compression amount
    char springCompressedDir; // direction of compressed spring
    char sideDirection;  // sideways movement direction while boosted

    // spring handling helpers:
    // apply the spring boost to the player, after ending compression phase
    void applySpringBoost(bool bounceOffWall);
    // get opposite direction of player movement
    char getOppositeDirection(char dir);

    // Obstacle handling helpers:
    // find all connected obstacle cells using flood-fill, returns count
    int findObstacleGroup(Board& board, int startRow, int startCol, bool visited[BOARD_HEIGHT][BOARD_WIDTH]);
    // check if obstacle group can be pushed without hitting walls/objects
    bool canPushObstacle(Board& board, bool visited[BOARD_HEIGHT][BOARD_WIDTH], int dx, int dy, Player* other);
    // move all cells in obstacle group by (dx, dy)
    void pushObstacleGroup(Board& board, bool visited[BOARD_HEIGHT][BOARD_WIDTH], int dx, int dy);

    // Movement helpers
    // handle sideways movement while boosted
    void handleSidewaysMovement(Board& board, Player& other);
    // try to pick up item at position if player has no item
    void tryPickupItem(Board& board, char cell, int newX, int newY);
    // get delta x and y for current direction
    void getDirectionDelta(int& dx, int& dy) const;
    // clamp position to room boundaries
    void clampToRoom(int& newX, int& newY) const;

public: enum Directions{
        // directions, just a number to assoiciate with each, in order to use constants instead of magic numbers
        DIRECTION_NONE = 0,
        DIRECTION_UP = 1,
        DIRECTION_DOWN = 2,
        DIRECTION_LEFT = 3,
        DIRECTION_RIGHT = 4,

        // This is the starting positions for players in room 0, it is just defined here for convenience
        // because we start in room 0 always
        START_X_P1 = 5,
        START_Y_P1 = 20,
        START_X_P2 = 5,
        START_Y_P2 = 21,

        // normal speed is always 1
        NORMAL_SPEED = 1,
        // no speed is 0
        NO_SPEED = 0,
        // symbol for no item held, convenience
        NO_ITEM = ' '
    };


public:
    // constructor: initialize player with given symbol, either '$' or '&'
    Player(char playerSymbol);

    // movement and game logic:
    // Move handles the player movement for one game cycle
    void Move(Board& board, Player& other, int otherStartX, int otherStartY, char otherStartDir, int otherStartForce);
    // change movement direction based on key input
    void changeDirection(char key);
    // reset player to given start position (for room transitions, bomb hit, etc.) 
    void ResetToStart(int startX, int startY);
    // step back to previous cell (used when hitting wall while boosting)
    void stepBackToPrevCell();
    // stop all movement from player
    void stopMovement();
    // remove player from board (used when transitioning rooms, when entering the door)
    void removeFromBoard();

    // item handling:
    // get currently held item of player
    char getItem() const;
    void setItem(char newItem);

    // use bomb if holding one, returns true if bomb was used
    bool useBomb(Board& board);
    // drop currently held item onto the board at position set in the code according to player position
    void dropItem(Board& board);
    // consume (remove) the currently held key, after using it on a door
    void consumeKey();

    // spring collision:
    // check if player is currently in boosting affect
    bool isBoosting() const;
    // this function is called when another player hits this player while this player is boosting from a spring
    void takeSpringImpactFrom(const Player& other);
    // clears the boost affect from the player
    void clearBoost();

    // force for pushing obstacles returns current force value based on boost state
    int getForce() const;

    // getters:
    int GetX() const;
    int GetY() const;
    char GetSymbol() const;
    char getDirection() const;

    //torch
    bool hasTorch() const;
    void forceSetItem(char it) { item = it; }

    //handle players life
    void loseLife();
    int getLife() const;
    
};

static bool isFreeForObstacle(char ch);
