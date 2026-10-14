#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define WORLD_X 1024
#define WORLD_Y 24
#define WORLD_Z 1024
#define SECTOR_SIZE 16
#define SECTOR_COUNT_X (WORLD_X / SECTOR_SIZE)
#define SECTOR_COUNT_Z (WORLD_Z / SECTOR_SIZE)
#define VIEW_SECTOR_RADIUS 1
#define PLAYER_RADIUS 0.28f
#define PLAYER_HEIGHT 1.8f
#define INTERACTION_REACH 6.0f
#define DAY_LENGTH 240.0f

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
    BLOCK_WORKBENCH,
    BLOCK_ROOF
};

enum InventoryType {
    INV_DIRT = 0,
    INV_STONE,
    INV_WOOD,
    INV_PLANK,
    INV_SAND,
    INV_COUNT
};

typedef struct {
    unsigned char blocks[WORLD_Y][WORLD_Z][WORLD_X];
    bool loaded[SECTOR_COUNT_Z][SECTOR_COUNT_X];
    bool sawmillBuilt;
} World;

typedef struct {
    Vector3 feet;
    float yaw;
    float pitch;
    float verticalSpeed;
    bool grounded;
} Player;

typedef struct {
    Player player;
    int inventory[INV_COUNT];
    int selectedSlot;
    float hunger;
    float thirst;
    float energy;
    float money;
    float dayTime;
    int day;
    bool gameOver;
    char message[100];
    float messageTimer;
} Game;

typedef struct {
    int x;
    int y;
    int z;
    RayCollision collision;
} BlockTarget;

static const char *slotNames[INV_COUNT] = {
    "TERRA", "PEDRA", "TRONCO", "TABUA", "AREIA"
};

static unsigned int worldSeed;

static int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static bool inWorld(int x, int y, int z) {
    return x >= 0 && x < WORLD_X && y >= 0 && y < WORLD_Y && z >= 0 && z < WORLD_Z;
}

static int getBlock(const World *world, int x, int y, int z) {
    if (!inWorld(x, y, z)) return BLOCK_EMPTY;
    if (!world->loaded[z / SECTOR_SIZE][x / SECTOR_SIZE]) return BLOCK_EMPTY;
    return world->blocks[y][z][x];
}

static void setBlock(World *world, int x, int y, int z, int block) {
    if (inWorld(x, y, z)) world->blocks[y][z][x] = (unsigned char)block;
}

static bool isSolid(int block) {
    return block != BLOCK_EMPTY && block != BLOCK_WATER;
}

static int blockToInventory(int block) {
    switch (block) {
        case BLOCK_GRASS:
        case BLOCK_DIRT: return INV_DIRT;
        case BLOCK_STONE: return INV_STONE;
        case BLOCK_LOG:
        case BLOCK_LEAVES: return INV_WOOD;
        case BLOCK_PLANK:
        case BLOCK_WORKBENCH:
        case BLOCK_ROOF: return INV_PLANK;
        case BLOCK_SAND: return INV_SAND;
        default: return -1;
    }
}

static int slotBlock(int slot) {
    switch (slot) {
        case 0: return BLOCK_DIRT;
        case 1: return BLOCK_STONE;
        case 2: return BLOCK_LOG;
        case 3: return BLOCK_PLANK;
        case 4: return BLOCK_SAND;
        default: return BLOCK_DIRT;
    }
}

static Color blockColor(int block) {
    switch (block) {
        case BLOCK_GRASS: return (Color){104, 160, 82, 255};
        case BLOCK_DIRT: return (Color){133, 94, 59, 255};
        case BLOCK_STONE: return (Color){122, 124, 127, 255};
        case BLOCK_SAND: return (Color){213, 187, 119, 255};
        case BLOCK_WATER: return (Color){58, 126, 183, 255};
        case BLOCK_LOG: return (Color){117, 76, 42, 255};
        case BLOCK_LEAVES: return (Color){67, 133, 65, 255};
        case BLOCK_PLANK: return (Color){164, 119, 72, 255};
        case BLOCK_WORKBENCH: return (Color){92, 61, 37, 255};
        case BLOCK_ROOF: return (Color){142, 93, 57, 255};
        default: return BLANK;
    }
}

