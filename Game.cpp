#include "Game.h"
#include "ConsoleUtils.h"
#include <conio.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <format>
#include <filesystem>
#include "SwitchAndGate.h"
#include "Torch.h"

// Game constructor, initializes players before 
Game::Game(RunMode m) : p1(P1_SYMBOL), p2(P2_SYMBOL), mode(m)
{
    silent = (mode == RunMode::LoadSilent);
    init();
}

void Game::init()
{
    // reset game state
    currentRoom = 0;
    p1WaitingNextRoom = false;
    p2WaitingNextRoom = false;
    theOtherPlayerDoor = nullptr;
    inLastRoom = false;
    gameIsOver = false;
    gameTime = 0;

    // bombs are reset by default (inactive)
    bombs[0] = Bomb();
    bombs[1] = Bomb();

    // reset doors
    doorCount = 0;
    for (int i = 0; i < MAX_DOORS; i++)
        doors[i] = Door();

    running = true;

    // set starting positions for players in each room
    initStartPositions();

    // create empty boards
    for (int i = 0; i < NUM_ROOMS; ++i)
        boards[i] = Board();

    // build room layouts
    buildRoomsLayouts();

    //load riddles file
    loadRiddlesFromFile("Riddles.txt");
    
    // setup doors
    setupDoors();


    // initialize players
    p1 = Player(P1_SYMBOL);
    p2 = Player(P2_SYMBOL);

    // reset players to starting positions in room 0
    p1.ResetToStart(p1Starts[0].x, p1Starts[0].y);
    p2.ResetToStart(p2Starts[0].x, p2Starts[0].y);

    // init previous positions for switch logic
    p1PrevX = p1.GetX();
    p1PrevY = p1.GetY();
    p2PrevX = p2.GetX();
    p2PrevY = p2.GetY();

    //initiallize tile under player
    p1Under = boards[currentRoom].getChar(p1.GetY(), p1.GetX());
    p2Under = boards[currentRoom].getChar(p2.GetY(), p2.GetX());

    hideCursor();

    // Initialize message system
    message = "";
    messageTimer = 0;
    if (mode!=RunMode::LoadSilent)
        showMessage("Welcome! Find the exit together.");
    roomInfoDirty = true;
}


// starts the main of the game
void Game::run()
{
    //if game mode is load than there is no menu
    if (mode == RunMode::Load || mode == RunMode::LoadSilent) 
    {
        openFilesForMode();
        init();
        loadStepsFile();
        actualResults.clear();
        expectedResults.clear();
        if (mode == RunMode::LoadSilent) 
        {
            loadExpectedResultFile();
            actualOut.open("adv-world.actualResult.txt", std::ios::out | std::ios::trunc);//new

        }
        gameLoop();

        if (mode == RunMode::LoadSilent) {
            // compare and report
            size_t i = 0;
            while (i < expectedResults.size() && i < actualResults.size() && expectedResults[i] == actualResults[i]) 
            {
                ++i;
            }

            if (i == expectedResults.size() && i == actualResults.size()) 
            {
                std::cout << "TEST PASSED\n";
            }
            else 
            {
                std::cout << "TEST FAILED\n";
                std::cout << "First mismatch at line " << i << ":\n";
                std::cout << "Expected: " << (i < expectedResults.size() ? expectedResults[i] : "<no more lines>") << "\n";
                std::cout << "Actual:   " << (i < actualResults.size() ? actualResults[i] : "<no more lines>") << "\n";
            }
        }
        if (actualOut.is_open())
            actualOut.close();
        return;
    }

    bool exitGame = false;

    while (!exitGame)
    {
        clearScreen();
        PrintWelcomeText();

        char choice = _getch();

        if (choice == START_GAME_CHOICE)
        {
            init();
            openFilesForMode();
            gameLoop();
        }
        else if (choice == INSTRUCTIONS_CHOICE)
        {
            showInstructions();
        }
        else if (choice == EXIT_CHOICE)
        {
            exitGame = true;
        }
    }
}

void Game::PrintWelcomeText()
{
    std::cout << "=== TEXT ADVENTURE WORLD ===" << std::endl << std::endl;
    std::cout << "(1) Start a new game" << std::endl;
    std::cout << "(8) Instructions and keys" << std::endl;
    std::cout << "(9) EXIT" << std::endl << std::endl;
    std::cout << "Choose: ";
}

