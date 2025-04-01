// Standard library includes for basic I/O, boolean types and math functions
#include "raylib.h"  // The core graphics/game library
#include <stdio.h>   // For printf and text formatting
#include <stdbool.h> // Gives us true/false support
#include <math.h>    // Needed for the sin function in our rainbow effect

// Game configuration constants
#define MAX_GRID_SIZE 6   // Largest grid we'll use (for hardest levels)
#define MAX_LEVEL 10      // Total number of levels in the game

// Game state enum - tracks which screen we're currently showing
typedef enum {
    GAME_START,        // Title/welcome screen
    GAME_LEVEL_SELECT, // Level selection menu
    GAME_PLAYING,      // Active gameplay
    GAME_OVER,         // Failure screen
    GAME_WIN           // Level completion screen
} GameState;

// Card structure - contains all data for a single card
typedef struct {
    int symbolId;        // Which symbol this card shows (A-Z)
    bool flipped;        // Is the card currently face-up?
    bool solved;         // Has this card been matched already?
    float coolAnimation; // Timer for the flashy effect when cards match
} Card;

// Level configuration - different settings for each level
typedef struct {
    int gridSize;        // Grid dimensions (4x4, 5x5, etc.)
    float timeLimit;     // How many seconds to complete the level
    float peekTime;      // How long players can see cards at the start
    int maxTries;        // How many failed matches are allowed
} LevelConfig;

// Simple symbol set - using letters to keep it clean and readable
const char* SYMBOLS[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
                         "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
const int MAX_SYMBOL_COUNT = 26;  // Maximum unique symbols available

// Animation configuration
#define FLASH_TIME 1.0f         // Duration of the match animation in seconds
#define RAINBOW_SPEED 10.0f     // Speed of color cycling (higher = faster)
#define NEW_HIGH_SCORE_DURATION 3.0f // Duration to show new high score banner

// Global game state variables
int currentLevel = 1;              // Which level we're playing (starts at 1)
bool levelUnlocked[MAX_LEVEL + 1]; // Tracks which levels player can access
int gridSize = 4;                  // Current grid dimensions
int triesLeft = 0;                 // Remaining match attempts
int totalTries = 0;                // Total attempts allowed in current level
float highScores[MAX_LEVEL + 1];   // Best completion times for each level (0 = no score yet)
float currentRunTime = 0.0f;       // Time spent in current game run
bool showNewHighScore = false;     // Flag to show new high score banner
float newHighScoreTimer = 0.0f;    // Timer for new high score banner

// Level difficulty progression - each level gets harder!
// Format: {gridSize, timeLimit, peekTime, maxTries}
LevelConfig levelConfigs[MAX_LEVEL + 1] = {
    {0, 0, 0, 0},               // Empty entry (index 0 not used)
    {4, 60.0f, 3.5f, 18},       // Level 1: 4x4 grid, generous time and tries
    {4, 55.0f, 3.0f, 17},       // Level 2: Slightly faster
    {4, 50.0f, 2.5f, 16},       // Level 3: Even less forgiving
    {5, 55.0f, 2.5f, 17},       // Level 4: Bigger grid (5x5)
    {5, 50.0f, 2.0f, 16},       // Level 5: Less time and peek
    {5, 45.0f, 1.5f, 15},       // Level 6: Getting tougher
    {5, 40.0f, 1.5f, 15},       // Level 7: Even less time
    {6, 50.0f, 1.5f, 6},        // Level 8: Largest grid (6x6)
    {6, 45.0f, 1.0f, 5},        // Level 9: Quick peek, fewer tries
    {6, 40.0f, 0.8f, 4}         // Level 10: Final challenge
};

/**
 * Prepares the game board by randomizing card symbols
 * Uses the Fisher-Yates shuffle to ensure fair distribution
 */
void ShuffleCards(Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE]) {
    int totalCards = gridSize * gridSize;
    int requiredSymbols = totalCards / 2;
    
    if (requiredSymbols > MAX_SYMBOL_COUNT) {
        requiredSymbols = MAX_SYMBOL_COUNT;
    }
    
    int symbolList[MAX_GRID_SIZE * MAX_GRID_SIZE];
    for (int i = 0; i < totalCards; i++) {
        symbolList[i] = (i / 2) % requiredSymbols;
    }
    
    for (int i = totalCards - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int temp = symbolList[i];
        symbolList[i] = symbolList[j];
        symbolList[j] = temp;
    }
    
    int index = 0;
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            gameBoard[row][col].symbolId = symbolList[index++];
            gameBoard[row][col].flipped = false;
            gameBoard[row][col].solved = false;
            gameBoard[row][col].coolAnimation = 0.0f;
        }
    }
}