static unsigned int hashCoordinates(int x, int z) {
    unsigned int hash = (unsigned int)x * 374761393u + (unsigned int)z * 668265263u + worldSeed;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    return hash ^ (hash >> 16);
}

static float latticeNoise(int x, int z) {
    return (float)(hashCoordinates(x, z) & 0x00ffffffu) / 8388607.5f - 1.0f;
}

static float smoothStep(float value) {
    return value * value * (3.0f - 2.0f * value);
}

static float interpolatedNoise(float x, float z) {
    int x0 = (int)floorf(x);
    int z0 = (int)floorf(z);
    float fx = smoothStep(x - (float)x0);
    float fz = smoothStep(z - (float)z0);
    float north = latticeNoise(x0, z0) * (1.0f - fx) + latticeNoise(x0 + 1, z0) * fx;
    float south = latticeNoise(x0, z0 + 1) * (1.0f - fx) + latticeNoise(x0 + 1, z0 + 1) * fx;
    return north * (1.0f - fz) + south * fz;
}

static int terrainHeight(int x, int z) {
    float broad = interpolatedNoise((float)x * 0.008f, (float)z * 0.008f);
    float hills = interpolatedNoise((float)x * 0.035f, (float)z * 0.035f);
    float detail = interpolatedNoise((float)x * 0.11f, (float)z * 0.11f);
    return clampInt(7 + (int)roundf(broad * 4.0f + hills * 2.0f + detail * 0.7f), 2, 14);
}

static void setIfEmpty(World *world, int x, int y, int z, int block) {
    if (getBlock(world, x, y, z) == BLOCK_EMPTY) setBlock(world, x, y, z, block);
}

static void buildSawmill(World *world) {
    const int x0 = 13;
    const int z0 = 12;
    const int floorY = terrainHeight(x0 + 2, z0 + 1) + 1;

    for (int x = x0; x < x0 + 5; x++) {
        for (int z = z0; z < z0 + 4; z++) {
            for (int y = floorY; y < floorY + 3; y++) {
                bool wall = x == x0 || x == x0 + 4 || z == z0 || z == z0 + 3;
                bool doorway = z == z0 + 3 && x == x0 + 2 && y < floorY + 2;
                if (wall && !doorway) setBlock(world, x, y, z, BLOCK_PLANK);
            }
            setBlock(world, x, floorY + 3, z, BLOCK_ROOF);
        }
    }

    setBlock(world, x0 + 2, floorY, z0 + 1, BLOCK_WORKBENCH);
    setBlock(world, x0 + 1, floorY, z0 + 1, BLOCK_LOG);
    setBlock(world, x0 + 3, floorY, z0 + 1, BLOCK_LOG);
}

static void generateSector(World *world, int sectorX, int sectorZ) {
    if (sectorX < 0 || sectorX >= SECTOR_COUNT_X || sectorZ < 0 || sectorZ >= SECTOR_COUNT_Z ||
        world->loaded[sectorZ][sectorX]) {
        return;
    }

    world->loaded[sectorZ][sectorX] = true;
    int startX = sectorX * SECTOR_SIZE;
    int endX = startX + SECTOR_SIZE;
    int startZ = sectorZ * SECTOR_SIZE;
    int endZ = startZ + SECTOR_SIZE;

    for (int x = startX; x < endX; x++) {
        for (int z = startZ; z < endZ; z++) {
            int height = terrainHeight(x, z);
            bool desert = (x > 55 && x < 245 && z > 25 && z < 250) ||
                          interpolatedNoise((float)x * 0.004f + 41.0f, (float)z * 0.004f - 17.0f) > 0.62f;
            for (int y = 0; y <= height; y++) {
                int block = (y == height) ? (desert ? BLOCK_SAND : BLOCK_GRASS) :
                            (y >= height - 3 ? BLOCK_DIRT : BLOCK_STONE);
                if (desert && y == height - 1) block = BLOCK_SAND;
                setBlock(world, x, y, z, block);
            }

            if (x >= 29 && x <= 35 && z >= 25 && z <= 30 && height <= 5) {
                setBlock(world, x, height + 1, z, BLOCK_WATER);
            }

            unsigned int treeHash = hashCoordinates(x, z);
            bool nearStartingTown = x < 24 && z < 24;
            bool nearWater = x >= 27 && x <= 37 && z >= 23 && z <= 32;
            if (!desert && !nearStartingTown && !nearWater && (treeHash % 1000u) < 22u) {
                int treeHeight = 3 + (int)((treeHash >> 10) % 3u);
                int baseY = height + 1;

                for (int y = 0; y < treeHeight; y++) {
                    setIfEmpty(world, x, baseY + y, z, BLOCK_LOG);
                }

                int canopyY = baseY + treeHeight - 1;
                for (int dy = -1; dy <= 2; dy++) {
                    int radius = (dy == 2) ? 1 : 2;
                    for (int dx = -radius; dx <= radius; dx++) {
                        for (int dz = -radius; dz <= radius; dz++) {
                            if (abs(dx) + abs(dz) <= radius + 1) {
                                setIfEmpty(world, x + dx, canopyY + dy, z + dz, BLOCK_LEAVES);
                            }
                        }
                    }
                }
            }
        }
    }

}