// display instructions screen
void Game::showInstructions()
{
    clearScreen();
    std::cout << "=== INSTRUCTIONS ===" << std::endl << std::endl;
    std::cout << "Two players must cooperate to escape!" << std::endl << std::endl;
    std::cout << "Player 1 [$]          Player 2 [&]" << std::endl;
    std::cout << "  W = up                I = up" << std::endl;
    std::cout << "  A = left              J = left" << std::endl;
    std::cout << "  S = stay              K = stay" << std::endl;
    std::cout << "  D = right             L = right" << std::endl;
    std::cout << "  X = down              M = down" << std::endl;
    std::cout << "  E = use item          O = use item" << std::endl << std::endl;
    std::cout << "Elements:" << std::endl;
    std::cout << "  W = Wall    * = Obstacle (push it!)" << std::endl;
    std::cout << "  # = Spring  @ = Bomb" << std::endl;
    std::cout << "  K = Key     1-9 = Doors" << std::endl;
    std::cout << "  / = Switch ON    \\ = Switch OFF" << std::endl << std::endl;
    std::cout << "ESC = pause game" << std::endl << std::endl;
    std::cout << "Press any key to return...";
    char ignore = _getch();
}

// handles one cycle of the game
void Game::gameLoop()
{
    if (!silent)
    {
        clearScreen();
        printScreen();
    }

    while (running)
    {
        if ((mode == RunMode::Load || mode == RunMode::LoadSilent) && gameTime > lastStepTime + 500)
            break;
        if ((mode == RunMode::Normal || mode == RunMode::Save) && !running)
            break;

        gameTime++;

        int player1X = p1.GetX(); int player1Y = p1.GetY();
        int player2X = p2.GetX(); int player2Y = p2.GetY();
        //the tiles under players before moving
        p1PrevUnder = p1Under;
        p2PrevUnder = p2Under;

        bool toContinue = boolHandleInput();
        if (toContinue)
            continue;

        // Save items before move to detect pickup
        char p1ItemBefore = p1.getItem();
        char p2ItemBefore = p2.getItem();

        int p1StartX = p1.GetX(), p1StartY = p1.GetY();
        int p2StartX = p2.GetX(), p2StartY = p2.GetY();
        char p1Dir = p1.getDirection();
        char p2Dir = p2.getDirection();
        int p1ForceStart = p1.getForce();
        int p2ForceStart = p2.getForce();

        // move players, only if they are not waiting at door to next room
        if (!p1WaitingNextRoom)
            p1.Move(boards[currentRoom], p2, p2StartX, p2StartY, p2Dir, p2ForceStart);
        if (!p2WaitingNextRoom)
            p2.Move(boards[currentRoom], p1, p1StartX, p1StartY, p1Dir, p1ForceStart);

        // after movement, update under tiles for new position
        p1Under = boards[currentRoom].getChar(p1.GetY(), p1.GetX());
        p2Under = boards[currentRoom].getChar(p2.GetY(), p2.GetX());

        tryPickup(p1, p1Under);
        tryPickup(p2, p2Under);
        
        boards[currentRoom].updateSwitchesAndGates(p1.GetX(), p1.GetY(), p2.GetX(), p2.GetY());

        // Switches/gates can change the tile under players -> refresh caches
        p1Under = boards[currentRoom].getChar(p1.GetY(), p1.GetX());
        p2Under = boards[currentRoom].getChar(p2.GetY(), p2.GetX());

        if (!p1WaitingNextRoom) handleRiddleIfNeeded(p1);
        if (!p2WaitingNextRoom) handleRiddleIfNeeded(p2);


        // Update previous positions for drawing next frame
        p1PrevX = player1X;
        p1PrevY = player1Y;
        p2PrevX = player2X;
        p2PrevY = player2Y;

        UpdateHeaderOnItemPickUp(p1ItemBefore, p1);
        UpdateHeaderOnItemPickUp(p2ItemBefore, p2);

        // update bombs (countdown and explode)
        updateBombs();

        // door and room logic
        if (isPlayerAtDoor(p1))
            HandlePlayerAtDoor(p1,true);

        if (isPlayerAtDoor(p2))
            HandlePlayerAtDoor(p2,false);

        if (areBothPlayersAtDoor())
            HandleBothPlayersAtNextRoom();

        if (!silent) 
            printScreen();
        handlePlayersDeath(p1, "Player 1");
        handlePlayersDeath(p2, "Player 2");
        
        
        if (inLastRoom && !gameIsOver)
        {
            recordToResult(ResultEvent::GameEnd, "", -1, "", "", false);
            gameIsOver = true;

            if (mode == RunMode::Normal || mode == RunMode::Save)
            {
                running = false;
            }
        }

        if (!silent)
            Sleep(55);
    }
    running = false;
}


