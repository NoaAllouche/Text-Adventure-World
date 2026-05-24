#pragma once

#include <windows.h>
#include <iostream>

// The two functions here are copied from Wissam's exercises

// Move cursor to position (x,y) in console
inline void gotoxy(int x, int y)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos;
    pos.X = static_cast<SHORT>(x);
    pos.Y = static_cast<SHORT>(y);
    std::cout.flush();
    SetConsoleCursorPosition(hConsole, pos);
}

// Clear the console screen
inline void clearScreen()
{
    system("cls");
}

// This function was taken from chatGPT in order to hide the blinking cursor in the console (it caused alot of flickering)

// Hide the blinking cursor
inline void hideCursor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(hConsole, &info);
    info.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &info);
}