static void loadSectorsAroundPlayer(World *world, float x, float z) {
    int centerSectorX = (int)floorf(x) / SECTOR_SIZE;
    int centerSectorZ = (int)floorf(z) / SECTOR_SIZE;

    for (int sectorZ = centerSectorZ - VIEW_SECTOR_RADIUS; sectorZ <= centerSectorZ + VIEW_SECTOR_RADIUS; sectorZ++) {
        for (int sectorX = centerSectorX - VIEW_SECTOR_RADIUS; sectorX <= centerSectorX + VIEW_SECTOR_RADIUS; sectorX++) {
            generateSector(world, sectorX, sectorZ);
        }
    }

    if (!world->sawmillBuilt && world->loaded[0][0] && world->loaded[0][1]) {
        buildSawmill(world);
        world->sawmillBuilt = true;
    }
}

static void setMessage(Game *game, const char *text) {
    snprintf(game->message, sizeof(game->message), "%s", text);
    game->messageTimer = 2.5f;
}

static void resetGame(Game *game) {
    memset(game, 0, sizeof(*game));
    game->player.feet = (Vector3){8.5f, (float)terrainHeight(8, 8) + 1.0f, 8.5f};
    game->player.yaw = 0.0f;
    game->player.pitch = 0.0f;
    game->player.grounded = true;
    game->inventory[INV_DIRT] = 12;
    game->inventory[INV_STONE] = 5;
    game->inventory[INV_WOOD] = 8;
    game->inventory[INV_PLANK] = 8;
    game->inventory[INV_SAND] = 0;
    game->selectedSlot = INV_WOOD;
    game->hunger = 100.0f;
    game->thirst = 100.0f;
    game->energy = 100.0f;
    game->money = 12.0f;
    game->day = 1;
    game->dayTime = 60.0f;
}

static bool playerCollides(const World *world, Vector3 feet) {
    int minX = (int)floorf(feet.x - PLAYER_RADIUS);
    int maxX = (int)floorf(feet.x + PLAYER_RADIUS);
    int minY = (int)floorf(feet.y + 0.02f);
    int maxY = (int)floorf(feet.y + PLAYER_HEIGHT - 0.02f);
    int minZ = (int)floorf(feet.z - PLAYER_RADIUS);
    int maxZ = (int)floorf(feet.z + PLAYER_RADIUS);

    if (minX < 0 || maxX >= WORLD_X || minZ < 0 || maxZ >= WORLD_Z || minY < 0 || maxY >= WORLD_Y) {
        return true;
    }

    for (int y = minY; y <= maxY; y++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                if (isSolid(getBlock(world, x, y, z))) return true;
            }
        }
    }
    return false;
}

static bool intersectsPlayer(const Game *game, int x, int y, int z) {
    Vector3 p = game->player.feet;
    return p.x + PLAYER_RADIUS > (float)x && p.x - PLAYER_RADIUS < (float)(x + 1) &&
           p.y + PLAYER_HEIGHT > (float)y && p.y < (float)(y + 1) &&
           p.z + PLAYER_RADIUS > (float)z && p.z - PLAYER_RADIUS < (float)(z + 1);
}