void Game::UpdateHeaderOnItemPickUp(char ItemBefore, Player& player)
{
    char playerCurItem = player.getItem();
    char playerChar = player.GetSymbol();
    // Check if items were picked up
    if (playerCurItem != ItemBefore && playerCurItem != ' ')
    {
        //update result file
        std::string who = (player.GetSymbol() == P1_SYMBOL) ? "P1" : "P2";
        recordToResult(ResultEvent::ItemPickup,who,-1,"",std::string(1, playerCurItem));
        std::string message = std::format("Player {} picked up an item!", playerChar);
        showMessage(message);
        PrintPlayerInfoInHeader();
    }
}

bool Game::boolHandleInput()
{
    //if the game is in load mode thers no input
    if (mode == RunMode::Load || mode == RunMode::LoadSilent) {
        useStepsFromStepsFile(); // from steps file
        return false;
    }

    // handle input
    if (_kbhit())
    {
        char ch = _getch();
        if (ch == KEY_ESC)
        {
            if (!handlePause())
            {
                return true;
            }
        }
        else
        {
            processInput(ch);
        }
    }
    return false;
}

bool Game::handlePause()
{
    gotoxy(0, BOARD_HEIGHT);
    std::cout << "PAUSED - ESC to continue, H for menu               ";

    while (true)
    {
        char ch = _getch();
        if (ch == KEY_ESC)
        {
            gotoxy(0, BOARD_HEIGHT);
            std::cout << "                                          ";
            return true;
        }
        else if (ch == 'h' || ch == 'H')
        {
            running = false;
            return false;
        }
    }
}

void Game::processInput(char ch)
{
    if (inLastRoom)
        return;

    // Player 1 movement
    if (ch == 'w' || ch == 'W' || ch == 'a' || ch == 'A' ||
        ch == 's' || ch == 'S' || ch == 'd' || ch == 'D' || ch == 'x' || ch == 'X')
    {
        p1.changeDirection(ch);
        if (mode == RunMode::Save)
        {
            stepsOut << gameTime << " P1 DIR " << ch << "\n";
            stepsOut.flush();
        }

    }
    // Player 1 action
    else if (ch == 'e' || ch == 'E')
    {
        playerUseItem(p1, 0);
        if (mode == RunMode::Save) 
        {
            stepsOut << gameTime << " P1 USE\n";
            stepsOut.flush();
        }

    }
    // Player 2 movement
    else if (ch == 'i' || ch == 'I' || ch == 'j' || ch == 'J' ||
        ch == 'k' || ch == 'K' || ch == 'l' || ch == 'L' || ch == 'm' || ch == 'M')
    {
        p2.changeDirection(ch);
        if (mode == RunMode::Save)
        {
            stepsOut << gameTime << " P2 DIR " << ch << "\n";
            stepsOut.flush();
        }
    }
    // Player 2 action
    else if (ch == 'o' || ch == 'O')
    {
        playerUseItem(p2, 1);
        if (mode == RunMode::Save)
        {
            stepsOut << gameTime << " P2 USE\n";
            stepsOut.flush();
        }
    }
}

void Game::playerUseItem(Player& player, int playerIndex)
{
    char item = player.getItem();
    
    if (item == BOMB_SYMBOL)
    {
        if (player.useBomb(boards[currentRoom]))
        {
            bombs[playerIndex].activate(player.GetX(), player.GetY());
            player.forceSetItem(' ');
            p1Under = ' ';
            showMessage("Bomb planted! Run!");
            PrintPlayerInfoInHeader();
        }
    }
    else if (item != ' ')
    {
        player.dropItem(boards[currentRoom]);
    }
}

//update both bombs each cycle
void Game::updateBombs()
{
    for (int i = 0; i < 2; i++)
    {
        if (bombs[i].tick(boards[currentRoom]))
        {
            // explosion happened - check if players got hit
            int bx = bombs[i].getX();
            int by = bombs[i].getY();

            // check player 1
            int dx1 = abs(p1.GetX() - bx);
            int dy1 = abs(p1.GetY() - by);
            if (dx1 <= 3 && dy1 <= 3)
            {
                //if player got hit -> his life reduced by 1
                p1.loseLife();
                recordToResult(ResultEvent::LifeLost, "P1");
                p1.ResetToStart(p1Starts[currentRoom].x, p1Starts[currentRoom].y);
            }

            // check player 2
            int dx2 = abs(p2.GetX() - bx);
            int dy2 = abs(p2.GetY() - by);
            if (dx2 <= 3 && dy2 <= 3)
            {
                //if player got hit -> his life reduced by 1
                p2.loseLife();
                recordToResult(ResultEvent::LifeLost, "P2");
                p2.ResetToStart(p2Starts[currentRoom].x, p2Starts[currentRoom].y);
            }
        }
    }
}

