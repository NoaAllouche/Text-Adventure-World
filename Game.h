#pragma once
#include "Board.h"
#include "Player.h"
#include "Bomb.h"
#include "Door.h"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

enum class RunMode { Normal, Save, Load, LoadSilent };

class Game {

    enum {

        KEY_ESC = 27,
        NUM_ROOMS = 4, //changed 3->4
        MAX_DOORS = 9,

        START_GAME_CHOICE = '1',
        INSTRUCTIONS_CHOICE = '8',
        EXIT_CHOICE = '9'
    };
   

    // Message system
    std::string message;
    int messageTimer=0;

    // starting positions for players in each room
    struct StartPos {
        int x;
        int y;
    };

    // arrays of starting positions for each player in each room
    StartPos p1Starts[NUM_ROOMS];
    StartPos p2Starts[NUM_ROOMS];

    // initialize starting positions for all rooms
    void initStartPositions();

    // array of boards, one for each room
    Board boards[NUM_ROOMS];
    int currentRoom=0;

    // two players
    Player p1;
    Player p2;

    // doors - each door can appear in multiple rooms
    Door doors[MAX_DOORS];
    int doorCount=0;

    // track if players are waiting at door to transition
    bool p1WaitingNextRoom=false;
    bool p2WaitingNextRoom=false;

    // the door used by the second player to enter (determines destination)
    Door* theOtherPlayerDoor=nullptr;

    // track previous positions for switch toggle logic
    int p1PrevX=0, p1PrevY=0;
    int p2PrevX=0, p2PrevY=0;

    // bombs - one per player, allows simultaneous deployment
    Bomb bombs[2];

    // game running state
    bool running=false;
    bool inLastRoom=false;

    // room setup
    void buildRoomsLayouts();
    void setupDoors();

    // game logic
    bool isPlayerAtDoor(Player& player);
    void HandleBothPlayersAtNextRoom();

    void printScreen();
    void printRoomInfo(bool illuminate);
    bool areBothPlayersAtDoor();
    
    void HandlePlayerAtDoor(Player& player, bool isPlayer1=true);
    Door* getDoorAtPosition(int room, int x, int y);
    //void handleRoomTransitions();
    void updateBombs();
    bool isBombPlantedHere(int x, int y) const;
    
    void playerUseItem(Player& player, int playerIndex);

    // input/output
    void processInput(char ch);
    void drawPlayers();
    void PrintPlayerInfoInHeader();
    void PrintMessageInHeader();
    void clearMessage();
    bool handlePause();
    void showInstructions();

    char p1Under = ' ';
    char p2Under = ' ';
    char p1PrevUnder = ' ';
    char p2PrevUnder = ' ';

    struct McqRiddle
    {
        std::string question;
        std::string options[4];
        int correctIndex=0;
        bool solved = false;
    };

    //handle riddles
    static const int MAX_RIDDLES = 10;
    McqRiddle riddles[MAX_RIDDLES];
    int riddleCount = 0;
    int nextRiddleIndex = 0;

    void loadRiddlesFromFile(const std::string& filename);
    bool askMcqRiddle(const McqRiddle& r, const std::string& who);
    bool askRiddle(const std::string& who);
    bool handleRiddleIfNeeded(Player& player);

    //to handle text under game frame
    bool lastWasDark = false;
    bool lastIlluminated = false;
    bool roomInfoDirty = true;   // force first draw

    //game time counter
    long long gameTime = 0;
    //game mode
    RunMode mode = RunMode::Normal;
    bool silent = false;

    std::ofstream stepsOut;
    std::ofstream resultOut;

    void openFilesForMode();
    void loadStepsFile();
    struct StepEvent {
        long long time;
        int player;
        char action;
        char value;
    };
    char getRiddleAnswerFromSteps(const std::string& who);


    enum class ResultEvent { ScreenChange, LifeLost, Riddle, GameEnd, ItemPickup,ScoreChange };

    void recordToResult(ResultEvent ev,const std::string& player = "",int screenIndex = -1,const std::string& question = "",const std::string& answer = "",bool correct = false);

    std::vector<StepEvent> steps;
    size_t nextStepIndex = 0;
    void useStepsFromStepsFile();

    //handle player death
    void handlePlayersDeath(Player& player, const std::string& playerName);
    void showGameOverBanner(const std::string& playerName);
    void updateScore(bool answeredCorrectly);
    int getScore()const;

    int score = 0;
    int RiddleScore = 200;

    //true when the players are in victory room or when one lost all life 
    bool gameIsOver = false;

    std::vector<std::string> actualResults;
    std::vector<std::string> expectedResults;
    void loadExpectedResultFile();
    long long lastStepTime = 0;
    std::ofstream actualOut;   // for LoadSilent actual results
    void tryPickup(Player& pl, char& underTile);
public:
    //Game();
    explicit Game(RunMode m);
    void init();
    void run();
    void PrintWelcomeText();
    void gameLoop();
    void UpdateHeaderOnItemPickUp(char ItemBefore, Player& player);
    bool boolHandleInput();
    void showMessage(const std::string& msg);
    
};