static void movePlayer(Game *game, const World *world, float forwardInput, float sideInput, float dt) {
    float inputLength = sqrtf(forwardInput * forwardInput + sideInput * sideInput);
    if (inputLength > 1.0f) {
        forwardInput /= inputLength;
        sideInput /= inputLength;
    }

    float speed = IsKeyDown(KEY_LEFT_SHIFT) ? 7.0f : 4.8f;
    float forwardX = sinf(game->player.yaw);
    float forwardZ = cosf(game->player.yaw);
    float rightX = -cosf(game->player.yaw);
    float rightZ = sinf(game->player.yaw);
    float dx = (forwardX * forwardInput + rightX * sideInput) * speed * dt;
    float dz = (forwardZ * forwardInput + rightZ * sideInput) * speed * dt;

    Vector3 next = game->player.feet;
    next.x += dx;
    if (!playerCollides(world, next)) {
        game->player.feet.x = next.x;
    } else if (game->player.grounded) {
        next.y += 1.0f;
        if (!playerCollides(world, next)) {
            game->player.feet.x = next.x;
            game->player.feet.y = next.y;
        }
    }

    next = game->player.feet;
    next.z += dz;
    if (!playerCollides(world, next)) {
        game->player.feet.z = next.z;
    } else if (game->player.grounded) {
        next.y += 1.0f;
        if (!playerCollides(world, next)) {
            game->player.feet.z = next.z;
            game->player.feet.y = next.y;
        }
    }

    if (fabsf(dx) + fabsf(dz) > 0.001f && IsKeyDown(KEY_LEFT_SHIFT)) {
        game->energy = fmaxf(0.0f, game->energy - dt * 0.8f);
    }
}