void Game::drawPlayers()
{
    // Draw player 1
    if (!p1WaitingNextRoom)
    {
        int p1X = p1.GetX();
        int p1Y = p1.GetY();
        //restore what was under P1 at previous position
        boards[currentRoom].setChar(p1PrevY, p1PrevX, p1PrevUnder);
        p1Under = boards[currentRoom].getChar(p1Y, p1X);
        //draw P1
        boards[currentRoom].setChar(p1Y, p1X, p1.GetSymbol());
    }

    // Draw player 2
    if (!p2WaitingNextRoom)
    {
        int p2X = p2.GetX();
        int p2Y = p2.GetY();
        boards[currentRoom].setChar(p2PrevY, p2PrevX, p2PrevUnder);
        p2Under = boards[currentRoom].getChar(p2Y, p2X);
        //draw P2
        boards[currentRoom].setChar(p2Y, p2X, p2.GetSymbol());
    }
}

void Game::PrintPlayerInfoInHeader()
{
    if (silent)
        return;
    char p1Item = p1.getItem();
    char p2Item = p2.getItem();
    //print players, lives and room number
    gotoxy(2, 0);
    std::cout << "Player1 Life: " << p1.getLife();

    // Room number in center
    gotoxy(35, 0);
    std::cout << "Room " << (currentRoom + 1);

    gotoxy(55, 0);
    std::cout << "Player2 Life: "<< p2.getLife();

    //print items
    gotoxy(2, 1);
    std::cout << "Item P1: " << (p1Item == ' ' ? '-' : p1Item);

    gotoxy(55, 1);
    std::cout << "Item P2: " << (p2Item == ' ' ? '-' : p2Item);

    //print score
    gotoxy(35, 1);
    std::cout << "Score: " << (getScore());

    // Row 3: message
    PrintMessageInHeader();
}

void Game::PrintMessageInHeader()
{
    if (silent)
        return;
    // --- Message System Display ---
    // Print on line 3 (inside the frame, below the status line)
    gotoxy(20, 2);

    if (messageTimer > 0)
    {
        std::cout << message;
        messageTimer--;
    }
    else
    {
        // Clear the message area if timer is done
        std::cout << "                                         ";
    }
}

void Game::initStartPositions()
{
    // room 0
    p1Starts[0] = { 3, 7 };
    p2Starts[0] = { 3, 8 };
    // room 1 
    p1Starts[1] = { 2, 20 };
    p2Starts[1] = { 77, 20 };
    // room 2  
    p1Starts[2] = { 5, 8 };
    p2Starts[2] = { 5, 9 };
}


// ============================================================
// ROOM LAYOUTS - This is where the puzzles are defined
// ============================================================


void Game::buildRoomsLayouts()
{
    // Load each room layout from Board
    for (int i = 0; i < NUM_ROOMS; ++i)
    {

        std::string filename = "adv-world" + std::to_string(i) + ".screen.txt";

        if (!boards[i].loadRoomFromFile(filename))
        {
            if (!silent)
            {
                std::cout << "Cannot load " << filename << "\n";
            }
            return;
        }
    }
}


// Setup doors - define which rooms they appear in and where they lead
void Game::setupDoors()
{
    // Door 2: appears in room 0, leads to room 1 (index 1)
    doors[0].addPosition(0, 53, 17);
    doors[0].setTargetRoom(1);

    // Door 1: appears in room 1, leads back to room 0 (or wherever you want)
    doors[1].addPosition(1, 28, 16);
    doors[1].setTargetRoom(0);  // goes back to room 0


    // Door 3: appears in room 1, leads to room 2 (index 2)
    doors[2].addPosition(1, 52, 16);
    doors[2].setTargetRoom(2);

    // Door 4: appears in room 2, leads to room 3 (index 3)
    doors[3].addPosition(2, 67, 17);
    doors[3].setTargetRoom(3);// goes to victory room

    doorCount = 4;
}



// Find door at given position in current room
Door* Game::getDoorAtPosition(int room, int x, int y)
{
    for (int i = 0; i < doorCount; i++)
    {
        if (doors[i].isAtPosition(room, x, y))
            return &doors[i];
    }
    return nullptr;
}




