#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define MAX_BUILDINGS                    15
#define MAX_EXPLOSIONS                  200
#define MAX_PLAYERS                       2

#define BUILDING_RELATIVE_ERROR          30        // Building size random range %
#define BUILDING_MIN_RELATIVE_HEIGHT     20        // Minimum height in % of the screenHeight
#define BUILDING_MAX_RELATIVE_HEIGHT     80        // Maximum height in % of the screenHeight
#define BUILDING_MIN_GRAYSCALE_COLOR    50        // Minimum gray color for the buildings
#define BUILDING_MAX_GRAYSCALE_COLOR    130        // Maximum gray color for the buildings

#define MIN_PLAYER_POSITION               5        // Minimum x position %
#define MAX_PLAYER_POSITION              20        // Maximum x position %

#define GRAVITY                       9.81f
#define DELTA_FPS                        60

#define MAX_INPUT_CHARS                   3

#define PLAYER1COLOR CLITERAL(Color){163,105,35,255}
#define PLAYER2COLOR CLITERAL(Color){249,191,48,255}

typedef struct Player {
    Vector2 position;
    Vector2 size;

    Vector2 aimingPoint;
    int aimingAngle;
    int aimingPower;

    Vector2 previousPoint;
    int previousAngle;
    int previousPower;

    Vector2 impactPoint;

    bool isLeftTeam;                // This player belongs to the left or to the right team
    bool isPlayer;                  // If is a player or an AI
    bool isAlive;
} Player;

typedef struct Building {
    Rectangle rectangle;
    Color color;
} Building;

typedef struct Explosion {
    Vector2 position;
    int radius;
    bool active;
} Explosion;

typedef struct Ball {
    Vector2 position;
    Vector2 speed;
    int radius;
    bool active;
} Ball;

static const int screenWidth = 800;
static const int screenHeight = 450;

static bool gameOver = false;
static bool pause = false;
char power[MAX_INPUT_CHARS + 1] = "\0";
char angle[MAX_INPUT_CHARS + 1] = "\0";

static Player player[MAX_PLAYERS] = { 0 };
static Building building[MAX_BUILDINGS] = { 0 };
static Explosion explosion[MAX_EXPLOSIONS] = { 0 };
static Ball ball = { 0 };

static int playerTurn = 0;
static bool ballOnAir = false;

Image player1Image;
Image player2Image;
Image bombImage;

Texture2D player1Texture;
Texture2D player2Texture;
Texture2D bombTexture;

int letterCount1 = 0;
Rectangle textBox1 = { screenWidth/2.0f - 100, 300, 225, 50 };
bool mouseOnText1 = false;
int framesCounter1 = 0;

int letterCount2 = 0;
Rectangle textBox2 = { screenWidth/2.0f - 100, 350, 225, 50 };
bool mouseOnText2 = false;
int framesCounter2 = 0;

static void InitGame(void);         // Initialize game
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

// Additional module functions
static void InitBuildings(void);
static void InitPlayers(void);
static bool UpdatePlayer(int playerTurn);
static bool UpdateBall(int playerTurn);

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Gorilla");

    InitGame();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();

    CloseWindow();

    return 0;
}