static bool findTarget(const World *world, Camera3D camera, BlockTarget *target) {
    Ray ray = GetMouseRay((Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, camera);
    float nearest = INTERACTION_REACH;
    bool found = false;

    int centerX = (int)floorf(camera.position.x);
    int centerY = (int)floorf(camera.position.y);
    int centerZ = (int)floorf(camera.position.z);

    for (int y = centerY - 7; y <= centerY + 7; y++) {
        for (int z = centerZ - 7; z <= centerZ + 7; z++) {
            for (int x = centerX - 7; x <= centerX + 7; x++) {
                if (getBlock(world, x, y, z) == BLOCK_EMPTY) continue;
                BoundingBox box = {
                    (Vector3){(float)x, (float)y, (float)z},
                    (Vector3){(float)x + 1.0f, (float)y + 1.0f, (float)z + 1.0f}
                };
                RayCollision collision = GetRayCollisionBox(ray, box);
                if (collision.hit && collision.distance < nearest) {
                    nearest = collision.distance;
                    target->x = x;
                    target->y = y;
                    target->z = z;
                    target->collision = collision;
                    found = true;
                }
            }
        }
    }
    return found;
}

static bool isNearSawmill(const Game *game) {
    float dx = game->player.feet.x - 15.5f;
    float dz = game->player.feet.z - 13.5f;
    return dx * dx + dz * dz <= 36.0f;
}

static void workAtSawmill(Game *game) {
    if (!isNearSawmill(game)) {
        setMessage(game, "Procure a serraria para trabalhar.");
        return;
    }
    if (game->energy < 12.0f) {
        setMessage(game, "Voce esta cansado demais para trabalhar.");
        return;
    }
    game->money += 4.0f;
    game->energy -= 12.0f;
    game->hunger = fmaxf(0.0f, game->hunger - 2.0f);
    game->thirst = fmaxf(0.0f, game->thirst - 3.0f);
    setMessage(game, "Turno concluido: recebeu $4.00.");
}

static void craftPlanks(Game *game) {
    if (!isNearSawmill(game)) {
        setMessage(game, "Use a bancada dentro da serraria para serrar madeira.");
        return;
    }
    if (game->inventory[INV_WOOD] < 1) {
        setMessage(game, "Voce nao tem troncos para serrar.");
        return;
    }
    game->inventory[INV_WOOD]--;
    game->inventory[INV_PLANK] += 4;
    setMessage(game, "Um tronco virou quatro tabuas.");
}

static void buyFood(Game *game) {
    if (game->money < 2.0f) {
        setMessage(game, "Dinheiro insuficiente: comida custa $2.00.");
        return;
    }
    game->money -= 2.0f;
    game->hunger = fminf(100.0f, game->hunger + 38.0f);
    setMessage(game, "Voce comprou uma refeicao no saloon.");
}

static void buyWater(Game *game) {
    if (game->money < 1.0f) {
        setMessage(game, "Dinheiro insuficiente: agua custa $1.00.");
        return;
    }
    game->money -= 1.0f;
    game->thirst = fminf(100.0f, game->thirst + 45.0f);
    setMessage(game, "Voce comprou um cantil de agua.");
}

static void rest(Game *game) {
    game->energy = fminf(100.0f, game->energy + 35.0f);
    game->hunger = fmaxf(0.0f, game->hunger - 3.0f);
    game->thirst = fmaxf(0.0f, game->thirst - 4.0f);
    setMessage(game, "Voce descansou um pouco.");
}

static void mineBlock(Game *game, World *world, BlockTarget target) {
    int block = getBlock(world, target.x, target.y, target.z);
    if (block == BLOCK_WATER || block == BLOCK_WORKBENCH || block == BLOCK_ROOF) {
        setMessage(game, "Esse bloco nao pode ser recolhido.");
        return;
    }

    int inventory = blockToInventory(block);
    if (inventory < 0) return;
    setBlock(world, target.x, target.y, target.z, BLOCK_EMPTY);
    if (block == BLOCK_LEAVES && rand() % 3 != 0) {
        setMessage(game, "Folhas removidas.");
        return;
    }
    game->inventory[inventory]++;
    if (block == BLOCK_GRASS) setMessage(game, "Bloco de grama recolhido como terra.");
    else setMessage(game, "Recurso recolhido.");
}

static void placeBlock(Game *game, World *world, BlockTarget target) {
    Vector3 normal = target.collision.normal;
    int nx = (fabsf(normal.x) > 0.5f) ? (normal.x > 0.0f ? 1 : -1) : 0;
    int ny = (fabsf(normal.y) > 0.5f) ? (normal.y > 0.0f ? 1 : -1) : 0;
    int nz = (fabsf(normal.z) > 0.5f) ? (normal.z > 0.0f ? 1 : -1) : 0;
    int x = target.x + nx;
    int y = target.y + ny;
    int z = target.z + nz;
    int block = slotBlock(game->selectedSlot);

    if (!inWorld(x, y, z) || getBlock(world, x, y, z) != BLOCK_EMPTY || intersectsPlayer(game, x, y, z)) return;
    if (game->inventory[game->selectedSlot] <= 0) {
        setMessage(game, "Voce nao tem mais desse bloco.");
        return;
    }

    setBlock(world, x, y, z, block);
    game->inventory[game->selectedSlot]--;
}

static bool hasExposedFace(const World *world, int x, int y, int z) {
    return getBlock(world, x + 1, y, z) == BLOCK_EMPTY ||
           getBlock(world, x - 1, y, z) == BLOCK_EMPTY ||
           getBlock(world, x, y + 1, z) == BLOCK_EMPTY ||
           getBlock(world, x, y - 1, z) == BLOCK_EMPTY ||
           getBlock(world, x, y, z + 1) == BLOCK_EMPTY ||
           getBlock(world, x, y, z - 1) == BLOCK_EMPTY;
}

static void drawWorld(const World *world, const Game *game, const BlockTarget *target, bool hasTarget) {
    int playerChunkX = (int)floorf(game->player.feet.x) / SECTOR_SIZE;
    int playerChunkZ = (int)floorf(game->player.feet.z) / SECTOR_SIZE;
    int minX = (playerChunkX - VIEW_SECTOR_RADIUS) * SECTOR_SIZE;
    int maxX = (playerChunkX + VIEW_SECTOR_RADIUS + 1) * SECTOR_SIZE - 1;
    int minZ = (playerChunkZ - VIEW_SECTOR_RADIUS) * SECTOR_SIZE;
    int maxZ = (playerChunkZ + VIEW_SECTOR_RADIUS + 1) * SECTOR_SIZE - 1;
    minX = clampInt(minX, 0, WORLD_X - 1);
    maxX = clampInt(maxX, 0, WORLD_X - 1);
    minZ = clampInt(minZ, 0, WORLD_Z - 1);
    maxZ = clampInt(maxZ, 0, WORLD_Z - 1);

    for (int y = 0; y < WORLD_Y; y++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                int block = getBlock(world, x, y, z);
                if (block == BLOCK_EMPTY || !hasExposedFace(world, x, y, z)) continue;

                Vector3 center = {(float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f};
                DrawCube(center, 1.0f, 1.0f, 1.0f, blockColor(block));
                DrawCubeWires(center, 1.0f, 1.0f, 1.0f, (Color){30, 32, 31, 70});
            }
        }
    }

    if (hasTarget) {
        Vector3 center = {(float)target->x + 0.5f, (float)target->y + 0.5f, (float)target->z + 0.5f};
        DrawCubeWires(center, 1.02f, 1.02f, 1.02f, YELLOW);
    }

    Vector3 sunDirection = {cosf(game->dayTime * 6.28318f / DAY_LENGTH), 0.35f, sinf(game->dayTime * 6.28318f / DAY_LENGTH)};
    Vector3 sun = {
        game->player.feet.x + sunDirection.x * 28.0f,
        14.0f + sunDirection.y * 18.0f,
        game->player.feet.z + sunDirection.z * 28.0f
    };
    DrawSphere(sun, 2.0f, (Color){255, 218, 132, 255});
}