bool Game::isPlayerAtDoor(Player& player)
{
    return getDoorAtPosition(currentRoom,player.GetX(),player.GetY()) != nullptr;
}

//bool Game::isPlayerAtDoor(Player& player)
//{
//    int playerX = player.GetX();
//    int playerY = player.GetY();
//    
//    if (playerX < 0 || playerX >= BOARD_WIDTH || playerY < 0 || playerY >= BOARD_HEIGHT)
//        return false;
//
//    char charOnBoard = boards[currentRoom].getChar(playerY, playerX);
//    return (charOnBoard >= '1' && charOnBoard <= '9');
//}

void Game::HandleBothPlayersAtNextRoom()
{

    int nextRoom = theOtherPlayerDoor->getTargetRoom();

    if (nextRoom >= 0 && nextRoom < NUM_ROOMS)
    {
        currentRoom = nextRoom;
        //update result file
        recordToResult(ResultEvent::ScreenChange, "BOTH", currentRoom);
        roomInfoDirty = true;
        if (!silent)
            printScreen();
        p1WaitingNextRoom = false;
        p2WaitingNextRoom = false;
        theOtherPlayerDoor = nullptr;

        if (currentRoom == NUM_ROOMS - 1)
            inLastRoom = true;

        p1.ResetToStart(p1Starts[currentRoom].x, p1Starts[currentRoom].y);
        p2.ResetToStart(p2Starts[currentRoom].x, p2Starts[currentRoom].y);

        p1PrevX = p1.GetX();
        p1PrevY = p1.GetY();
        p2PrevX = p2.GetX();
        p2PrevY = p2.GetY();

        bombs[0] = Bomb();
        bombs[1] = Bomb();
        //refresh the tiles under the players for the new room
        p1Under = boards[currentRoom].getChar(p1PrevY, p1PrevX);
        p2Under = boards[currentRoom].getChar(p2PrevY, p2PrevX);
        p1PrevUnder = p1Under;
        p2PrevUnder = p2Under;
    }



}
void Game::printScreen()
{
    PrintPlayerInfoInHeader();
    drawPlayers();
    gotoxy(0, BOARD_TOP);
    bool illuminate = p1.hasTorch() || p2.hasTorch();
    boards[currentRoom].printBoardMasked(illuminate);
   bool isDark = boards[currentRoom].getIsDarkRoom();
    if (roomInfoDirty || isDark != lastWasDark || illuminate != lastIlluminated)
    {
        printRoomInfo(illuminate);
        lastWasDark = isDark;
        lastIlluminated = illuminate;
        roomInfoDirty = false;
    }
}

bool Game::areBothPlayersAtDoor()
{
    if (p1WaitingNextRoom && p2WaitingNextRoom && theOtherPlayerDoor != nullptr)
        return true;
    return false;
}
void Game::HandlePlayerAtDoor(Player& player, bool isPlayer1)
{

    Door* door = getDoorAtPosition(currentRoom, player.GetX(), player.GetY());
    if (door != nullptr)
    {
        if (!door->getIsOpen())
        {
            // door locked
            if (player.getItem() == KEY_SYMBOL)
            {
                // has key - use it, open door, but push player back
                player.consumeKey();
                PrintPlayerInfoInHeader();
                door->open();
                showMessage("Door unlocked!");
            }
            else
            {
                showMessage("Door is locked. You need a key!");
                // push player back
                player.stepBackToPrevCell();
                player.stopMovement();
            }
        }
        if (door->getIsOpen())
        {
            // door open - player enters
            if (isPlayer1)
                p1WaitingNextRoom = true;
            else
                p2WaitingNextRoom = true;
            player.removeFromBoard();
            theOtherPlayerDoor = door;
            showMessage("Waiting for other player...");
        }
    }

}
void Game::clearMessage()
{
    if (silent)
        return;
    gotoxy(20, 2);
    std::cout << "                                         ";
}
void Game::showMessage(const std::string& msg)
{
    if (silent)
        return;
    clearMessage();
    message = msg;
    messageTimer = 30; // Display for ~2 seconds
    PrintMessageInHeader();
}

