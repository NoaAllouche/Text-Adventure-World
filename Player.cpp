#include "Player.h"
#include "Board.h"
#include <cstdlib>  // for abs()
#include "ConsoleUtils.h"
#include "Torch.h"
// Constructor: initialize player with given symbol, either '$' or '&'
Player::Player(char playerSymbol)
{
    // set symbol
    symbol = playerSymbol;
    // set item as no item
    item = NO_ITEM;
    // start with no speed
    speed = NO_SPEED;
    // no movement direction
    direction = DIRECTION_NONE;

    // spring state: no boost, normal speed, not compressing spring
    boostTicks = 0;
    boostSpeed_ = NORMAL_SPEED;
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;

    // set starting position based on symbol
    if (symbol == '$')
    {
        x = START_X_P1;
        y = START_Y_P1;
    }
    else // if symbol is '&'
    {
        x = START_X_P2;
        y = START_Y_P2;
    }

    // the previous position is the same as starting
    prevX = x;
    prevY = y;
}

// apply the spring boost to the player, after ending compression phase
void Player::applySpringBoost(bool bounceOffWall)
{
    if (springCompressed <= 0)
        return;

    // calculate boost speed and ticks based on compression amount
    int n = springCompressed;
    // as to the requirements, boost speed = compression amount , ticks = square of n
    boostSpeed_ = n;
    boostTicks = n * n;

    // if the spring interaction was when the springs was attached to a wall
    if (bounceOffWall)
        // change direction to opposite
        direction = getOppositeDirection(springCompressedDir);
    else 
        // keep the same direction as when compressing
        direction = springCompressedDir;

    // set speed to boost speed
    speed = boostSpeed_;
    //2 pushes per boost
    m_pushesLeftInBoost = 2;

    //clear spring state
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;
}

// get opposite direction of player movement
char Player::getOppositeDirection(char dir)
{
    if (dir == DIRECTION_UP) return DIRECTION_DOWN;
    if (dir == DIRECTION_DOWN) return DIRECTION_UP;
    if (dir == DIRECTION_LEFT) return DIRECTION_RIGHT;
    if (dir == DIRECTION_RIGHT) return DIRECTION_LEFT;
    return DIRECTION_NONE;
}

// Find all obstacle cells connected to the starting cell
// How it works: keep checking neighbors until no new obstacles are found
// Returns count of connected obstacles, stores positions in visited array
int Player::findObstacleGroup(Board& board, int startRow, int startCol, bool visited[BOARD_HEIGHT][BOARD_WIDTH])
{
    // initialize all cells as not visited
    for (int r = 0; r < BOARD_HEIGHT; ++r)
        for (int c = 0; c < BOARD_WIDTH; ++c)
            visited[r][c] = false;

    // mark starting obstacle cell
    visited[startRow][startCol] = true;
    int count = 1;

    // keep looping until no new neighbors found
    bool foundNew = true;
    while (foundNew)
    {
        foundNew = false;

        // scan room area looking for cells already in our group
        for (int row = Room_MIN_Y; row <= Room_MAX_Y; ++row)
        {
            for (int col = Room_MIN_X; col <= Room_MAX_X; ++col)
            {
                // skip cells not in our group
                if (!visited[row][col])
                    continue;

                // check 4 neighbors: up, down, left, right
                int checkRows[4] = { row - 1, row + 1, row, row };
                int checkCols[4] = { col, col, col - 1, col + 1 };

                for (int i = 0; i < 4; ++i)
                {
                    int nr = checkRows[i];
                    int nc = checkCols[i];

                    // skip if out of room bounds
                    if (nr < Room_MIN_Y || nr > Room_MAX_Y)
                        continue;
                    if (nc < Room_MIN_X || nc > Room_MAX_X)
                        continue;

                    // if neighbor is obstacle and not yet in group, add it
                    if (board.getChar(nr, nc) == OBSTACLE_SYMBOL && !visited[nr][nc])
                    {
                        visited[nr][nc] = true;
                        count++;
                        foundNew = true;
                    }
                }
            }
        }
    }

    return count;
}

static bool isFreeForObstacle(char ch)
{
    return ch == NOTHING_SYMBOL || ch == SWITCH_ON_SYMBOL || ch == SWITCH_OFF_SYMBOL || ch == SPRING_SYMBOL;
}