/**
 * Creates a rainbow color effect using sine waves
 */
Color MakeRainbowColor(float time) {
    float r = sin(time * RAINBOW_SPEED) * 0.5f + 0.5f;
    float g = sin(time * RAINBOW_SPEED + 2.0f) * 0.5f + 0.5f;
    float b = sin(time * RAINBOW_SPEED + 4.0f) * 0.5f + 0.5f;
    return (Color){ (unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), 255 };
}

/**
 * Renders all cards on the screen based on their current state
 */
void ShowCards(Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE], float time, bool peekMode) {
    int cardSize = 400 / gridSize;
    int spacing = cardSize + 5;
    int startX = (512 - (gridSize * spacing)) / 2 + 10;
    int startY = 100;
    
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            int x = col * spacing + startX;
            int y = row * spacing + startY;
            
            if (gameBoard[row][col].solved) {
                if (gameBoard[row][col].coolAnimation > 0.0f) {
                    Color partyColor = MakeRainbowColor(time);
                    DrawRectangle(x, y, cardSize, cardSize, partyColor);
                } else {
                    DrawRectangle(x, y, cardSize, cardSize, GRAY);
                }
                const char* symbol = SYMBOLS[gameBoard[row][col].symbolId];
                int fontSize = cardSize / 3;
                int textWidth = MeasureText(symbol, fontSize);
                DrawText(symbol, x + (cardSize - textWidth)/2, y + cardSize/3, fontSize, BLACK);
            } else if (gameBoard[row][col].flipped || peekMode) {
                DrawRectangle(x, y, cardSize, cardSize, WHITE);
                const char* symbol = SYMBOLS[gameBoard[row][col].symbolId];
                int fontSize = cardSize / 3;
                int textWidth = MeasureText(symbol, fontSize);
                DrawText(symbol, x + (cardSize - textWidth)/2, y + cardSize/3, fontSize, BLACK);
            } else {
                DrawRectangle(x, y, cardSize, cardSize, BLUE);
            }
        }
    }
}

/**
 * Utility function to check if a mouse click hits within a rectangle
 */
bool ClickedOnCard(Vector2 click, int x, int y, int width, int height) {
    return (click.x >= x && click.x <= x + width && click.y >= y && click.y <= y + height);
}

/**
 * Helper function for creating interactive buttons
 */
bool DrawButton(const char* text, int x, int y, int width, int height, Color color, Color textColor) {
    bool clicked = false;
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = ClickedOnCard(mousePos, x, y, width, height);
    Color btnColor = mouseOver ? ColorBrightness(color, 0.3f) : color;
    
    DrawRectangle(x, y, width, height, btnColor);
    int textWidth = MeasureText(text, 20);
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - 20) / 2;
    DrawText(text, textX, textY, 20, textColor);
    
    if (mouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        clicked = true;
    }
    return clicked;
}

/**
 * Specialized button for level selection
 */
bool DrawLevelButton(int level, int x, int y, int width, int height, bool unlocked) {
    bool clicked = false;
    char levelText[20];
    sprintf(levelText, "Level %d", level);
    
    Vector2 mousePos = GetMousePosition();
    bool mouseOver = ClickedOnCard(mousePos, x, y, width, height) && unlocked;
    Color btnColor = unlocked ? (mouseOver ? ColorBrightness(GREEN, 0.3f) : GREEN) : DARKGRAY;
    
    DrawRectangle(x, y, width, height, btnColor);
    int textWidth = MeasureText(levelText, 20);
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - 20) / 2;
    DrawText(levelText, textX, textY, 20, WHITE);
    
    if (!unlocked) {
        DrawText("🔒", x + width - 30, y + 5, 20, WHITE);
    }
    
    if (mouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && unlocked) {
        clicked = true;
    }
    return clicked;
}

