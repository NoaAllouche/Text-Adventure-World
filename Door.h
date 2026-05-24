#pragma once

class Door {

    // A door can appear in multiple rooms at different positions
    struct Position {
        int room;
        int x, y;
        bool used;
    };

    Position positions[3];
    int positionCount;
    int targetRoom;
    bool isOpen;

public:
    Door();

    // Setup
    void addPosition(int room, int x, int y);
    void setTargetRoom(int room);

    // Queries
    bool isAtPosition(int room, int x, int y) const;
    int getTargetRoom() const;
    bool getIsOpen() const;

    // Actions
    void open();
    void reset();
};