// Check if all obstacle cells in the group can be pushed
// Goes through each cell in group, checks if destination is free
bool Player::canPushObstacle(Board& board, bool visited[BOARD_HEIGHT][BOARD_WIDTH], int dx, int dy, Player* other)
{
    // check each cell in the obstacle group
    for (int row = Room_MIN_Y; row <= Room_MAX_Y; ++row)
    {
        for (int col = Room_MIN_X; col <= Room_MAX_X; ++col)
        {
            // skip cells not in our group
            if (!visited[row][col])
                continue;

            // calculate new cell position
            int destRow = row + dy;
            int destCol = col + dx;

            // bounds check
            if (destCol < Room_MIN_X || destCol > Room_MAX_X)
                return false;
            if (destRow < Room_MIN_Y || destRow > Room_MAX_Y)
                return false;

            // if destination is also part of obstacle group, it moves too
            if (visited[destRow][destCol])
                continue;

            // other player blocks
            if (other && destCol == other->GetX() && destRow == other->GetY())
                return false;

            // check if destination cell is not empty
            char destCell = board.getChar(destRow, destCol);
            if (destCell == NOTHING_SYMBOL)
            {
                return true;
            }
        }
    }
    return true;
}



// Move all obstacle cells in the group by (dx, dy)
void Player::pushObstacleGroup(Board& board, bool visited[BOARD_HEIGHT][BOARD_WIDTH], int dx, int dy)
{
    // Step 1: clear all old positions
    for (int row = Room_MIN_Y; row <= Room_MAX_Y-3; ++row)
        for (int col = Room_MIN_X; col <= Room_MAX_X; ++col)
            if (visited[row][col])
                board.setChar(row, col, NOTHING_SYMBOL);

    // Step 2: set new positions
    for (int row = Room_MIN_Y; row <= Room_MAX_Y-3; ++row)
        for (int col = Room_MIN_X; col <= Room_MAX_X; ++col)
            if (visited[row][col])
                board.setChar(row + dy, col + dx, OBSTACLE_SYMBOL);
}


// get current force (speed) of player
int Player::getForce() const
{
    if (boostTicks > 0)
        return boostSpeed_;
    return NORMAL_SPEED;
}


//convert current directions to dx and dy
void Player::getDirectionDelta(int& dx, int& dy) const
{
    dx = 0;
    dy = 0;
    if (direction == DIRECTION_UP) dy = -1;
    else if (direction == DIRECTION_DOWN) dy = 1;
    else if (direction == DIRECTION_LEFT) dx = -1;
    else if (direction == DIRECTION_RIGHT) dx = 1;
}

// clamp position to room bounds (keeps player inside the playable area)
void Player::clampToRoom(int& newX, int& newY) const
{
    int maxX = BOARD_WIDTH - 2;
    if (newX < 1) newX = 1;
    if (newX > maxX) newX = maxX;
    if (newY < 1) newY = 1;
    if (newY > BOARD_HEIGHT) newY = BOARD_HEIGHT;
}

// handle sideways movement while boosted (spring associated function)
void Player::handleSidewaysMovement(Board& board, Player& other)
{
    // only if boosted and side direction is set
    if (boostTicks <= 0 || sideDirection == DIRECTION_NONE)
        return;

    // calculate side position
    int sideX = x, sideY = y;

    // calculate side position based on input side direction
    if (sideDirection == DIRECTION_UP) sideY--;
    else if (sideDirection == DIRECTION_DOWN) sideY++;
    else if (sideDirection == DIRECTION_LEFT) sideX--;
    else if (sideDirection == DIRECTION_RIGHT) sideX++;

    // keep inside room bounds (this was done to prevent bugs)
    clampToRoom(sideX, sideY);

    // avoid other player
    if (sideX == other.GetX() && sideY == other.GetY())
        return;

    // check if side position is free on board (that there is a ' ' there)
    char sideCell = board.getChar(sideY, sideX);

    // if free, move to side position
    if (sideCell != WALL_SYMBOL && sideCell != OBSTACLE_SYMBOL)
    {
        x = sideX;
        y = sideY;
    }
}