void InitGame(void)
{
    ball.radius = 10;
    ballOnAir = false;
    ball.active = false;

    player1Image = LoadImage("res/player1Image.png");
    player2Image = LoadImage("res/player2Image.png");
    bombImage = LoadImage("res/bombImage.png");

    player1Texture = LoadTextureFromImage(player1Image);
    player2Texture = LoadTextureFromImage(player2Image);
    bombTexture = LoadTextureFromImage(bombImage);

    InitBuildings();
    InitPlayers();

    // Init explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosion[i].position = (Vector2){ 0.0f, 0.0f };
        explosion[i].radius = 30;
        explosion[i].active = false;
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver)
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (!pause)
        {
            if (CheckCollisionPointRec(GetMousePosition(), textBox1)) mouseOnText1 = true;
            else mouseOnText1 = false;

            if (CheckCollisionPointRec(GetMousePosition(), textBox2)) mouseOnText2 = true;
            else mouseOnText2 = false;
            

        if (mouseOnText1)
        {
            // Set the window's cursor to the I-Beam
            SetMouseCursor(MOUSE_CURSOR_IBEAM);

            // Get char pressed (unicode character) on the queue
            int key1 = GetCharPressed();

            // Check if more characters have been pressed on the same frame
            while (key1 > 0)
            {
                // NOTE: Only allow keys in range [48..57]
                if ((key1 >= 48) && (key1 <= 57) && (letterCount1 < MAX_INPUT_CHARS))
                {
                    power[letterCount1] = (char)key1;
                    power[letterCount1 + 1] = '\0'; // Add null terminator at the end of the string.
                    letterCount1++;
                }

                key1 = GetCharPressed(); // Check next character in the queue
            }

            if (IsKeyPressed(KEY_BACKSPACE))
            {
                letterCount1--;
                if (letterCount1 < 0)
                    letterCount1 = 0;
                power[letterCount1] = '\0';
            }
        }
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (mouseOnText1)
            framesCounter1++;
        else
            framesCounter1 = 0;

        if (mouseOnText2)
        {
            // Set the window's cursor to the I-Beam
            SetMouseCursor(MOUSE_CURSOR_IBEAM);

            // Get char pressed (unicode character) on the queue
            int key2 = GetCharPressed();

            // Check if more characters have been pressed on the same frame
            while (key2 > 0)
            {
                // NOTE: Only allow keys in range [48..57]
                if ((key2 >= 48) && (key2 <= 57) && (letterCount2 < MAX_INPUT_CHARS))
                {
                    angle[letterCount2] = (char)key2;
                    angle[letterCount2 + 1] = '\0'; // Add null terminator at the end of the string.
                    letterCount2++;
                }

                key2 = GetCharPressed(); // Check next character in the queue
            }

            if (IsKeyPressed(KEY_BACKSPACE))
            {
                letterCount2--;
                if (letterCount2 < 0)
                    letterCount2 = 0;
                angle[letterCount2] = '\0';
            }
        }
        else
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (mouseOnText2)
            framesCounter2++;
        else
            framesCounter2 = 0;

        if (!ballOnAir)
            ballOnAir = UpdatePlayer(playerTurn); // If we are aiming
        else
        {int letterCount1 = 0;
            Rectangle textBox1 = {screenWidth / 2.0f - 100, 300, 225, 50};
            bool mouseOnText1 = false;
            int framesCounter1 = 0;

            int letterCount2 = 0;
            Rectangle textBox2 = {screenWidth / 2.0f - 100, 350, 225, 50};
            bool mouseOnText2 = false;
            int framesCounter2 = 0;
            if (UpdateBall(playerTurn)) // If collision
            {
                // Game over logic
                bool leftTeamAlive = false;
                bool rightTeamAlive = false;

                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    if (player[i].isAlive)
                    {
                        if (player[i].isLeftTeam)
                            leftTeamAlive = true;
                        if (!player[i].isLeftTeam)
                            rightTeamAlive = true;
                    }
                }

                if (leftTeamAlive && rightTeamAlive)
                {
                    ballOnAir = false;
                    ball.active = false;

                    playerTurn++;

                    if (playerTurn == MAX_PLAYERS)
                        playerTurn = 0;
                }
                else
                {
                    gameOver = true;

                    // if (leftTeamAlive) left team wins
                    // if (rightTeamAlive) right team wins
                }
            }
        }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

        ClearBackground(SKYBLUE);

        if (!gameOver)
        {
            // Draw buildings
            for (int i = 0; i < MAX_BUILDINGS; i++) DrawRectangleRec(building[i].rectangle, building[i].color);

            // Draw explosions
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                if (explosion[i].active) DrawCircle(explosion[i].position.x, explosion[i].position.y, explosion[i].radius, SKYBLUE);
            }

            // Draw players
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (player[i].isAlive)
                {
                    if (player[i].isLeftTeam)
                    {
                        DrawTexture(player1Texture, player[i].position.x - player[i].size.x/2 - 5, player[i].position.y - player[i].size.y/2 - 7, WHITE);
                    }
                    else
                    {
                        DrawTexture(player2Texture, player[i].position.x - player[i].size.x/2 - 5, player[i].position.y - player[i].size.y/2 - 7, WHITE);
                    }
                }
            }

            // Draw ball
            if (ball.active)
            {
                DrawTexture(bombTexture, ball.position.x - 18, ball.position.y - 30, WHITE);
            }

            // Draw the angle and the power of the aim, and the previous ones
            if (!ballOnAir)
            {
                // Draw textboxes
                if (player[playerTurn].isLeftTeam) //first player
                {
                    DrawRectangleRec(textBox1, (Color){ 0, 0, 0, 100 });
                    if (mouseOnText1)
                        DrawRectangleLines((int)textBox1.x, (int)textBox1.y, (int)textBox1.width, (int)textBox1.height, PLAYER1COLOR);
                    else
                        DrawRectangleLines((int)textBox1.x, (int)textBox1.y, (int)textBox1.width, (int)textBox1.height, BLACK);

                    DrawText(power, (int)textBox1.x + 5, (int)textBox1.y + 8, 40, PLAYER1COLOR);

                    if (mouseOnText1)
                    {
                        if (letterCount1 < MAX_INPUT_CHARS)
                        {
                            // Draw blinking underscore char
                            if (((framesCounter1 / 20) % 2) == 0)
                                DrawText("_", (int)textBox1.x + 8 + MeasureText(power, 40), (int)textBox1.y + 12, 40, PLAYER1COLOR);
                        }
                    }

                    DrawRectangleRec(textBox2, (Color){ 0, 0, 0, 100 });
                    if (mouseOnText2)
                        DrawRectangleLines((int)textBox2.x, (int)textBox2.y, (int)textBox2.width, (int)textBox2.height, PLAYER1COLOR);
                    else
                        DrawRectangleLines((int)textBox2.x, (int)textBox2.y, (int)textBox2.width, (int)textBox2.height, BLACK);

                    DrawText(angle, (int)textBox2.x + 5, (int)textBox2.y + 8, 40, PLAYER1COLOR);

                    if (mouseOnText2)
                    {
                        if (letterCount2 < MAX_INPUT_CHARS)
                        {
                            // Draw blinking underscore char
                            if (((framesCounter2 / 20) % 2) == 0)
                                DrawText("_", (int)textBox2.x + 8 + MeasureText(angle, 40), (int)textBox2.y + 12, 40, PLAYER1COLOR);
                        }
                    }
                }
                else //second player
                {
                    DrawRectangleRec(textBox1, (Color){ 0, 0, 0, 100 });
                    if (mouseOnText1)
                        DrawRectangleLines((int)textBox1.x, (int)textBox1.y, (int)textBox1.width, (int)textBox1.height, PLAYER2COLOR);
                    else
                        DrawRectangleLines((int)textBox1.x, (int)textBox1.y, (int)textBox1.width, (int)textBox1.height, BLACK);

                    DrawText(power, (int)textBox1.x + 5, (int)textBox1.y + 8, 40, PLAYER2COLOR);

                    if (mouseOnText1)
                    {
                        if (letterCount1 < MAX_INPUT_CHARS)
                        {
                            // Draw blinking underscore char
                            if (((framesCounter1 / 20) % 2) == 0)
                                DrawText("_", (int)textBox1.x + 8 + MeasureText(power, 40), (int)textBox1.y + 12, 40, PLAYER2COLOR);
                        }
                    }

                    DrawRectangleRec(textBox2, (Color){ 0, 0, 0, 100 });
                    if (mouseOnText2)
                        DrawRectangleLines((int)textBox2.x, (int)textBox2.y, (int)textBox2.width, (int)textBox2.height, PLAYER2COLOR);
                    else
                        DrawRectangleLines((int)textBox2.x, (int)textBox2.y, (int)textBox2.width, (int)textBox2.height, BLACK);

                    DrawText(angle, (int)textBox2.x + 5, (int)textBox2.y + 8, 40, PLAYER2COLOR);

                    if (mouseOnText2)
                    {
                        if (letterCount2 < MAX_INPUT_CHARS)
                        {
                            // Draw blinking underscore char
                            if (((framesCounter2 / 20) % 2) == 0)
                                DrawText("_", (int)textBox2.x + 8 + MeasureText(angle, 40), (int)textBox2.y + 12, 40, PLAYER2COLOR);
                        }
                    }
                }
            }

            if (pause) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 - 40, 40, BLACK);
        }
        else DrawText("PRESS [SPACE] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [SPACE] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, BLACK);

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    UnloadImage(player1Image);
    UnloadImage(player2Image);
    UnloadImage(bombImage);
    UnloadTexture(player1Texture);
    UnloadTexture(player2Texture);
    UnloadTexture(bombTexture);
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}

