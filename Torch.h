#pragma once
#include "Board.h"

// Torch is a collectible item represented by a single char.
// Its effect (illumination) is handled by the Game / Board logic.
namespace Torch
{
    inline bool isTorch(char c)
    {
        return c == TORCH_SYMBOL;
    }
}
