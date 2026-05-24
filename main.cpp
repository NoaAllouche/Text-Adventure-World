#include "Game.h"
#include <string>

static RunMode parseMode(int argc, char* argv[]) {
    bool load = false, save = false, silent = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-load") load = true;
        if (a == "-save") save = true;
        if (a == "-silent") silent = true;
        if (a == "-load -silent") silent = true;
    }
    if (load) return silent ? RunMode::LoadSilent : RunMode::Load;
    if (save) return RunMode::Save;
    return RunMode::Normal;
}
int main(int argc, char* argv[]) 
{
    RunMode mode = parseMode(argc, argv);
    Game game(mode);
    game.run();
    return 0;
}