// try to pick up collectible items
void Player::tryPickupItem(Board& board, char cell, int newX, int newY)
{
    // if already holding an item, cannot pick up another, so return
    if (item != NO_ITEM)
        return;

    // pick up the item
    if (cell == KEY_SYMBOL || cell == BOMB_SYMBOL || cell == TORCH_SYMBOL)
    {
        // set held item for player
        item = cell;
        // remove item from board
        board.setChar(newY, newX, NOTHING_SYMBOL);
    }
}

// handles the player movement for one game cycle 
void Player::Move(Board& board, Player& other, int otherStartX, int otherStartY, char otherStartDir, int otherStartForce)
{
    // if no movement and no boost, nothing to do 
    if (direction == DIRECTION_NONE && boostTicks <= 0)
        return;

    int pushesThisMove = 0;
    const int MAX_PUSHES_PER_MOVE = 2;

    int savedBoostTicks = boostTicks;

    // steps count to move this cycle (start with normal speed)
    int steps = NORMAL_SPEED;
    // if boosted, use boost speed
    if (boostTicks > 0)
        steps = boostSpeed_;
    // if no speed, no movement 
    else if (speed <= 0 || direction == DIRECTION_NONE)
        return;
    // else just use the player's current speed
    else
        steps = speed;

    // handle sideways movement before forward movement while boosted
    handleSidewaysMovement(board, other);

    // for each step to move
    for (int step = 0; step < steps; ++step)
    {
        // save previous position
        prevX = x;
        prevY = y;

        // get the change in position based on current direction (-1,0, or 1 for dx and dy)
        int dx, dy;
        getDirectionDelta(dx, dy);

        // calculate new position
        int newX = x + dx;
        int newY = y + dy;
        // make sure new position is inside room bounds
        clampToRoom(newX, newY);

        // handle collision with other player:
        if (newX == other.GetX() && newY == other.GetY())
        {
            // if this player is boosted, apply spring impact to other player
            if (boostTicks > 0)
                other.takeSpringImpactFrom(*this);
            break;
        }

        // get cell at new position
        char cell = board.getChar(newY, newX);
        if (cell == '\0')
            break;


        // a flag to indicate if movement will be blocked
        bool blocked = false;

        // leaving spring without hitting wall - just cancel, no boost
        char curCell = board.getChar(y, x);

        if (compressingSpring && curCell == SPRING_SYMBOL && cell != SPRING_SYMBOL)
        {
            // If we're not hitting a wall/gate, release boost normally
                if (cell != WALL_SYMBOL && cell != GATE_SYMBOL)
                    applySpringBoost(false);
                else
                    applySpringBoost(true);
            newX = prevX;
            newY = prevY;
            break;
        
        }
        
        // handle different cell types:
        switch (cell)
        {
            // obstacle pushing
        case OBSTACLE_SYMBOL:
        {
            if (pushesThisMove >= MAX_PUSHES_PER_MOVE) 
            {
                blocked = true;   // player stops pushing
                break;
            }

            // find all connected obstacle cells
            bool obstacleGroup[BOARD_HEIGHT][BOARD_WIDTH];
            int obstacleSize = findObstacleGroup(board, newY, newX, obstacleGroup);

            // calculate total force (speed) applied to obstacle
            int totalForce = getForce();

            // two players pushing together - combine forces
            bool otherAdjacent = (abs(otherStartX - x) + abs(otherStartY - y) == 1);
            bool otherPushingSameDir = (otherStartDir == direction);
            if (otherAdjacent && otherPushingSameDir)
            {
                totalForce += otherStartForce;
            }

            bool canPush = canPushObstacle(board, obstacleGroup, dx, dy, &other);
            
            // if enough force to push obstacle, push it
            if ( (totalForce >= obstacleSize) && canPush)
            {
                pushObstacleGroup(board, obstacleGroup, dx, dy);
                //count the push
                pushesThisMove++;
            }
            // if not enough force, block movement
            else
                blocked = true;
            break;
            
        }


        // wall collision
        case WALL_SYMBOL:
        case GATE_SYMBOL:
            // if hitting wall while compressing spring in that direction
            if (compressingSpring && direction == springCompressedDir)
            {
                // apply spring boost with wall bounce
                applySpringBoost(true);
                newX = prevX;
                newY = prevY;
            }
            // if not compressing spring, just block movement
            else
            {
                blocked = true;
            }
            break;


            // Door interaction - Game handles all door logic
        case DOOR1_SYMBOL:
        case DOOR2_SYMBOL:
        case DOOR3_SYMBOL:
        case DOOR4_SYMBOL:
        case DOOR5_SYMBOL:
        case DOOR6_SYMBOL:
        case DOOR7_SYMBOL:
        case DOOR8_SYMBOL:
        case DOOR9_SYMBOL:
            break;

            // collectible items
        case KEY_SYMBOL:
        case BOMB_SYMBOL:
        case TORCH_SYMBOL:
            // use the function that deals with item pickup, detailed there
            tryPickupItem(board, cell, newX, newY);
            break;

            // spring interaction
        case SPRING_SYMBOL:
            // if not in a boost affect 
            if (boostTicks == 0)
            {
                // if not already compressing springs - compress, count and set direction
                if (!compressingSpring)
                {
                    compressingSpring = true;
                    springCompressed = 1;
                    springCompressedDir = direction;
                }
                else
                {
                    // increment count of compressed springs
                    springCompressed++;
                }
            }
            break;

        default:
            break;
        }


        // if movement is blocked, then clear boost and stop movement
        if (blocked)
        {
            if (boostTicks > 0)
            {
                boostTicks = 0;
                boostSpeed_ = NORMAL_SPEED;
                speed = (direction == DIRECTION_NONE ? NO_SPEED : NORMAL_SPEED);
                sideDirection = DIRECTION_NONE;
            }
            break;
        }

        //update position
        x = newX;
        y = newY;

        
    }


    // handle boost decay
    if (savedBoostTicks > 0 && boostTicks > 0)
    {
        boostTicks--;
        if (boostTicks == 0)
        {
            speed = (direction == DIRECTION_NONE ? NO_SPEED : NORMAL_SPEED);
            sideDirection = DIRECTION_NONE;
        }
    }
}

