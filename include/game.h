#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  BLOCK_TYPE_AIR = -1,
  BLOCK_TYPE_GRASS,
  BLOCK_TYPE_DIRT,
  BLOCK_TYPE_STONE,
} BlockType;

#define SCREEN_WIDTH 800
#define GRAVITY 0.5
#define SCREEN_HEIGHT 600
#define GRID_X 16
#define GRID_Y 12
#define BLOCK_SIZE_X (int)(SCREEN_WIDTH / GRID_X)
#define BLOCK_SIZE_Y (int)(SCREEN_HEIGHT / GRID_Y)
#define N_CHUNKS ((int)24)

typedef struct {
  Rectangle bounds;
  BlockType blocks[BLOCK_SIZE_Y][BLOCK_SIZE_X];
} Chunk;

typedef struct {
  Texture2D *frames;
  size_t n_frames;
  size_t frame;
} Animation;


typedef struct {
  Vector2 position;
  Vector2 size;
  Vector2 velocity;
  Animation animation;
} Character;


#endif