static Color skyColor(float dayTime) {
    float daylight = 0.5f + 0.5f * sinf(dayTime * 6.28318f / DAY_LENGTH);
    daylight = fmaxf(0.12f, daylight);
    return (Color){
        (unsigned char)(22.0f + daylight * 110.0f),
        (unsigned char)(31.0f + daylight * 145.0f),
        (unsigned char)(54.0f + daylight * 167.0f),
        255
    };
}

static void drawBar(int x, int y, int width, float amount, Color color) {
    DrawRectangle(x, y, width, 10, (Color){35, 37, 39, 230});
    DrawRectangle(x, y, (int)((float)width * amount / 100.0f), 10, color);
}

static void drawHud(const Game *game, bool hasTarget, int targetBlock) {
    DrawRectangle(16, 16, 304, 142, (Color){20, 23, 25, 215});
    DrawText(TextFormat("DIA %d", game->day), 30, 27, 24, RAYWHITE);
    DrawText(TextFormat("FOME  %3.0f%%", game->hunger), 30, 62, 16, (Color){237, 144, 83, 255});
    drawBar(132, 67, 164, game->hunger, (Color){210, 121, 65, 255});
    DrawText(TextFormat("SEDE  %3.0f%%", game->thirst), 30, 90, 16, (Color){103, 180, 232, 255});
    drawBar(132, 95, 164, game->thirst, (Color){63, 143, 205, 255});
    DrawText(TextFormat("VIGOR %3.0f%%", game->energy), 30, 118, 16, (Color){150, 211, 119, 255});
    drawBar(132, 123, 164, game->energy, (Color){115, 180, 83, 255});
    DrawText(TextFormat("DINHEIRO  $%.2f", game->money), 30, 140, 15, GOLD);

    int slotWidth = 132;
    int gap = 8;
    int startX = (SCREEN_WIDTH - (INV_COUNT * slotWidth + (INV_COUNT - 1) * gap)) / 2;
    int slotY = SCREEN_HEIGHT - 102;
    for (int slot = 0; slot < INV_COUNT; slot++) {
        int x = startX + slot * (slotWidth + gap);
        DrawRectangle(x, slotY, slotWidth, 70, (Color){25, 27, 29, 225});
        DrawRectangle(x + 6, slotY + 7, 38, 38, blockColor(slotBlock(slot)));
        DrawRectangleLines(x, slotY, slotWidth, 70, slot == game->selectedSlot ? GOLD : (Color){116, 119, 121, 220});
        DrawText(TextFormat("%d", slot + 1), x + 10, slotY + 49, 14, LIGHTGRAY);
        DrawText(slotNames[slot], x + 50, slotY + 12, 13, RAYWHITE);
        DrawText(TextFormat("x%d", game->inventory[slot]), x + 50, slotY + 33, 17, RAYWHITE);
    }

    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;
    DrawLine(centerX - 8, centerY, centerX - 2, centerY, RAYWHITE);
    DrawLine(centerX + 2, centerY, centerX + 8, centerY, RAYWHITE);
    DrawLine(centerX, centerY - 8, centerX, centerY - 2, RAYWHITE);
    DrawLine(centerX, centerY + 2, centerX, centerY + 8, RAYWHITE);

    DrawRectangle(SCREEN_WIDTH - 385, 16, 368, 118, (Color){20, 23, 25, 190});
    DrawText("WASD mover | Mouse olhar | Espaco pular", SCREEN_WIDTH - 372, 28, 15, RAYWHITE);
    DrawText("Clique esquerdo minerar | direito colocar", SCREEN_WIDTH - 372, 52, 15, RAYWHITE);
    DrawText("E trabalho | C serrar madeira | F comida", SCREEN_WIDTH - 372, 76, 15, RAYWHITE);
    DrawText("G agua | R descansar | Shift correr", SCREEN_WIDTH - 372, 100, 15, RAYWHITE);

    if (hasTarget) {
        DrawRectangle(SCREEN_WIDTH / 2 - 110, centerY + 24, 220, 24, (Color){20, 23, 25, 180});
        DrawText(TextFormat("Bloco: %d", targetBlock), SCREEN_WIDTH / 2 - 40, centerY + 28, 14, RAYWHITE);
    }
    if (isNearSawmill(game)) {
        DrawText("SERRARIA: E trabalhar | C criar tabuas", SCREEN_WIDTH / 2 - 170, 22, 17, GOLD);
    }
    if (game->messageTimer > 0.0f) {
        int width = MeasureText(game->message, 18);
        DrawRectangle(SCREEN_WIDTH / 2 - width / 2 - 14, 58, width + 28, 32, (Color){24, 27, 29, 220});
        DrawText(game->message, SCREEN_WIDTH / 2 - width / 2, 65, 18, RAYWHITE);
    }
}