// Return true if player was blocked (wrong answer), false otherwise
bool Game::handleRiddleIfNeeded(Player& player)
{
    int x = player.GetX();
    int y = player.GetY();

    if (boards[currentRoom].getChar(y, x) != RIDDLE_SYMBOL)
        return false;

    //reset riddle score
    RiddleScore = 200;

    std::string who = (player.GetSymbol() == P1_SYMBOL) ? "P1" : "P2";
    bool solved = askRiddle(who);
    recordToResult(ResultEvent::Riddle, who, -1, "Q" + std::to_string(nextRiddleIndex), "-", solved);


    if (solved)
    {
        boards[currentRoom].setChar(y, x, ' ');
        //make sure the tile under is updated
        if (player.GetSymbol() == P1_SYMBOL) 
            p1Under = ' ';
        else
            p2Under = ' ';
        showMessage("Riddle solved!");
        updateScore(true);
    }
    else
    {
        player.stepBackToPrevCell();
        player.stopMovement();
        showMessage("Wrong answer!");
        player.loseLife();
        updateScore(false);
    }

    return true;
}


bool Game::askRiddle(const std::string& who)
{
    if (nextRiddleIndex >= riddleCount)
        return true;

    McqRiddle& r = riddles[nextRiddleIndex];
    bool correct = askMcqRiddle(r,who);

    if (correct)
        nextRiddleIndex++;

    return correct;
}


bool Game::askMcqRiddle(const McqRiddle& r, const std::string& who)
{
    if (!silent)
    {
        gotoxy(0, RIDDLE_LINE);
        std::cout << "RIDDLE: ";
        gotoxy(0, RIDDLE_LINE + 1);
        std::cout << r.question;

        gotoxy(0, RIDDLE_LINE + 3);
        std::cout << "1) " << r.options[0];

        gotoxy(0, RIDDLE_LINE + 4);
        std::cout << "2) " << r.options[1];

        gotoxy(0, RIDDLE_LINE + 5);
        std::cout << "3) " << r.options[2];

        gotoxy(0, RIDDLE_LINE + 6);
        std::cout << "4) " << r.options[3];

        gotoxy(0, RIDDLE_LINE + 2);
        std::cout << "Choose 1-4: ";
    }

    char ch = 0;
    // save mode or normal mode: read from keyboard
    if (mode == RunMode::Normal || mode == RunMode::Save)
    {
        while (true)
        {
            ch = _getch();
            if (ch >= '1' && ch <= '4')
                break;
        }

        //record riddle answer into steps file
        if (mode == RunMode::Save)
        {
            stepsOut << gameTime << " " << who << " RIDDLE " << ch << "\n";
            stepsOut.flush();
        }
    }
    else
    {
        // LOAD: take answer from steps file (we’ll implement this below)
        ch = getRiddleAnswerFromSteps(who); // returns '1'..'4'
    }
    
    int chosen = (ch - '1');
    bool correct = (chosen == r.correctIndex);

    if (!silent)
    {
        gotoxy(0, RIDDLE_LINE + 7);
        if (correct)
            std::cout << "Correct!";
        else
        {
            std::cout << "Wrong!";

        }

        Sleep(900);

        // Clear riddle area
        for (int i = 0; i < 8; ++i)
        {
            gotoxy(0, RIDDLE_LINE + i);
            std::cout << "                                                                                ";
        }
    }

    return correct;
}

void Game::loadRiddlesFromFile(const std::string& filename)
{
    riddleCount = 0;
    nextRiddleIndex = 0;

    std::ifstream in(filename);
    if (!in)
    {
        std::cout << "Failed to open riddles file: " << filename << "\n";
        return;
    }

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;

        if (riddleCount >= MAX_RIDDLES)
            break;

        McqRiddle r;

        // Question
        r.question = line;

        // A) .. D)
        for (int i = 0; i < 4; ++i)
        {
            std::getline(in, line);
            r.options[i] = (line.size() >= 3) ? line.substr(3) : "";
        }

        // "Correct answer: X"
        std::getline(in, line);
        if (!line.empty())
        {
            char correctChar = line.back();
            r.correctIndex = correctChar - 'A';
        }
        else
        {
            r.correctIndex = 0;
        }
        riddles[riddleCount++] = r;
    }
}

