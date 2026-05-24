# Text Adventure World

A 2-player cooperative console adventure game written in C++.

The game is played inside an 80x25 console world made of rooms, puzzles, obstacles, riddles, switches, bombs, and springs.  
Both players must cooperate in order to solve puzzles and reach the final victory room.

---

# Main Menu

- `(1)` Start a new game
- `(8)` Instructions and keys
- `(9)` Exit

The game ends when both players reach the final room.

---

# Controls

## Player 1
| Action | Key |
|---|---|
| Move Up | `W` |
| Move Down | `X` |
| Move Left | `A` |
| Move Right | `D` |
| Stay | `S` |
| Use / Drop Item | `E` |

## Player 2
| Action | Key |
|---|---|
| Move Up | `I` |
| Move Down | `M` |
| Move Left | `J` |
| Move Right | `L` |
| Stay | `K` |
| Use / Drop Item | `O` |

## Additional Keys
| Action | Key |
|---|---|
| Pause / Return to menu | `ESC` |
| Return to main menu | `H` / `h` |

---

# Game Elements

## Players
| Character | Description |
|---|---|
| `$` | Player 1 |
| `&` | Player 2 |

## Collectible Items
| Character | Item |
|---|---|
| `@` | Bomb |
| `K` | Key |
| `!` | Torch |

## Puzzle Elements
| Character | Description |
|---|---|
| `1-9` | Doors |
| `/` | Switch ON |
| `\` | Switch OFF |
| `=` | Gate |
| `?` | Riddle |
| `#` | Spring |
| `*` | Obstacle |

---

# Gameplay Mechanics

## Continuous Movement
At the start of movement, each player chooses a direction once.

The player continues moving automatically until:
- The STAY key is pressed
- A wall or obstacle blocks movement
- Spring or collision logic changes movement

---

## Items
Each player can hold one item at a time:
- Key
- Bomb
- Torch

Using `E` or `O`:
- Bombs are planted at the player's current location
- Other items are dropped into a neighboring free cell

---

## HUD Information
At the top of the screen the game displays:
- Current held items
- Player lives
- Shared score
- Game status messages

Players start with 5 lives.

Lives are reduced when:
- Answering riddles incorrectly
- Getting hit by bombs

---

# Special Mechanics

## Springs (`#`)
Springs compress when players move toward walls through them.

When released:
- Players gain boosted speed
- Boost duration equals `(compressed length)^2`

While boosted:
- Players cannot stop or reverse direction
- Sideways movement is allowed
- Boosts can transfer between players on collision

---

## Obstacles (`*`)
Obstacles are connected groups of blocks.

They:
- Cannot be walked through
- Can be pushed if enough force is applied

Force Rules:
- Normal movement force = 1
- Spring boosts increase force
- Both players can combine force together

---

## Switches and Gates
Switches work as binary bits.

Each room may contain up to 4 switches:
- Values: `1, 2, 4, 8`

When the binary value matches a gate number:
- The matching gate opens

Otherwise:
- The gate closes again

---

## Bombs
Bomb explosions:
- Spread up to radius 3
- Destroy walls
- Stop when blocked by walls
- Preserve doors

---

## Dark Rooms and Torches
In dark rooms:
- Players can only see themselves and the torch

After collecting the torch:
- The entire room becomes visible

---

## Riddles (`?`)
When stepping on a riddle:
- The player stops moving
- A multiple-choice question appears

Rules:
- Correct answer awards points
- Wrong answers reduce score value
- Wrong answers also reduce player lives

Each riddle starts at:
- `200` points
- `-20` points per wrong answer

---

# Technical Features

## Generic Switch-Gate System
Implemented using:
- Binary switch combinations
- Dynamic gate opening logic

This allows flexible puzzle creation.

---

## Obstacle Physics
Obstacles are treated as connected components.

Movement checks:
- Group size
- Combined pushing force
- Collision validation

---

## Spring Physics
Spring boosts:
- Store compression length
- Transfer momentum between players
- Support side movement during boosts

---

# File Structure

- Room layouts are loaded from external files
- Gate definitions are loaded separately
- `files_format.txt` explains the room and output formats

---

# Technologies

- C++
- Object-Oriented Programming
- Visual Studio
- Console Rendering

#Credits

Created by: Noa Allouche