/**
 * Set up everything for a new level
 */
void StartLevel(Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE], int level,
               float *peekTimer, bool *peekingAtStart, 
               int *firstCard_row, int *firstCard_col, int *secondCard_row, int *secondCard_col, 
               bool *checkingPair, float *matchTimer, float *gameTimer, GameState *gameState) {
    currentLevel = level;
    gridSize = levelConfigs[level].gridSize;
    ShuffleCards(gameBoard);
    
    *peekTimer = levelConfigs[level].peekTime;
    *peekingAtStart = true;
    *firstCard_row = -1;
    *firstCard_col = -1;
    *secondCard_row = -1;
    *secondCard_col = -1;
    *checkingPair = false;
    *matchTimer = 0.0f;
    *gameTimer = levelConfigs[level].timeLimit;
    triesLeft = levelConfigs[level].maxTries;
    totalTries = levelConfigs[level].maxTries;
    currentRunTime = 0.0f; // Reset current run time
    showNewHighScore = false; // Reset high score banner
    *gameState = GAME_PLAYING;
}

/**
 * Main game loop and program entry point
 */
int main(void) {
    InitWindow(512, 512, "Memory Matching Game");
    SetTargetFPS(60);
    
    Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE];
    for (int i = 0; i <= MAX_LEVEL; i++) {
        levelUnlocked[i] = (i == 1); // Only level 1 starts unlocked
        highScores[i] = 0.0f;        // Initialize high scores to 0 (no score yet)
    }
    
    int firstCard_row = -1, firstCard_col = -1;
    int secondCard_row = -1, secondCard_col = -1;
    float matchTimer = 0.0f, gameTime = 0.0f;
    float peekTimer = levelConfigs[1].peekTime;
    bool peekingAtStart = true, checkingPair = false;
    float gameTimer = levelConfigs[1].timeLimit;
    bool gameWon = false;
    GameState gameState = GAME_START;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        gameTime += deltaTime;

        BeginDrawing();
        ClearBackground(BLACK);
        DrawRectangle(0, 0, 512, 50, DARKGRAY);
        Vector2 logoPos = {10, 5};
        DrawText("Memory Game", 150, 15, 20, WHITE);

        if (gameState == GAME_START) {
            if (DrawButton("Start Game", 160, 200, 200, 50, GREEN, WHITE)) {
                gameState = GAME_LEVEL_SELECT;
            }
            DrawText("Click the cards to match pairs!", 100, 280, 20, WHITE);
            DrawText("Complete each level to unlock the next!", 70, 310, 20, WHITE);
            DrawText("Watch your time and tries remaining!", 80, 340, 20, WHITE);
        }
        
        else if (gameState == GAME_LEVEL_SELECT) {
            DrawText("Select a Level", 180, 70, 25, WHITE);
            int buttonWidth = 90, buttonHeight = 50, buttonsPerRow = 3;
            int startX = 512/2 - (buttonsPerRow * buttonWidth)/2, startY = 120, spacing = 10;
            
            for (int i = 1; i <= MAX_LEVEL; i++) {
                int row = (i-1) / buttonsPerRow;
                int col = (i-1) % buttonsPerRow;
                int x = startX + col * (buttonWidth + spacing);
                int y = startY + row * (buttonHeight + spacing);
                if (DrawLevelButton(i, x, y, buttonWidth, buttonHeight, levelUnlocked[i])) {
                    StartLevel(gameBoard, i, &peekTimer, &peekingAtStart, 
                               &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col, 
                               &checkingPair, &matchTimer, &gameTimer, &gameState);
                    gameWon = false;
                }
            }
            if (DrawButton("Back", 200, 400, 120, 40, RED, WHITE)) {
                gameState = GAME_START;
            }
        }
        
        else if (gameState == GAME_PLAYING) {
            char levelText[20];
            sprintf(levelText, "Level %d", currentLevel);
            DrawText(levelText, 150, 55, 20, WHITE);

            if (peekingAtStart) {
                peekTimer -= deltaTime;
                if (peekTimer <= 0.0f) {
                    peekingAtStart = false;
                }
            }

            if (!peekingAtStart && !gameWon) {
                gameTimer -= deltaTime;
                currentRunTime += deltaTime; // Track time spent playing
                if (gameTimer <= 0.0f) {
                    gameState = GAME_OVER;
                }
            }

            char timerText[20];
            snprintf(timerText, sizeof(timerText), "Time: %.1f", gameTimer);
            DrawText(timerText, 250, 60, 15, gameTimer <= 5.0f ? RED : WHITE);

            char triesText[20];
            snprintf(triesText, sizeof(triesText), "Tries: %d/%d", triesLeft, totalTries);
            DrawText(triesText, 320, 55, 20, triesLeft <= 1 ? RED : WHITE);

            // Display high score
            char highScoreText[30];
            if (highScores[currentLevel] > 0.0f) {
                snprintf(highScoreText, sizeof(highScoreText), "Best: %.1f s", highScores[currentLevel]);
            } else {
                snprintf(highScoreText, sizeof(highScoreText), "Best: None");
            }
            DrawText(highScoreText, 10, 55, 20, YELLOW);

            for (int row = 0; row < gridSize; row++) {
                for (int col = 0; col < gridSize; col++) {
                    if (gameBoard[row][col].coolAnimation > 0.0f) {
                        gameBoard[row][col].coolAnimation -= deltaTime;
                    }
                }
            }

            if (checkingPair) {
                matchTimer -= deltaTime;
                if (matchTimer <= 0.0f) {
                    checkingPair = false;
                    if (gameBoard[firstCard_row][firstCard_col].symbolId == 
                        gameBoard[secondCard_row][secondCard_col].symbolId) {
                        gameBoard[firstCard_row][firstCard_col].solved = true;
                        gameBoard[secondCard_row][secondCard_col].solved = true;
                        gameBoard[firstCard_row][firstCard_col].coolAnimation = FLASH_TIME;
                        gameBoard[secondCard_row][secondCard_col].coolAnimation = FLASH_TIME;

                        gameWon = true;
                        for (int row = 0; row < gridSize; row++) {
                            for (int col = 0; col < gridSize; col++) {
                                if (!gameBoard[row][col].solved) {
                                    gameWon = false;
                                    break;
                                }
                            }
                            if (!gameWon) break;
                        }

                        if (gameWon) {
                            if (currentLevel < MAX_LEVEL) {
                                levelUnlocked[currentLevel + 1] = true;
                            }
                            // Check for new high score (faster time = better)
                            if (highScores[currentLevel] == 0.0f || currentRunTime < highScores[currentLevel]) {
                                highScores[currentLevel] = currentRunTime;
                                showNewHighScore = true;
                                newHighScoreTimer = NEW_HIGH_SCORE_DURATION;
                            }
                            gameState = GAME_WIN;
                        }
                    } else {
                        gameBoard[firstCard_row][firstCard_col].flipped = false;
                        gameBoard[secondCard_row][secondCard_col].flipped = false;
                        triesLeft--;
                        if (triesLeft <= 0) {
                            gameState = GAME_OVER;
                        }
                    }
                    firstCard_row = -1;
                    firstCard_col = -1;
                    secondCard_row = -1;
                    secondCard_col = -1;
                }
            }

            if (!checkingPair && !peekingAtStart && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mousePos = GetMousePosition();
                int cardSize = 400 / gridSize;
                int spacing = cardSize + 5;
                int startX = (512 - (gridSize * spacing)) / 2 + 10;
                int startY = 100;

                for (int row = 0; row < gridSize; row++) {
                    for (int col = 0; col < gridSize; col++) {
                        int x = col * spacing + startX;
                        int y = row * spacing + startY;
                        if (ClickedOnCard(mousePos, x, y, cardSize, cardSize)) {
                            if (!gameBoard[row][col].flipped && !gameBoard[row][col].solved) {
                                gameBoard[row][col].flipped = true;
                                if (firstCard_row == -1) {
                                    firstCard_row = row;
                                    firstCard_col = col;
                                } else {
                                    secondCard_row = row;
                                    secondCard_col = col;
                                    checkingPair = true;
                                    matchTimer = 1.0f;
                                }
                            }
                        }
                    }
                }
            }

            ShowCards(gameBoard, gameTime, peekingAtStart);

            if (DrawButton("Level Select", 10, 480, 120, 30, ORANGE, WHITE)) {
                gameState = GAME_LEVEL_SELECT;
            }
            if (DrawButton("Reset Level", 380, 480, 120, 30, RED, WHITE)) {
                StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart, 
                          &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col, 
                          &checkingPair, &matchTimer, &gameTimer, &gameState);
                gameWon = false;
            }

            // Show new high score banner if active
            if (showNewHighScore) {
                newHighScoreTimer -= deltaTime;
                if (newHighScoreTimer > 0.0f) {
                    DrawRectangle(0, 200, 512, 60, (Color){0, 255, 0, 200});
                    DrawText("New High Score Set!", 150, 220, 25, WHITE);
                } else {
                    showNewHighScore = false;
                }
            }
        }
        
        else if (gameState == GAME_OVER) {
            ShowCards(gameBoard, gameTime, false);
            DrawRectangle(0, 0, 512, 512, (Color){0, 0, 0, 200});
            if (triesLeft <= 0) {
                DrawText("OUT OF TRIES!", 170, 200, 25, RED);
            } else {
                DrawText("TIME'S UP!", 190, 200, 25, RED);
            }
            DrawText("GAME OVER!", 180, 230, 25, RED);
            if (DrawButton("Try Again", 200, 280, 120, 40, GREEN, WHITE)) {
                StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart, 
                          &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col, 
                          &checkingPair, &matchTimer, &gameTimer, &gameState);
                gameWon = false;
            }
            if (DrawButton("Level Select", 200, 330, 120, 40, BLUE, WHITE)) {
                gameState = GAME_LEVEL_SELECT;
            }
        }
        
        else if (gameState == GAME_WIN) {
            ShowCards(gameBoard, gameTime, false);
            DrawRectangle(0, 0, 512, 512, (Color){0, 0, 0, 200});
            DrawText("LEVEL COMPLETE!", 160, 180, 25, GREEN);
            char levelText[50];
            sprintf(levelText, "You completed Level %d in %.1f s!", currentLevel, currentRunTime);
            DrawText(levelText, 130, 220, 20, WHITE);
            char highScoreText[30];
            snprintf(highScoreText, sizeof(highScoreText), "Best: %.1f s", highScores[currentLevel]);
            DrawText(highScoreText, 190, 250, 20, YELLOW);

            if (currentLevel < MAX_LEVEL) {
                if (DrawButton("Next Level", 200, 280, 120, 40, GREEN, WHITE)) {
                    StartLevel(gameBoard, currentLevel + 1, &peekTimer, &peekingAtStart, 
                             &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col, 
                             &checkingPair, &matchTimer, &gameTimer, &gameState);
                    gameWon = false;
                }
            } else {
                DrawText("Congratulations! You beat all levels!", 100, 280, 20, GOLD);
            }
            if (DrawButton("Level Select", 200, 330, 120, 40, BLUE, WHITE)) {
                gameState = GAME_LEVEL_SELECT;
            }
            if (DrawButton("Replay Level", 200, 380, 120, 40, ORANGE, WHITE)) {
                StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart, 
                          &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col, 
                          &checkingPair, &matchTimer, &gameTimer, &gameState);
                gameWon = false;
            }

            // Show new high score banner if active
            if (showNewHighScore) {
                newHighScoreTimer -= deltaTime;
                if (newHighScoreTimer > 0.0f) {
                    DrawRectangle(0, 120, 512, 60, (Color){0, 255, 0, 200});
                    DrawText("New High Score Set!", 150, 140, 25, WHITE);
                } else {
                    showNewHighScore = false;
                }
            }
        }

        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}