void Game::printRoomInfo(bool illuminate)
{
    // Clear exactly 2 info lines
    for (int i = 0; i < 2; ++i)
    {
        gotoxy(0, ROOM_INFO + i);
        std::cout << "                                                                                ";
    }

    if (boards[currentRoom].getIsDarkRoom())
    {
        gotoxy(0, ROOM_INFO);
        std::cout << "This is a dark room.";

        //gotoxy(0, ROOM_INFO + 1);
        if (!illuminate)
        {
            gotoxy(0, ROOM_INFO + 1);
            std::cout << "Find the torch (!) to see the room and solve riddles.";
            gotoxy(0, ROOM_INFO + 2);
            std::cout << "Collect the required items to solve the room.";
            gotoxy(0, ROOM_INFO + 3);
            std::cout << "Switches: OFF=\\  ON=/   (open gates by matching door number in bits)";
        }
        else
        {
            gotoxy(0, ROOM_INFO + 1);
            std::cout << "Torch is active - you can see the room and solve riddles.";
            gotoxy(0, ROOM_INFO + 2);
            std::cout << "Collect the required items to solve the room.";
            gotoxy(0, ROOM_INFO + 3);
            std::cout << "Switches: OFF=\\  ON=/   (open gates by matching door number in bits)";
        }

    }
    else
    {
        gotoxy(0, ROOM_INFO);
        std::cout << "Collect the required items to solve the room.";

        gotoxy(0, ROOM_INFO + 1);
        std::cout << "Switches: OFF=\\  ON=/   (open gates by matching door number in bits)";
    }
}

void Game::tryPickup(Player& pl, char& underTile)
{
    if (pl.getItem() != ' ') return;

    // if it's a bomb char but it's actually a planted bomb - don't pick it up
    if (underTile == BOMB_SYMBOL && isBombPlantedHere(pl.GetX(), pl.GetY()))
        return;
    if (underTile == KEY_SYMBOL || underTile == BOMB_SYMBOL || underTile == TORCH_SYMBOL)
    {
        char item = underTile;
        pl.forceSetItem(item);
        underTile = ' ';   // remove from world

        std::string who = (pl.GetSymbol() == P1_SYMBOL) ? "P1" : "P2";
        recordToResult(ResultEvent::ItemPickup, who, -1, "", std::string(1, item));
    }
}

bool Game::isBombPlantedHere(int x, int y) const
{
    for (int i = 0; i < 2; i++)
    {
        if (bombs[i].isActive() && bombs[i].getX() == x && bombs[i].getY() == y)
            return true;
    }
    return false;
}


void Game::openFilesForMode() {
    silent = (mode == RunMode::LoadSilent);

    if (mode == RunMode::Save) {
        stepsOut.open("adv-world.steps.txt", std::ios::out | std::ios::trunc);
        resultOut.open("adv-world.result.txt", std::ios::out | std::ios::trunc);

        stepsOut << "0 INIT\n";
        resultOut << "0 INIT\n";
        
        stepsOut << "FILES ";
        for (int i = 0; i < NUM_ROOMS; i++) stepsOut << "adv-world" << i << ".screen.txt ";
        stepsOut << "\n";

        resultOut << "FILES ";
        for (int i = 0; i < NUM_ROOMS; i++) resultOut << "adv-world" << i << ".screen.txt ";
        resultOut << "\n";

        stepsOut.flush();
        resultOut.flush();
    }
}


void Game::loadStepsFile()
{
    std::ifstream in("adv-world.steps.txt");
    if (!in) {
        std::cout << "Failed to open adv-world.steps.txt\n";
        running = false;
        return;
    }

    steps.clear();
    nextStepIndex = 0;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;

        // skip header
        if (line.rfind("FILES", 0) == 0) continue;
        if (line.rfind("0 INIT", 0) == 0) continue;

        std::istringstream iss(line);

        StepEvent e{};
        std::string playerTok, actionTok;
        iss >> e.time >> playerTok >> actionTok;

        if (!iss) continue;

        e.player = (playerTok == "P1") ? 1 : 2;

        if (actionTok == "DIR") 
        {
            e.action = 'D';
            iss >> e.value;      // direction key
        }
        else if (actionTok == "USE") 
        {
            e.action = 'U';
            e.value = '-';       // placeholder
        }
        else if (actionTok == "RIDDLE") 
        {
            e.action = 'R';
            iss >> e.value; // '1'..'4'
        }
        else {
            continue;
        }

        steps.push_back(e);
    }
    lastStepTime = steps.empty() ? 0 : steps.back().time;
}

char Game::getRiddleAnswerFromSteps(const std::string& who)
{
    int playerId = (who == "P1") ? 1 : 2;

    // scan forward until we find the next riddle for this player
    for (size_t i = nextStepIndex; i < steps.size(); ++i)
    {
        if (steps[i].time != gameTime) break;

        if (steps[i].player == playerId && steps[i].action == 'R')
        {
            char ans = steps[i].value;
            nextStepIndex = i + 1; // consume it
            return ans;
        }
    }

    // If missing, fail safely (or default)
    return '1';
}
void Game::useStepsFromStepsFile()
{
    while (nextStepIndex < steps.size() && steps[nextStepIndex].time == gameTime)
    {
        StepEvent& e = steps[nextStepIndex];

        //riddle answer is read in askMcqRiddle()
        if (e.action == 'R')
            break;

        Player& pl = (e.player == 1) ? p1 : p2;
        std::string who = (e.player == 1) ? "P1" : "P2";

        if (e.action == 'D')
        {
            pl.changeDirection(e.value);
        }
        else if (e.action == 'U')
        {
            playerUseItem(pl, (e.player == 1) ? 0 : 1);
        }
        
         nextStepIndex++;
    }
}