static void InitBuildings(void)
{
    // Horizontal generation
    int currentWidth = 0;

    float relativeWidth = 100/(100 - BUILDING_RELATIVE_ERROR);
    float buildingWidthMean = (screenWidth*relativeWidth/MAX_BUILDINGS) + 1;        // We add one to make sure we will cover the whole screen.

    // Vertical generation
    int currentHeighth = 0;
    int grayLevel;

    // Creation
    for (int i = 0; i < MAX_BUILDINGS; i++)
    {
        // Horizontal
        building[i].rectangle.x = currentWidth;
        building[i].rectangle.width = GetRandomValue(buildingWidthMean*(100 - BUILDING_RELATIVE_ERROR/2)/100 + 1, buildingWidthMean*(100 + BUILDING_RELATIVE_ERROR)/100);

        currentWidth += building[i].rectangle.width;

        // Vertical
        currentHeighth = GetRandomValue(BUILDING_MIN_RELATIVE_HEIGHT, BUILDING_MAX_RELATIVE_HEIGHT);
        building[i].rectangle.y = screenHeight - (screenHeight*currentHeighth/100);
        building[i].rectangle.height = screenHeight*currentHeighth/100 + 1;

        // Color
        grayLevel = GetRandomValue(BUILDING_MIN_GRAYSCALE_COLOR, BUILDING_MAX_GRAYSCALE_COLOR);
        building[i].color = (Color){ grayLevel, grayLevel, grayLevel, 255 };
    }
}

