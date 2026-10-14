#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TILE_SIZE 32
#define WORLD_W 120
#define WORLD_H 32
#define PLAYER_WIDTH 18
#define PLAYER_HEIGHT 28

enum BlockType {
    BLOCK_EMPTY = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_SAND,
    BLOCK_WATER,
    BLOCK_LOG,
    BLOCK_LEAVES,
    BLOCK_PLANK,
    BLOCK_WALL,
    BLOCK_ROOF,
    BLOCK_COUNT
};

enum InventoryType {
    INV_DIRT = 0,
    INV_STONE,
    INV_WOOD,
    INV_PLANK,
    INV_WALL,
    INV_COUNT
};

typedef struct {
    int tiles[WORLD_H][WORLD_W];
} World;

typedef struct {
    float x;
    float y;
} Player;

typedef struct {
    Player player;
    float hunger;
    float thirst;
    float energy;
    float money;
    int inventory[INV_COUNT];
    int selectedSlot;
    int day;
    float dayTime;
    int houseLevel;
    bool gameOver;
} Game;

static bool isSolidTile(int tile) {
    return tile == BLOCK_GRASS || tile == BLOCK_DIRT || tile == BLOCK_STONE || tile == BLOCK_SAND ||
           tile == BLOCK_LOG || tile == BLOCK_LEAVES || tile == BLOCK_PLANK || tile == BLOCK_WALL ||
           tile == BLOCK_ROOF;
}

static int tileToInventory(int tile) {
    switch (tile) {
        case BLOCK_DIRT: return INV_DIRT;
        case BLOCK_STONE: return INV_STONE;
        case BLOCK_LOG: return INV_WOOD;
        case BLOCK_PLANK: return INV_PLANK;
        case BLOCK_WALL: return INV_WALL;
        default: return -1;
    }
}

static int selectedBlockType(int slot) {
    switch (slot) {
        case 0: return BLOCK_DIRT;
        case 1: return BLOCK_STONE;
        case 2: return BLOCK_LOG;
        case 3: return BLOCK_PLANK;
        case 4: return BLOCK_WALL;
        default: return BLOCK_DIRT;
    }
}

static Color blockColor(int tile) {
    switch (tile) {
        case BLOCK_GRASS: return (Color){92, 154, 82, 255};
        case BLOCK_DIRT: return (Color){123, 90, 58, 255};
        case BLOCK_STONE: return (Color){122, 122, 130, 255};
        case BLOCK_SAND: return (Color){202, 180, 100, 255};
        case BLOCK_WATER: return (Color){62, 128, 198, 200};
        case BLOCK_LOG: return (Color){118, 82, 45, 255};
        case BLOCK_LEAVES: return (Color){62, 140, 64, 255};
        case BLOCK_PLANK: return (Color){160, 117, 70, 255};
        case BLOCK_WALL: return (Color){132, 111, 94, 255};
        case BLOCK_ROOF: return (Color){161, 115, 70, 255};
        default: return (Color){0, 0, 0, 0};
    }
}

static bool isInsideMap(int x, int y) {
    return x >= 0 && x < WORLD_W && y >= 0 && y < WORLD_H;
}

static bool isBlockedAt(World *world, float px, float py, float w, float h) {
    int left = (int)(px / TILE_SIZE);
    int right = (int)((px + w - 1.0f) / TILE_SIZE);
    int top = (int)(py / TILE_SIZE);
    int bottom = (int)((py + h - 1.0f) / TILE_SIZE);

    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            if (!isInsideMap(x, y)) {
                return true;
            }
            int tile = world->tiles[y][x];
            if (isSolidTile(tile)) {
                return true;
            }
        }
    }
    return false;
}