void Game::recordToResult(ResultEvent ev, const std::string& player, int screenIndex, const std::string& question, const std::string& answer, bool correct)
{
    std::ostringstream resultOutStr;

    switch (ev)
    {
    case ResultEvent::ScreenChange:
        // SCREEN_CHANGE <player> <screen_index>
        resultOutStr << gameTime << " SCREEN_CHANGE " << player << " " << screenIndex; // << "\n";
        break;

    case ResultEvent::LifeLost:
        // LIFE_LOST <player>
        resultOutStr << gameTime << " LIFE_LOST " << player;// << "\n";
        break;

    case ResultEvent::Riddle:
        // RIDDLE <player> <question> <answer> <CORRECT|WRONG>
        resultOutStr << gameTime << " RIDDLE " << player << " " << question << " " << answer << " " << (correct ? "CORRECT" : "WRONG"); // << "\n";
        break;

    case ResultEvent::GameEnd:
        // GAME_END <score>
        resultOutStr << gameTime << " GAME_END " << score;// << "\n";
        break;
    case ResultEvent::ItemPickup:
        // ITEM_PICKUP <player> <item>
        resultOutStr << gameTime << " ITEM_PICKUP " << player << " " << answer;// << "\n";   // reuse 'answer' as item symbol/name
        break;
    case ResultEvent::ScoreChange:
        // SCORE_CHANGE <score>
        resultOutStr << gameTime << " SCORE_CHANGE " << score;// << "\n";
        break;
    }

    std::string line = resultOutStr.str();
    // Save actual in silent mode (for comparison)
    if (mode == RunMode::LoadSilent)
    {
        actualResults.push_back(line);
        if (actualOut.is_open())
            actualOut << line << "\n";
    }

    // Write only in save mode
    if (mode == RunMode::Save && resultOut.is_open()) {
        resultOut << line << "\n";
        resultOut.flush();
    }

    resultOut.flush();
}


void Game::handlePlayersDeath(Player &player, const std::string& playerName)
{
    //if player has no life left - game over
    if (player.getLife() == 0)
    {
        if (!silent)
        {
            clearScreen();
            showGameOverBanner(playerName);
        }
        recordToResult(ResultEvent::GameEnd);
        running = false;
    }
}

void Game::showGameOverBanner(const std::string& playerName)
{
    if (silent) return;

    int boxWidth = 30;
    int boxHeight = 5;

    int startX = (BOARD_WIDTH - boxWidth) / 2;
    int startY = (BOARD_HEIGHT - boxHeight) / 2;

    // Title
    gotoxy(startX + 6, startY);
    std::cout << "GAME OVER";

    // Message
    gotoxy(startX + 3, startY + 2);
    std::cout << playerName << " has no lives!";

   
    // Pause until key
    gotoxy(startX, startY + 4);
    std::cout << "Press any key to continue...";
    char c = _getch();
}

//if an answer to riddle is right - add the riddl's score to the total score
//if an answer to riddle is wrong - reduce 20 from the riddl's score
void Game::updateScore(bool answeredCorrectly)
{
    if (answeredCorrectly)
    {
        // add remaining points to total score
        score += RiddleScore;

        // reset for next riddle
        RiddleScore = 200;
        //update result file
        recordToResult(ResultEvent::ScoreChange);
    }
    else
    {
        // subtract 20 points for wrong answer
        RiddleScore -= 20;

        if (RiddleScore < 0)
            RiddleScore = 0;
        //update result file
        recordToResult(ResultEvent::ScoreChange);
    }
}
int Game::getScore()const
{
    return score;
}

void Game::loadExpectedResultFile()
{
    std::ifstream in("adv-world.result.txt");
    if (!in) {
        std::cout << "Failed to open adv-world.result.txt\n";
        running = false;
        return;
    }

    expectedResults.clear();
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        if (line.rfind("FILES", 0) == 0) continue;
        if (line.rfind("0 INIT", 0) == 0) continue;

        expectedResults.push_back(line);
    }
}