// change movement direction based on key input
void Player::changeDirection(char key)
{
    char newDir = DIRECTION_NONE;
    bool stayPressed = false;

    // A note here about the the magic charts: this was done because player keys dont change so we dont really need constants for them
    // Player 1: W,A,S,D,X
    // Player 2: I,J,K,L,M

    if (key == 'w' || key == 'W' || key == 'i' || key == 'I')
        newDir = DIRECTION_UP;
    else if (key == 'x' || key == 'X' || key == 'm' || key == 'M')
        newDir = DIRECTION_DOWN;
    else if (key == 'a' || key == 'A' || key == 'j' || key == 'J')
        newDir = DIRECTION_LEFT;
    else if (key == 'd' || key == 'D' || key == 'l' || key == 'L')
        newDir = DIRECTION_RIGHT;
    else if (key == 's' || key == 'S' || key == 'k' || key == 'K')
        stayPressed = true; // stay in place key
    else
        return;

    // cancelling spring compression
    if (compressingSpring)
    {
        // if we are trying to change direction away from the spring compression direction
        if (stayPressed || (newDir != DIRECTION_NONE && newDir != springCompressedDir))
        {
            // cancel spring compression
            applySpringBoost(true);
            return;
        }
    }

    // if currently boosted, we need to block attempts to change direction that would reverse or conflict 
    if (boostTicks > 0)
    {
        // if trying to stay in place, don't allow
        if (stayPressed || newDir == DIRECTION_NONE)
            return;

        // prevent reversing direction
        if ((direction == DIRECTION_UP && newDir == DIRECTION_DOWN) ||
            (direction == DIRECTION_DOWN && newDir == DIRECTION_UP) ||
            (direction == DIRECTION_LEFT && newDir == DIRECTION_RIGHT) ||
            (direction == DIRECTION_RIGHT && newDir == DIRECTION_LEFT))
            return;

        //if new direction is same as current, end here
        if (newDir == direction)
            return;

        // allow changing sideways direction only
        bool currentVertical = (direction == DIRECTION_UP || direction == DIRECTION_DOWN);
        bool newHorizontal = (newDir == DIRECTION_LEFT || newDir == DIRECTION_RIGHT);
        bool currentHorizontal = (direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT);
        bool newVertical = (newDir == DIRECTION_UP || newDir == DIRECTION_DOWN);

        if ((currentVertical && newHorizontal) || (currentHorizontal && newVertical))
            sideDirection = newDir;

        return;
    }

    // Normal movement change handling:

    // if staying in place
    if (stayPressed)
    {
        direction = DIRECTION_NONE;
        speed = NO_SPEED;
    }
    // if changing to a new direction
    else if (newDir != DIRECTION_NONE)
    {
        direction = newDir;
        speed = NORMAL_SPEED;
    }

    
}