static void movePlayer(Game *game, World *world, float moveX, float moveY, float dt) {
    float newX = game->player.x + moveX * dt;
    if (!isBlockedAt(world, newX, game->player.y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
        game->player.x = newX;
    }

    float newY = game->player.y + moveY * dt;
    if (!isBlockedAt(world, game->player.x, newY, PLAYER_WIDTH, PLAYER_HEIGHT)) {
        game->player.y = newY;
    }
}

static int randomRange(int minValue, int maxValue) {
    return minValue + (rand() % (maxValue - minValue + 1));
}

static void generateWorld(World *world) {
    memset(world->tiles, BLOCK_EMPTY, sizeof(world->tiles));

    for (int x = 0; x < WORLD_W; x++) {
        int terrainHeight = 16 + (int)(sinf(x * 0.4f) * 3.0f + cosf(x * 0.2f) * 2.0f);

        for (int y = 0; y < WORLD_H; y++) {
            if (y < terrainHeight - 1) {
                world->tiles[y][x] = BLOCK_STONE;
            }
            if (y < terrainHeight) {
                world->tiles[y][x] = BLOCK_DIRT;
            }
            if (y == terrainHeight - 1) {
                world->tiles[y][x] = BLOCK_GRASS;
            }
        }

        if (x > 60 && x < 95) {
            for (int y = terrainHeight; y < terrainHeight + 5; y++) {
                if (y >= 0 && y < WORLD_H) {
                    world->tiles[y][x] = BLOCK_WATER;
                }
            }
        }
    }

    for (int x = 8; x < WORLD_W - 10; x += randomRange(6, 11)) {
        if (randomRange(0, 100) > 35) {
            int trunkHeight = randomRange(3, 5);
            int baseY = 18 + (int)(sinf(x * 0.7f) * 2.0f);
            for (int y = baseY - trunkHeight; y < baseY; y++) {
                if (y >= 0 && y < WORLD_H) {
                    world->tiles[y][x] = BLOCK_LOG;
                }
            }

            int leafCenterY = baseY - trunkHeight - 1;
            for (int y = leafCenterY - 1; y <= leafCenterY + 1; y++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int leafX = x + dx;
                    int leafY = y;
                    if (leafX >= 0 && leafX < WORLD_W && leafY >= 0 && leafY < WORLD_H && world->tiles[leafY][leafX] == BLOCK_EMPTY) {
                        world->tiles[leafY][leafX] = BLOCK_LEAVES;
                    }
                }
            }
        }
    }

    for (int x = 12; x < 28; x++) {
        for (int y = 0; y < 5; y++) {
            if (y == 0) {
                world->tiles[20 + y][x] = BLOCK_SAND;
            }
        }
    }
}

static void resetGame(Game *game, World *world) {
    memset(game, 0, sizeof(*game));
    game->player.x = 8.0f * TILE_SIZE;
    game->player.y = 12.0f * TILE_SIZE;
    game->hunger = 100.0f;
    game->thirst = 100.0f;
    game->energy = 100.0f;
    game->money = 4.0f;
    game->selectedSlot = 2;
    game->day = 1;
    game->dayTime = 0.0f;
    game->houseLevel = 0;
    game->inventory[INV_DIRT] = 8;
    game->inventory[INV_STONE] = 2;
    game->inventory[INV_WOOD] = 4;
    game->inventory[INV_PLANK] = 0;
    game->inventory[INV_WALL] = 0;

    generateWorld(world);
}

static void buildHouse(Game *game, World *world) {
    int centerX = (int)(game->player.x / TILE_SIZE);
    int centerY = (int)(game->player.y / TILE_SIZE);

    if (game->inventory[INV_WOOD] < 8 || game->inventory[INV_PLANK] < 6 || game->inventory[INV_WALL] < 4) {
        return;
    }

    int startX = centerX - 2;
    int startY = centerY - 2;
    int buildCount = 0;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 6; x++) {
            int wx = startX + x;
            int wy = startY + y;
            if (!isInsideMap(wx, wy)) {
                continue;
            }

            bool isFrame = (x == 0 || x == 5 || y == 0 || y == 4);
            if (isFrame && world->tiles[wy][wx] == BLOCK_EMPTY) {
                world->tiles[wy][wx] = BLOCK_WALL;
                buildCount++;
            }
            if (x == 2 && y == 1 && world->tiles[wy][wx] == BLOCK_EMPTY) {
                world->tiles[wy][wx] = BLOCK_ROOF;
                buildCount++;
            }
            if (x == 3 && y == 1 && world->tiles[wy][wx] == BLOCK_EMPTY) {
                world->tiles[wy][wx] = BLOCK_ROOF;
                buildCount++;
            }
        }
    }

    if (buildCount > 0) {
        game->inventory[INV_WOOD] -= 8;
        game->inventory[INV_PLANK] -= 6;
        game->inventory[INV_WALL] -= 4;
        game->houseLevel += 1;
        game->money += 2.5f;
    }
}

static void doWork(Game *game) {
    if (game->energy < 20.0f) {
        return;
    }

    int sawmillX = 8;
    int sawmillY = 17;
    int px = (int)(game->player.x / TILE_SIZE);
    int py = (int)(game->player.y / TILE_SIZE);

    if (abs(px - sawmillX) <= 4 && abs(py - sawmillY) <= 4) {
        game->money += 3.5f;
        game->inventory[INV_WOOD] += 2;
        game->hunger -= 8.0f;
        game->thirst -= 10.0f;
        game->energy -= 16.0f;
    }
}