static void InitPlayers(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        player[i].isAlive = true;

        // Decide the team of this player
        if (i % 2 == 0) player[i].isLeftTeam = true;
        else player[i].isLeftTeam = false;

        // Now there is no AI
        player[i].isPlayer = true;

        // Set size, by default by now
        player[i].size = (Vector2){ 40, 40 };

        // Set position
        if (player[i].isLeftTeam) player[i].position.x = GetRandomValue(screenWidth*MIN_PLAYER_POSITION/100, screenWidth*MAX_PLAYER_POSITION/100);
        else player[i].position.x = screenWidth - GetRandomValue(screenWidth*MIN_PLAYER_POSITION/100, screenWidth*MAX_PLAYER_POSITION/100);

        for (int j = 0; j < MAX_BUILDINGS; j++)
        {
            if (building[j].rectangle.x > player[i].position.x)
            {
                // Set the player in the center of the building
                player[i].position.x = building[j-1].rectangle.x + building[j-1].rectangle.width/2;
                // Set the player at the top of the building
                player[i].position.y = building[j-1].rectangle.y - player[i].size.y/2;
                break;
            }
        }

        // Set statistics to 0
        player[i].aimingPoint.x = screenWidth/2;
        player[i].aimingPoint.y = screenHeight/2;
        player[i].previousPower = 0;
        player[i].previousPoint = player[i].aimingPoint;
        player[i].aimingAngle = 0;
        player[i].aimingPower = 0;

        player[i].impactPoint = (Vector2){ -100, -100 };
    }
}