static void updateCameraFromPlayer(const Game *game, Camera3D *camera) {
    Vector3 direction = {
        sinf(game->player.yaw) * cosf(game->player.pitch),
        sinf(game->player.pitch),
        cosf(game->player.yaw) * cosf(game->player.pitch)
    };
    camera->position = (Vector3){game->player.feet.x, game->player.feet.y + 1.62f, game->player.feet.z};
    camera->target = (Vector3){
        camera->position.x + direction.x,
        camera->position.y + direction.y,
        camera->position.z + direction.z
    };
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fronteira Arida - Sobrevivencia 3D");
    SetTargetFPS(60);
    DisableCursor();
    srand((unsigned int)time(NULL));

    worldSeed = (unsigned int)time(NULL);
    World *world = (World *)calloc(1, sizeof(*world));
    if (world == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel alocar memoria para o mundo procedural.");
        CloseWindow();
        return 1;
    }

    Game game;
    resetGame(&game);
    loadSectorsAroundPlayer(world, game.player.feet.x, game.player.feet.z);

    Camera3D camera = {0};
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);

        if (!game.gameOver) {
            Vector2 mouseDelta = GetMouseDelta();
            game.player.yaw -= mouseDelta.x * 0.0025f;
            game.player.pitch -= mouseDelta.y * 0.0025f;
            game.player.pitch = fmaxf(-1.25f, fminf(1.25f, game.player.pitch));

            float forward = (IsKeyDown(KEY_W) ? 1.0f : 0.0f) - (IsKeyDown(KEY_S) ? 1.0f : 0.0f);
            float side = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) - (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
            movePlayer(&game, world, forward, side, dt);
            loadSectorsAroundPlayer(world, game.player.feet.x, game.player.feet.z);

            if (IsKeyPressed(KEY_SPACE) && game.player.grounded) {
                game.player.verticalSpeed = 8.0f;
                game.player.grounded = false;
            }

            game.player.verticalSpeed -= 22.0f * dt;
            Vector3 nextFeet = game.player.feet;
            nextFeet.y += game.player.verticalSpeed * dt;
            if (!playerCollides(world, nextFeet)) {
                game.player.feet.y = nextFeet.y;
                game.player.grounded = false;
            } else if (game.player.verticalSpeed < 0.0f) {
                int px0 = (int)floorf(game.player.feet.x - PLAYER_RADIUS);
                int px1 = (int)floorf(game.player.feet.x + PLAYER_RADIUS);
                int pz0 = (int)floorf(game.player.feet.z - PLAYER_RADIUS);
                int pz1 = (int)floorf(game.player.feet.z + PLAYER_RADIUS);
                float floorY = -1.0f;
                for (int z = pz0; z <= pz1; z++) {
                    for (int x = px0; x <= px1; x++) {
                        int y = (int)floorf(game.player.feet.y);
                        for (; y >= 0; y--) {
                            if (isSolid(getBlock(world, x, y, z))) {
                                if ((float)(y + 1) > floorY) floorY = (float)(y + 1);
                                break;
                            }
                        }
                    }
                }
                if (floorY >= 0.0f) game.player.feet.y = floorY;
                game.player.verticalSpeed = 0.0f;
                game.player.grounded = true;
            } else {
                game.player.verticalSpeed = 0.0f;
            }

            if (IsKeyPressed(KEY_ONE)) game.selectedSlot = 0;
            if (IsKeyPressed(KEY_TWO)) game.selectedSlot = 1;
            if (IsKeyPressed(KEY_THREE)) game.selectedSlot = 2;
            if (IsKeyPressed(KEY_FOUR)) game.selectedSlot = 3;
            if (IsKeyPressed(KEY_FIVE)) game.selectedSlot = 4;
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                game.selectedSlot = (game.selectedSlot + (wheel > 0.0f ? INV_COUNT - 1 : 1)) % INV_COUNT;
            }

            game.dayTime += dt;
            if (game.dayTime >= DAY_LENGTH) {
                game.dayTime -= DAY_LENGTH;
                game.day++;
            }
            game.hunger = fmaxf(0.0f, game.hunger - dt * 0.04f);
            game.thirst = fmaxf(0.0f, game.thirst - dt * 0.07f);
            if (game.messageTimer > 0.0f) game.messageTimer -= dt;

            updateCameraFromPlayer(&game, &camera);
            BlockTarget target = {0};
            bool hasTarget = findTarget(world, camera, &target);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hasTarget) {
                mineBlock(&game, world, target);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hasTarget) {
                placeBlock(&game, world, target);
            }

            if (IsKeyPressed(KEY_E)) workAtSawmill(&game);
            if (IsKeyPressed(KEY_C)) craftPlanks(&game);
            if (IsKeyPressed(KEY_F)) buyFood(&game);
            if (IsKeyPressed(KEY_G)) buyWater(&game);
            if (IsKeyPressed(KEY_R)) rest(&game);

            if (game.hunger <= 0.0f || game.thirst <= 0.0f || game.energy <= 0.0f) {
                game.gameOver = true;
            }

            BeginDrawing();
            ClearBackground(skyColor(game.dayTime));
            BeginMode3D(camera);
            drawWorld(world, &game, &target, hasTarget);
            EndMode3D();
            drawHud(&game, hasTarget, hasTarget ? getBlock(world, target.x, target.y, target.z) : BLOCK_EMPTY);
            EndDrawing();
        } else {
            if (IsKeyPressed(KEY_ENTER)) {
                resetGame(&game);
                updateCameraFromPlayer(&game, &camera);
            }

            BeginDrawing();
            ClearBackground((Color){25, 29, 33, 255});
            DrawText("A FRONTEIRA COBROU SEU PRECO", SCREEN_WIDTH / 2 - 245, SCREEN_HEIGHT / 2 - 48, 28, (Color){221, 104, 74, 255});
            DrawText("Pressione ENTER para recomecar", SCREEN_WIDTH / 2 - 175, SCREEN_HEIGHT / 2 + 10, 20, RAYWHITE);
            EndDrawing();
        }
    }

    free(world);
    CloseWindow();
    return 0;
}
