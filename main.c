#include "include/raylib.h"
#include "raylib.h"
#include "raymath.h"
#include "stdlib.h"
#include "time.h"
#include <stdio.h>

typedef enum {
  BLOCK_TYPE_AIR = -1,
  BLOCK_TYPE_GRASS,
  BLOCK_TYPE_DIRT,
  BLOCK_TYPE_STONE,
} BlockType;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GRID_X = 16;
const int GRID_Y = 12;
const int BLOCK_SIZE_X = SCREEN_WIDTH / GRID_X;
const int BLOCK_SIZE_Y = SCREEN_HEIGHT / GRID_Y;

int main(void) {
  srand(GetTime());

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Break");
  SetTargetFPS(60);

  BlockType blocks[BLOCK_SIZE_Y][BLOCK_SIZE_X];
  const int start_depth = 6;
  for (int y = 0; y < BLOCK_SIZE_Y; ++y) {
    for (int x = 0; x < BLOCK_SIZE_Y; ++x) {
      if (y < start_depth) {
        blocks[y][x] = BLOCK_TYPE_AIR;
        continue;
      }

      if (y - start_depth == 0) {
        blocks[y][x] = BLOCK_TYPE_GRASS;
      } else if (y - start_depth < 2) {
        blocks[y][x] = (double)((double)rand() / RAND_MAX) > 0.5
                           ? BLOCK_TYPE_GRASS
                           : BLOCK_TYPE_DIRT;
      } else if (y - start_depth < (GRID_Y)) {
        blocks[y][x] = (double)((double)rand() / RAND_MAX) > 0.5
                           ? BLOCK_TYPE_STONE
                           : BLOCK_TYPE_DIRT;
      }
    }
  }

  Texture2D textures[] = {
      LoadTexture("assets/grass.jpg"),
      LoadTexture("assets/dirt.jpg"),
      LoadTexture("assets/stone.jpg"),
  };

  int selected_block_type = BLOCK_TYPE_STONE;
  double ui_action_last_time = 0.0f;
  Camera2D camera = {0};
  camera.rotation = 0.0;
  camera.offset = (Vector2){0, 0};
  camera.target = (Vector2){0, 0};
  camera.zoom = 1.0;

  while (!WindowShouldClose()) {
    BeginDrawing();
      BeginMode2D(camera);
      ClearBackground(SKYBLUE);

      if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        camera.target.x += delta.x;
      }

      Vector2 mouse = GetMousePosition();
      for (int y = 0; y < GRID_Y; ++y) {
        for (int x = 0; x < GRID_X; ++x) {
          Rectangle block_rect = {.x = x * BLOCK_SIZE_X,
                                  .y = y * BLOCK_SIZE_Y,
                                  .width = BLOCK_SIZE_X,
                                  .height = BLOCK_SIZE_Y};

          BlockType block = blocks[y][x];
          Texture2D texture = textures[block];
          Rectangle texture_rect = {
              .x = x * BLOCK_SIZE_X,
              .y = y * BLOCK_SIZE_Y,
              .width = texture.width,
              .height = texture.height,
          };

          if (block != BLOCK_TYPE_AIR) { // We don't draw air. DUh!.
            DrawTexturePro(texture, texture_rect, block_rect, Vector2Zero(), 0.0,
                          WHITE);
          }

          if (CheckCollisionPointRec(mouse, block_rect)) {
            DrawRectangle(block_rect.x, block_rect.y, block_rect.width,
                          block_rect.height, ColorAlpha(YELLOW, 0.25));

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                block != BLOCK_TYPE_AIR) {
              blocks[y][x] = BLOCK_TYPE_AIR;
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
                      block == BLOCK_TYPE_AIR) {
              blocks[y][x] = selected_block_type;
            }
          }
        }
      }

      int scroll = GetMouseWheelMove();
      if (scroll != 0) {
        ui_action_last_time = GetTime();
        selected_block_type -= scroll;
        selected_block_type = Clamp(selected_block_type, 0, BLOCK_TYPE_STONE);
      }

      double delta =  GetTime() - ui_action_last_time;

      const float ELEMENT_SIZE = BLOCK_SIZE_Y;

      Vector2 ui_start_position = {
          (float)SCREEN_WIDTH / 2 - (2 * ELEMENT_SIZE),
          SCREEN_HEIGHT - ELEMENT_SIZE,
      };

      if (delta < 1.0) {
        for (int i = 0; i < 3; ++i) {
          Texture2D texture = textures[i];
          Rectangle texture_rect = {
              .x = ui_start_position.x,
              .y = ui_start_position.y,
              .width = texture.width,
              .height = texture.height,
          };
          Rectangle block_rect = {
              .x = ui_start_position.x,
              .y = ui_start_position.y,
              .width = ELEMENT_SIZE,
              .height = ELEMENT_SIZE,
          };

          DrawTexturePro(texture, texture_rect, block_rect, Vector2Zero(), 0.0,
                        WHITE);

          Color color = BLACK;
          if (i == selected_block_type) {
            static char* names[3] = {
              "Grass",
              "Dirt",
              "Stone",
            };
            color = WHITE;

            DrawText(names[i], ui_start_position.x, ui_start_position.y - ELEMENT_SIZE / 2, 16, WHITE);
          }
          DrawRectangleLines(ui_start_position.x, ui_start_position.y,
                            ELEMENT_SIZE, ELEMENT_SIZE, color);
          ui_start_position.x += ELEMENT_SIZE;
        }
      }
    EndMode2D();
    EndDrawing();
  }

  return 0;
}