static bool UpdatePlayer(int playerTurn)
{
    // Left team
    if (player[playerTurn].isLeftTeam)
    {
        // Ball fired
        if (IsKeyPressed(KEY_SPACE) && atoi(power) <= 300 && atoi(angle) <= 90 && atoi(power) != 0 && atoi(angle) != 0)
        {
            player[playerTurn].aimingPower = atoi(power);
            
            player[playerTurn].aimingAngle = atoi(angle);

            player[playerTurn].previousPower = player[playerTurn].aimingPower;
            player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
            ball.position = player[playerTurn].position;

            return true;
        }
    }
    // Right team
    else if (!player[playerTurn].isLeftTeam)
    {
        // Ball fired
        if (IsKeyPressed(KEY_SPACE) && atoi(power) <= 300 && atoi(angle) <= 90 && atoi(power) != 0 && atoi(angle) != 0)
        {
            player[playerTurn].aimingPower = atoi(power);
            
            player[playerTurn].aimingAngle = atoi(angle);

            player[playerTurn].previousPower = player[playerTurn].aimingPower;
            player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
            ball.position = player[playerTurn].position;

            return true;
        }
    }
    else
    {
        player[playerTurn].aimingPoint = player[playerTurn].position;
        player[playerTurn].aimingPower = 0;
        player[playerTurn].aimingAngle = 0;
    }

    return false;
}

static bool UpdateBall(int playerTurn)
{
    static int explosionNumber = 0;

    // Activate ball
    if (!ball.active)
    {
        if (player[playerTurn].isLeftTeam)
        {
            ball.speed.x = cos(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            ball.speed.y = -sin(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            ball.active = true;

            for(int i = 0; i < MAX_INPUT_CHARS; i++)
            {
                power[i] = '\0';
                angle[i] = '\0';

                letterCount1 = 0;
                mouseOnText1 = false;
                framesCounter1 = 0;

                letterCount2 = 0;
                mouseOnText2 = false;
                framesCounter2 = 0;
            }
        }
        else
        {
            ball.speed.x = -cos(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            ball.speed.y = -sin(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            ball.active = true;

            for(int i = 0; i < MAX_INPUT_CHARS; i++)
            {
                power[i] = '\0';
                angle[i] = '\0';

                letterCount1 = 0;
                mouseOnText1 = false;
                framesCounter1 = 0;

                letterCount2 = 0;
                mouseOnText2 = false;
                framesCounter2 = 0;
            }
        }
    }

    ball.position.x += ball.speed.x;
    ball.position.y += ball.speed.y;
    ball.speed.y += GRAVITY/DELTA_FPS;

    // Collision
    if (ball.position.x + ball.radius < 0) return true;
    else if (ball.position.x - ball.radius > screenWidth) return true;
    else if (ball.position.y - ball.radius > screenHeight) return true;
    else
    {
        // Player collision
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (CheckCollisionCircleRec(ball.position, ball.radius,  (Rectangle){ player[i].position.x - player[i].size.x/2, player[i].position.y - player[i].size.y/2,
                                                                                  player[i].size.x, player[i].size.y }))
            {
                // We can't hit ourselves
                if (i == playerTurn) return false;
                else
                {
                    // We set the impact point
                    player[playerTurn].impactPoint.x = ball.position.x;
                    player[playerTurn].impactPoint.y = ball.position.y + ball.radius;

                    // We destroy the player
                    player[i].isAlive = false;
                    return true;
                }
            }
        }

        // Building collision
        // NOTE: We only check building collision if we are not inside an explosion
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
        {
            if (CheckCollisionCircles(ball.position, ball.radius, explosion[i].position, explosion[i].radius - ball.radius))
            {
                return false;
            }
        }

        for (int i = 0; i < MAX_BUILDINGS; i++)
        {
            if (CheckCollisionCircleRec(ball.position, ball.radius, building[i].rectangle))
            {
                // We set the impact point
                player[playerTurn].impactPoint.x = ball.position.x;
                player[playerTurn].impactPoint.y = ball.position.y + ball.radius;

                // We create an explosion
                explosion[explosionNumber].position = player[playerTurn].impactPoint;
                explosion[explosionNumber].active = true;
                explosionNumber++;

                return true;
            }
        }
    }

    return false;
}