// use bomb at current position
bool Player::useBomb(Board& board)
{
    if (item != BOMB_SYMBOL)
        return false;

    // place bomb at current position
    board.setChar(y, x, BOMB_SYMBOL);
    // remove bomb from player inventory
    item = NO_ITEM;
    // indicate bomb was deployed
    return true;
}

// reset player to given start position (for room transitions, bomb hit, etc.)
void Player::ResetToStart(int startX, int startY)
{
    // reset position
    x = startX;
    y = startY;

    // reset also other attributes of player that are necessary
    prevX = x;
    prevY = y;

    // stop movement
    direction = DIRECTION_NONE;
    speed = NO_SPEED;

    // player spring attributes reset
    boostTicks = 0;
    boostSpeed_ = NORMAL_SPEED;
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;
    m_pushesLeftInBoost = 0;
}

// return player boosted state (if boosted or not)
bool Player::isBoosting() const
{
    return boostTicks > 0;
}

// apply spring impact from another player (used when colliding with boosted player)
void Player::takeSpringImpactFrom(const Player& other)
{
    // apply boost from other player
    boostSpeed_ = other.boostSpeed_;
    boostTicks = other.boostTicks;
    direction = other.direction;
    speed = other.boostSpeed_;

    // this lines came from chat-gpt suggestion, because of unexpected behavior
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;
}

// clear the boost affect from the player
void Player::clearBoost()
{
    boostTicks = 0;
    boostSpeed_ = NORMAL_SPEED;
    speed = (direction == DIRECTION_NONE ? NO_SPEED : NORMAL_SPEED);
    sideDirection = DIRECTION_NONE;
    m_pushesLeftInBoost = 0;
}

// drop currently held item onto the board at position set in the code (inside the function) according to player position
void Player::dropItem(Board& board)
{
    // if no item held, nothing to drop
    if (item == NO_ITEM)
        return;

    // try to drop item in adjacent cell, try in this order: up, down, left, right
    if (board.getChar(y - 1, x) == NOTHING_SYMBOL)
        board.setChar(y - 1, x, item);
    else if (board.getChar(y + 1, x) == NOTHING_SYMBOL)
        board.setChar(y + 1, x, item);
    else if (board.getChar(y, x - 1) == NOTHING_SYMBOL)
        board.setChar(y, x - 1, item);
    else if (board.getChar(y, x + 1) == NOTHING_SYMBOL)
        board.setChar(y, x + 1, item);
    // if no place aviable to drop, do nothing
    else
        return;

    // clear held item from player inventory
    item = NO_ITEM;
}


// simple getters:
char Player::getItem() const { return item; }
int Player::GetX() const { return x; }
int Player::GetY() const { return y; }
char Player::GetSymbol() const { return symbol; }
char Player::getDirection() const { return direction; }

void Player::setItem(char newItem) { item = newItem; }


// step back to previous cell (used when hitting wall while boosting)
void Player::stepBackToPrevCell()
{
    // set player position to previous position (before last move)
    x = prevX;
    y = prevY;
    // stop all movement
    stopMovement();
    // clear boost affect
    clearBoost();
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;
    m_pushesLeftInBoost = 0;
}

void Player::stopMovement()
{
    direction = DIRECTION_NONE;
    speed = NO_SPEED;
}

void Player::consumeKey()
{
    if (item == KEY_SYMBOL)
    {
        item = NO_ITEM;
    }
}

// remove player from board (used when transitioning rooms, when entering the door)
// chat-gpt helped with this function implementation because of unexpected behavior earlier but my logic is still here
void Player::removeFromBoard()
{
    x = 0;
    y = 0;
    prevX = 0;
    prevY = 0;
    stopMovement();
    clearBoost();
    compressingSpring = false;
    springCompressed = 0;
    springCompressedDir = DIRECTION_NONE;
    sideDirection = DIRECTION_NONE;
    m_pushesLeftInBoost = 0;
}

bool Player::hasTorch() const
{
    return Torch::isTorch(item);
}

void Player::loseLife()
{
    life -= 1;
}

int Player::getLife()const
{
    return life;
}