/*
 * Memory Matching Game
 *
 * A simple memory game using raylib.
 * Global variables are used for simplicity.
 *
 * References:
 * - GeeksforGeeks (2025): Fisher-Yates Shuffle Algorithm. https://www.geeksforgeeks.org/shuffle-a-given-array-using-fisher-yates-shuffle-algorithm/
 * - Krazydad (no date): How to make colors from numbers. https://krazydad.com/tutorials/makecolors.php
 * - Fiedler, G. (2004): Fix your timestep. https://gafferongames.com/post/fix_your_timestep/
 */

 #include "raylib.h"
 #include <stdio.h>
 #include <stdbool.h>
 #include <math.h>
 
 #define MAX_GRID_SIZE 6
 #define MAX_LEVEL 10
 
 // Game screen state control
 typedef enum {
     GAME_START,
     GAME_LEVEL_SELECT,
     GAME_PLAYING,
     GAME_OVER,
     GAME_WIN
 } GameState;
 
 // Represents a single card on the grid
 typedef struct {
     int symbolId;          // ID of the symbol (A-Z)
     bool flipped;          // Card face-up state
     bool solved;           // Matched state
     float coolAnimation;   // Timer for match animation
 } Card;
 
 // Per-level configuration
 typedef struct {
     int gridSize;
     float timeLimit;
     float peekTime;
     int maxTries;
 } LevelConfig;
 
 // List of symbols to use on cards
 const char* SYMBOLS[] = {
     "A","B","C","D","E","F","G","H","I","J","K","L","M",
     "N","O","P","Q","R","S","T","U","V","W","X","Y","Z"
 };
 const int MAX_SYMBOL_COUNT = 26;
 
 #define FLASH_TIME 1.0f
 #define RAINBOW_SPEED 10.0f
 #define NEW_HIGH_SCORE_DURATION 3.0f
 
 // Global gameplay variables
 int currentLevel = 1;
 bool levelUnlocked[MAX_LEVEL + 1];
 int gridSize = 4;
 int triesLeft = 0;
 int totalTries = 0;
 float highScores[MAX_LEVEL + 1];
 float currentRunTime = 0.0f;
 bool showNewHighScore = false;
 float newHighScoreTimer = 0.0f;
 
 // Level-specific settings
 LevelConfig levelConfigs[MAX_LEVEL + 1] = {
     {0, 0, 0, 0},
     {4, 60.0f, 3.5f, 18},
     {4, 55.0f, 3.0f, 17},
     {4, 50.0f, 2.5f, 16},
     {5, 55.0f, 2.5f, 17},
     {5, 50.0f, 2.0f, 16},
     {5, 45.0f, 1.5f, 15},
     {5, 40.0f, 1.5f, 15},
     {6, 50.0f, 1.5f, 26},
     {6, 45.0f, 1.0f, 28},
     {6, 40.0f, 0.8f, 30}
 };
 
 /*
  * ShuffleCards
  * 
  * Sets up and shuffles card symbols using Fisher-Yates shuffle
  */
 void ShuffleCards(Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE]) {
     int totalCards = gridSize * gridSize;
     int requiredSymbols = totalCards / 2;
     if (requiredSymbols > MAX_SYMBOL_COUNT) requiredSymbols = MAX_SYMBOL_COUNT;
 
     int symbolList[MAX_GRID_SIZE * MAX_GRID_SIZE];
 
     // Add each symbol twice
     for (int i = 0; i < totalCards; i++) {
         symbolList[i] = (i / 2) % requiredSymbols;
     }
 
     // Shuffle symbolList
     for (int i = totalCards - 1; i > 0; i--) {
         int j = GetRandomValue(0, i);
         int temp = symbolList[i];
         symbolList[i] = symbolList[j];
         symbolList[j] = temp;
     }
 
     // Assign shuffled symbols to game board
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
 
 /*
  * MakeRainbowColor
  *
  * Creates animated RGB color using sine wave offsets
  */
 Color MakeRainbowColor(float time) {
     float r = sin(time * RAINBOW_SPEED) * 0.5f + 0.5f;
     float g = sin(time * RAINBOW_SPEED + 2.0f) * 0.5f + 0.5f;
     float b = sin(time * RAINBOW_SPEED + 4.0f) * 0.5f + 0.5f;
     return (Color){ (unsigned char)(r * 255), (unsigned char)(g * 255),
                     (unsigned char)(b * 255), 255 };
 }
 
 /*
  * ShowCards
  *
  * Renders cards on screen based on their current state
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
 
             // Draw solved cards with animation
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
                 DrawText(symbol, x + (cardSize - textWidth) / 2, y + cardSize / 3, fontSize, BLACK);
             }
             // Show face-up cards or during peek mode
             else if (gameBoard[row][col].flipped || peekMode) {
                 DrawRectangle(x, y, cardSize, cardSize, WHITE);
                 const char* symbol = SYMBOLS[gameBoard[row][col].symbolId];
                 int fontSize = cardSize / 3;
                 int textWidth = MeasureText(symbol, fontSize);
                 DrawText(symbol, x + (cardSize - textWidth) / 2, y + cardSize / 3, fontSize, BLACK);
             }
             // Face-down card
             else {
                 DrawRectangle(x, y, cardSize, cardSize, BLUE);
             }
         }
     }
 }
 
 /*
  * ClickedOnCard
  *
  * Returns true if mouse click is within rectangle bounds
  */
 bool ClickedOnCard(Vector2 click, int x, int y, int width, int height) {
     return (click.x >= x && click.x <= x + width &&
             click.y >= y && click.y <= y + height);
 }
 
 /*
  * DrawButton
  *
  * Draws a button and returns true if clicked
  */
 bool DrawButton(const char* text, int x, int y, int width, int height, Color color, Color textColor) {
     bool clicked = false;
     Vector2 mousePos = GetMousePosition();
     bool mouseOver = ClickedOnCard(mousePos, x, y, width, height);
     Color btnColor = mouseOver ? ColorBrightness(color, 0.3f) : color;
 
     DrawRectangleRounded((Rectangle){ x, y, width, height }, 0.2f, 10, btnColor);
     int textWidth = MeasureText(text, 20);
     int textX = x + (width - textWidth) / 2;
     int textY = y + (height - 20) / 2;
     DrawText(text, textX, textY, 20, textColor);
 
     if (mouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
         clicked = true;
 
     return clicked;
 }
 
 /*
  * DrawLevelButton
  *
  * Displays level button, shows lock if level is locked
  */
 bool DrawLevelButton(int level, int x, int y, int width, int height, bool unlocked) {
     bool clicked = false;
     char levelText[20];
     sprintf(levelText, "Level %d", level);
 
     Vector2 mousePos = GetMousePosition();
     bool mouseOver = ClickedOnCard(mousePos, x, y, width, height) && unlocked;
     Color btnColor = unlocked ? (mouseOver ? ColorBrightness(GREEN, 0.3f) : GREEN) : DARKGRAY;
 
     DrawRectangleRounded((Rectangle){ x, y, width, height }, 0.2f, 10, btnColor);
 
     int textWidth = MeasureText(levelText, 20);
     int textX = x + (width - textWidth) / 2;
     int textY = y + (height - 20) / 2;
     DrawText(levelText, textX, textY, 20, WHITE);
 
     if (!unlocked)
         DrawText("🔒", x + width - 30, y + 5, 20, WHITE);
 
     if (mouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && unlocked)
         clicked = true;
 
     return clicked;
 }
 
 /*
  * StartLevel
  *
  * Sets all values to start a new level and resets game state
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
 
     *firstCard_row = *firstCard_col = -1;
     *secondCard_row = *secondCard_col = -1;
     *checkingPair = false;
     *matchTimer = 0.0f;
     *gameTimer = levelConfigs[level].timeLimit;
     triesLeft = totalTries = levelConfigs[level].maxTries;
     currentRunTime = 0.0f;
     showNewHighScore = false;
     *gameState = GAME_PLAYING;
 }

    int main(void) {
        InitWindow(512, 512, "Memory Matching Game");
        SetTargetFPS(60);

        Card gameBoard[MAX_GRID_SIZE][MAX_GRID_SIZE];

        // Unlock level 1 at start
        for (int i = 0; i <= MAX_LEVEL; i++) {
            levelUnlocked[i] = (i == 1);
            highScores[i] = 0.0f;
        }

        // Game control vars
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
            // Background gradient
            Rectangle bgRec = { 0, 0, 512, 512 };
            DrawRectangleGradientEx(bgRec, DARKBLUE, BLUE, BLACK, DARKGRAY);

            // Header
            DrawRectangle(0, 0, 512, 50, DARKGRAY);
            DrawText("Memory Game", 150, 15, 20, WHITE);

            // Main menu screen
            if (gameState == GAME_START) {
                if (DrawButton("Start Game", 160, 200, 200, 50, GREEN, WHITE))
                    gameState = GAME_LEVEL_SELECT;

                DrawText("Click cards to match pairs!", 100, 280, 20, WHITE);
                DrawText("Complete levels to unlock new ones!", 70, 310, 20, WHITE);
                DrawText("Watch your time and tries!", 80, 340, 20, WHITE);
            }

            // Level select screen
            else if (gameState == GAME_LEVEL_SELECT) {
                DrawText("Select a Level", 180, 70, 25, WHITE);
                int buttonWidth = 90, buttonHeight = 50, buttonsPerRow = 3;
                int startX = 512 / 2 - (buttonsPerRow * buttonWidth) / 2, startY = 120, spacing = 10;

                // Loop through and draw each level button
                for (int i = 1; i <= MAX_LEVEL; i++) {
                    int row = (i - 1) / buttonsPerRow;
                    int col = (i - 1) % buttonsPerRow;
                    int x = startX + col * (buttonWidth + spacing);
                    int y = startY + row * (buttonHeight + spacing);
                    if (DrawLevelButton(i, x, y, buttonWidth, buttonHeight, levelUnlocked[i])) {
                        StartLevel(gameBoard, i, &peekTimer, &peekingAtStart,
                                &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col,
                                &checkingPair, &matchTimer, &gameTimer, &gameState);
                        gameWon = false;
                    }
                }
                if (DrawButton("Back", 200, 400, 120, 40, RED, WHITE))
                    gameState = GAME_START;
            }

            // Active gameplay screen
            else if (gameState == GAME_PLAYING) {
                char levelText[20];
                sprintf(levelText, "Level %d", currentLevel);
                DrawText(levelText, 150, 55, 20, WHITE);

                if (peekingAtStart) {
                    peekTimer -= deltaTime;
                    if (peekTimer <= 0.0f) peekingAtStart = false;
                }

                if (!peekingAtStart && !gameWon) {
                    gameTimer -= deltaTime;
                    currentRunTime += deltaTime;
                    if (gameTimer <= 0.0f)
                        gameState = GAME_OVER;
                }

                char timerText[20];
                snprintf(timerText, sizeof(timerText), "Time: %.1f", gameTimer);
                DrawText(timerText, 250, 60, 15, gameTimer <= 5.0f ? RED : WHITE);

                char triesText[20];
                snprintf(triesText, sizeof(triesText), "Tries: %d/%d", triesLeft, totalTries);
                DrawText(triesText, 320, 55, 20, triesLeft <= 1 ? RED : WHITE);

                char highScoreText[30];
                if (highScores[currentLevel] > 0.0f)
                    snprintf(highScoreText, sizeof(highScoreText), "Best: %.1f s", highScores[currentLevel]);
                else
                    snprintf(highScoreText, sizeof(highScoreText), "Best: None");
                DrawText(highScoreText, 10, 55, 20, YELLOW);

                // Update solved card animations
                for (int row = 0; row < gridSize; row++) {
                    for (int col = 0; col < gridSize; col++) {
                        if (gameBoard[row][col].coolAnimation > 0.0f)
                            gameBoard[row][col].coolAnimation -= deltaTime;
                    }
                }

                // Check match after delay
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
                            // Check win condition
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
                                if (currentLevel < MAX_LEVEL)
                                    levelUnlocked[currentLevel + 1] = true;
                                if (highScores[currentLevel] == 0.0f || currentRunTime < highScores[currentLevel]) {
                                    highScores[currentLevel] = currentRunTime;
                                    showNewHighScore = true;
                                    newHighScoreTimer = NEW_HIGH_SCORE_DURATION;
                                }
                                gameState = GAME_WIN;
                            }
                        } else {
                            // Mismatch, flip back
                            gameBoard[firstCard_row][firstCard_col].flipped = false;
                            gameBoard[secondCard_row][secondCard_col].flipped = false;
                            triesLeft--;
                            if (triesLeft <= 0)
                                gameState = GAME_OVER;
                        }
                        // Reset card selections
                        firstCard_row = -1;
                        firstCard_col = -1;
                        secondCard_row = -1;
                        secondCard_col = -1;
                    }
                }

                // Flip logic
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

                if (DrawButton("Level Select", 10, 480, 120, 30, ORANGE, WHITE))
                    gameState = GAME_LEVEL_SELECT;
                if (DrawButton("Reset Level", 380, 480, 120, 30, RED, WHITE)) {
                    StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart,
                            &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col,
                            &checkingPair, &matchTimer, &gameTimer, &gameState);
                    gameWon = false;
                }

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

            // Game over screen
            else if (gameState == GAME_OVER) {
                ShowCards(gameBoard, gameTime, false);
                DrawRectangle(0, 0, 512, 512, (Color){0, 0, 0, 200});
                if (triesLeft <= 0)
                    DrawText("OUT OF TRIES!", 170, 200, 25, RED);
                else
                    DrawText("TIME'S UP!", 190, 200, 25, RED);
                DrawText("GAME OVER!", 180, 230, 25, RED);

                if (DrawButton("Try Again", 200, 280, 120, 40, GREEN, WHITE)) {
                    StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart,
                            &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col,
                            &checkingPair, &matchTimer, &gameTimer, &gameState);
                    gameWon = false;
                }
                if (DrawButton("Level Select", 200, 330, 120, 40, BLUE, WHITE))
                    gameState = GAME_LEVEL_SELECT;
            }

            // Level complete screen
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

                if (DrawButton("Level Select", 200, 330, 120, 40, BLUE, WHITE))
                    gameState = GAME_LEVEL_SELECT;
                if (DrawButton("Replay Level", 200, 380, 120, 40, ORANGE, WHITE)) {
                    StartLevel(gameBoard, currentLevel, &peekTimer, &peekingAtStart,
                            &firstCard_row, &firstCard_col, &secondCard_row, &secondCard_col,
                            &checkingPair, &matchTimer, &gameTimer, &gameState);
                    gameWon = false;
                }

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