static void eatFood(Game *game) {
    if (game->money >= 2.0f) {
        game->money -= 2.0f;
        game->hunger = (game->hunger + 35.0f > 100.0f) ? 100.0f : game->hunger + 35.0f;
        game->thirst -= 4.0f;
    }
}

static void rest(Game *game) {
    if (game->houseLevel > 0) {
        game->energy = 100.0f;
        game->hunger -= 12.0f;
        game->thirst -= 12.0f;
    }
}

static void drawWorld(World *world, Camera2D *camera, Game *game) {
    BeginMode2D(*camera);

    for (int y = 0; y < WORLD_H; y++) {
        for (int x = 0; x < WORLD_W; x++) {
            int tile = world->tiles[y][x];
            if (tile == BLOCK_EMPTY) {
                continue;
            }

            Rectangle rec = { (float)(x * TILE_SIZE), (float)(y * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };
            DrawRectangleRec(rec, blockColor(tile));

            if (tile == BLOCK_WATER) {
                DrawRectangleLinesEx(rec, 1, (Color){255, 255, 255, 80});
            }
        }
    }

    int targetTileX = (int)(GetScreenToWorld2D(GetMousePosition(), *camera).x / TILE_SIZE);
    int targetTileY = (int)(GetScreenToWorld2D(GetMousePosition(), *camera).y / TILE_SIZE);
    if (targetTileX >= 0 && targetTileX < WORLD_W && targetTileY >= 0 && targetTileY < WORLD_H) {
        Rectangle selectRect = { (float)(targetTileX * TILE_SIZE), (float)(targetTileY * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };
        DrawRectangleLinesEx(selectRect, 2, YELLOW);
    }

    Vector2 p = { game->player.x, game->player.y };
    DrawRectangle(p.x, p.y, PLAYER_WIDTH, PLAYER_HEIGHT, (Color){58, 99, 198, 255});
    DrawRectangle(p.x + 4, p.y - 8, PLAYER_WIDTH - 8, 10, (Color){224, 198, 146, 255});
    DrawRectangle(p.x + 5, p.y + PLAYER_HEIGHT, 5, 12, (Color){50, 50, 55, 255});
    DrawRectangle(p.x + PLAYER_WIDTH - 10, p.y + PLAYER_HEIGHT, 5, 12, (Color){50, 50, 55, 255});

    EndMode2D();
}

static void updateSelectedBlock(Game *game) {
    if (IsKeyPressed(KEY_ONE)) game->selectedSlot = 0;
    if (IsKeyPressed(KEY_TWO)) game->selectedSlot = 1;
    if (IsKeyPressed(KEY_THREE)) game->selectedSlot = 2;
    if (IsKeyPressed(KEY_FOUR)) game->selectedSlot = 3;
    if (IsKeyPressed(KEY_FIVE)) game->selectedSlot = 4;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fronteira Arida - Minecraft-like Survival");
    SetTargetFPS(60);

    srand((unsigned int)time(NULL));

    World world;
    Game game;
    resetGame(&game, &world);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (!game.gameOver) {
            float moveX = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
            float moveY = (IsKeyDown(KEY_S) ? 1.0f : 0.0f) - (IsKeyDown(KEY_W) ? 1.0f : 0.0f);

            if (moveX != 0.0f || moveY != 0.0f) {
                float length = sqrtf(moveX * moveX + moveY * moveY);
                if (length > 0.0f) {
                    moveX /= length;
                    moveY /= length;
                }
                movePlayer(&game, &world, moveX, moveY, 180.0f * dt);
            }

            updateSelectedBlock(&game);

            if (IsKeyPressed(KEY_E)) {
                doWork(&game);
            }

            if (IsKeyPressed(KEY_F)) {
                eatFood(&game);
            }

            if (IsKeyPressed(KEY_R)) {
                rest(&game);
            }

            if (IsKeyPressed(KEY_B)) {
                buildHouse(&game, &world);
            }

            game.dayTime += dt;
            if (game.dayTime >= 45.0f) {
                game.dayTime = 0.0f;
                game.day += 1;
            }

            game.hunger -= dt * 0.7f;
            game.thirst -= dt * 1.2f;
            game.energy -= dt * ((game.thirst < 30.0f) ? 1.2f : 0.45f);

            if (game.hunger < 0.0f) game.hunger = 0.0f;
            if (game.thirst < 0.0f) game.thirst = 0.0f;
            if (game.energy < 0.0f) game.energy = 0.0f;

            if (game.hunger <= 0.0f || game.thirst <= 0.0f || game.energy <= 0.0f) {
                game.gameOver = true;
            }
        }

        camera.target.x = game.player.x;
        camera.target.y = game.player.y;

        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        int tileX = (int)(mouseWorld.x / TILE_SIZE);
        int tileY = (int)(mouseWorld.y / TILE_SIZE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (tileX >= 0 && tileX < WORLD_W && tileY >= 0 && tileY < WORLD_H) {
                int tile = world.tiles[tileY][tileX];
                if (tile != BLOCK_EMPTY && abs(tileX - (int)(game.player.x / TILE_SIZE)) <= 5 && abs(tileY - (int)(game.player.y / TILE_SIZE)) <= 5) {
                    int invIndex = tileToInventory(tile);
                    if (invIndex >= 0) {
                        world.tiles[tileY][tileX] = BLOCK_EMPTY;
                        game.inventory[invIndex] += 1;
                    }
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            int selected = selectedBlockType(game.selectedSlot);
            if (tileX >= 0 && tileX < WORLD_W && tileY >= 0 && tileY < WORLD_H) {
                if (world.tiles[tileY][tileX] == BLOCK_EMPTY && abs(tileX - (int)(game.player.x / TILE_SIZE)) <= 5 && abs(tileY - (int)(game.player.y / TILE_SIZE)) <= 5) {
                    int invIndex = -1;
                    if (selected == BLOCK_DIRT) invIndex = INV_DIRT;
                    else if (selected == BLOCK_STONE) invIndex = INV_STONE;
                    else if (selected == BLOCK_LOG) invIndex = INV_WOOD;
                    else if (selected == BLOCK_PLANK) invIndex = INV_PLANK;
                    else if (selected == BLOCK_WALL) invIndex = INV_WALL;

                    if (invIndex >= 0 && game.inventory[invIndex] > 0) {
                        world.tiles[tileY][tileX] = selected;
                        game.inventory[invIndex]--;
                    }
                }
            }
        }

        if (game.gameOver && IsKeyPressed(KEY_ENTER)) {
            resetGame(&game, &world);
            game.gameOver = false;
        }

        BeginDrawing();
        ClearBackground((Color){126, 180, 209, 255});

        drawWorld(&world, &camera, &game);

        DrawRectangle(14, 14, 360, 160, (Color){18, 22, 27, 200});
        DrawText(TextFormat("DIA %d", game.day), 26, 26, 24, WHITE);
        DrawText(TextFormat("FOME: %.0f%%", game.hunger), 26, 58, 20, (Color){255, 99, 71, 255});
        DrawText(TextFormat("SEDE: %.0f%%", game.thirst), 26, 86, 20, (Color){77, 150, 255, 255});
        DrawText(TextFormat("ENERGIA: %.0f%%", game.energy), 26, 114, 20, (Color){102, 204, 89, 255});
        DrawText(TextFormat("DINHEIRO: $%.2f", game.money), 26, 142, 20, (Color){255, 214, 83, 255});

        DrawRectangle(14, SCREEN_HEIGHT - 120, SCREEN_WIDTH - 28, 96, (Color){30, 30, 35, 220});
        for (int i = 0; i < 5; i++) {
            int slotX = 32 + i * 110;
            int slotY = SCREEN_HEIGHT - 96;
            int blockType = selectedBlockType(i);
            Color slotColor = blockColor(blockType);
            DrawRectangle(slotX, slotY, 80, 64, (Color){60, 60, 65, 255});
            DrawRectangle(slotX + 4, slotY + 4, 72, 56, slotColor);
            DrawText(TextFormat("%d", i + 1), slotX + 8, slotY + 6, 16, WHITE);

            if (i == game.selectedSlot) {
                DrawRectangleLines(slotX - 2, slotY - 2, 84, 68, GOLD);
            }

            DrawText(TextFormat("%d", game.inventory[(i == 0) ? INV_DIRT : (i == 1) ? INV_STONE : (i == 2) ? INV_WOOD : (i == 3) ? INV_PLANK : INV_WALL]), slotX + 8, slotY + 38, 16, WHITE);
        }

        DrawText("WASD = MOVER | E = TRABALHAR | F = COMPRAR COMIDA | R = DESCANSAR | B = CONSTRUIR | MOUSE = MINERAR / COLOCAR", 18, SCREEN_HEIGHT - 36, 14, WHITE);

        if (game.gameOver) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 170});
            DrawText("VOCÊ FALHOU NA FRONTEIRA...", SCREEN_WIDTH / 2 - 260, SCREEN_HEIGHT / 2 - 40, 28, RED);
            DrawText("Pressione ENTER para recomeçar", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 + 8, 20, WHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
