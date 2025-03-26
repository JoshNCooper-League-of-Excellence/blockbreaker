#include "include/raylib.h"
#include "include/game.h"
#include "include/serialize.h"
#include "raylib.h"
#include "raymath.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

void chunk_generate(Chunk *chunk, float x_offset) {
  chunk->bounds = (Rectangle){
    .x = x_offset,
    .y = 0,
    .width = BLOCK_SIZE_X * GRID_X,
    .height = BLOCK_SIZE_Y * GRID_Y,
  };
  const int start_depth = 6;
  for (int y = 0; y < BLOCK_SIZE_Y; ++y) {
    for (int x = 0; x < BLOCK_SIZE_Y; ++x) {
      if (y < start_depth) {
        chunk->blocks[y][x] = BLOCK_TYPE_AIR;
        continue;
      }

      if (y - start_depth == 0) {
        chunk->blocks[y][x] = BLOCK_TYPE_GRASS;
      } else if (y - start_depth < 2) {
        chunk->blocks[y][x] = (double)((double)rand() / RAND_MAX) > 0.5
                                  ? BLOCK_TYPE_GRASS
                                  : BLOCK_TYPE_DIRT;
      } else if (y - start_depth < (GRID_Y)) {
        chunk->blocks[y][x] = (double)((double)rand() / RAND_MAX) > 0.5
                                  ? BLOCK_TYPE_STONE
                                  : BLOCK_TYPE_DIRT;
      }
    }
  }
}

void chunk_draw(Chunk *chunk, Texture2D const* textures, int selected_block_type, Vector2 mouse) {
  bool pointer_in_chunk = CheckCollisionPointRec(mouse, chunk->bounds);

  for (int y = 0; y < GRID_Y; ++y) {
    for (int x = 0; x < GRID_X; ++x) {
      Rectangle block_rect = {.x = chunk->bounds.x + x * BLOCK_SIZE_X,
                              .y = y * BLOCK_SIZE_Y,
                              .width = BLOCK_SIZE_X,
                              .height = BLOCK_SIZE_Y};

      BlockType block = chunk->blocks[y][x];
      Texture2D texture = textures[block];
      Rectangle texture_rect = {
          .x = chunk->bounds.x + x * BLOCK_SIZE_X,
          .y = y * BLOCK_SIZE_Y,
          .width = texture.width,
          .height = texture.height,
      };

      if (block != BLOCK_TYPE_AIR) { // We don't draw air. DUh!.
        DrawTexturePro(texture, texture_rect, block_rect, Vector2Zero(), 0.0,
                       WHITE);
      }

      if (pointer_in_chunk && CheckCollisionPointRec(mouse, block_rect)) {
        DrawRectangle(block_rect.x, block_rect.y, block_rect.width,
                      block_rect.height, ColorAlpha(YELLOW, 0.25));

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            block != BLOCK_TYPE_AIR) {
          chunk->blocks[y][x] = BLOCK_TYPE_AIR;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
                   block == BLOCK_TYPE_AIR) {
          chunk->blocks[y][x] = selected_block_type;
        }
      }
    }
  }
}

#include "dirent.h"

void select_filename(char **filename) {
  DIR *dir = opendir("./");
  struct dirent *entry;
  *filename = NULL;
  char *files[16];
  int index = 0;
  while (index < 16 && (entry = readdir(dir)) != NULL) {
    if (strstr(entry->d_name, ".data") != NULL) {
      files[index] = entry->d_name;
      index++;
    }
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    Vector2 begin = {
      250, 150
    };
    for (int i = 0; i < index; ++i) {
      char buffer[1024];
      snprintf(buffer, 1024, "1 : %s", files[i]);
      DrawText(buffer, begin.x, begin.y + (i * 14), 14, WHITE);
    }

    for (int key = KEY_ONE; key < KEY_NINE && (key - KEY_ONE) < index; key++) {
      if (IsKeyPressed(key)) {
        *filename = strdup(files[key - KEY_ONE]);
        EndDrawing();
        goto found_file;
      }
    }

    EndDrawing();
  }
  found_file:
  closedir(dir);
}

int main(void) {
  srand(GetTime());

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Break");
  SetTargetFPS(60);

  const Texture2D textures[] = {
      LoadTexture("assets/grass.jpg"),
      LoadTexture("assets/dirt.jpg"),
      LoadTexture("assets/stone.jpg"),
  };


  int selected_block_type = BLOCK_TYPE_STONE;
  double ui_action_last_time = 0.0f;
  Camera2D camera = {0};
  camera.rotation = 0.0;
  camera.offset = (Vector2){0, 10};
  camera.target = (Vector2){0, 0};
  camera.zoom = 1.0;

  Chunk chunks[24];

  char * filename;

  reset_world:
  select_filename(&filename);
  if (filename && FileExists(filename)) {
    read_world_from_file(&camera, chunks, filename);
  } else {
    for (int i = 0; i < 24; ++i) {
      float x_offset = i * (BLOCK_SIZE_X * GRID_X);
      chunk_generate(&chunks[i], x_offset);
    } 
  }
  
  while (!WindowShouldClose()) {
    BeginDrawing();
    BeginMode2D(camera);
    ClearBackground(SKYBLUE);

    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    for (int i = 0; i < N_CHUNKS; ++i) {
      chunk_draw(&chunks[i], textures, selected_block_type, mouse);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
      EndMode2D();
      EndDrawing();
    
      while (true) {
        BeginDrawing();
        BeginMode2D(camera);
        if (WindowShouldClose()) {
          CloseWindow();
          return 0;
        }
        DrawText("are you sure you want to reset the world?\npress [y/n] to confirm/cancel", 250, 350, 25, RED);
        if (IsKeyPressed(KEY_Y)) {
          goto reset_world;
        } else if (IsKeyPressed(KEY_N)) {
          break;
        }
        EndMode2D();
        EndDrawing();
      }
      continue;
    }

    Vector2 last_pos;
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
      last_pos = GetMousePosition();
      DisableCursor();
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
      EnableCursor();
      SetMousePosition(last_pos.x, last_pos.y);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      Vector2 delta = GetMouseDelta();
      camera.target.x -= delta.x;
    } 

    int scroll = GetMouseWheelMove();
    if (scroll != 0) {
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        camera.zoom += (float)scroll / 100;
      }
      ui_action_last_time = GetTime();
      selected_block_type -= scroll;
      selected_block_type = Clamp(selected_block_type, 0, BLOCK_TYPE_STONE);
    }

    double delta = GetTime() - ui_action_last_time;

    const float ELEMENT_SIZE = BLOCK_SIZE_Y;

    Vector2 ui_start_position = GetScreenToWorld2D((Vector2){
        (float)SCREEN_WIDTH / 2 - (2 * ELEMENT_SIZE),
        SCREEN_HEIGHT - ELEMENT_SIZE,
    }, camera);

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
          static char *names[3] = {
              "Grass",
              "Dirt",
              "Stone",
          };
          color = WHITE;

          DrawText(names[i], ui_start_position.x,
                   ui_start_position.y - ELEMENT_SIZE / 2, 16, WHITE);
        }
        DrawRectangleLines(ui_start_position.x, ui_start_position.y,
                           ELEMENT_SIZE, ELEMENT_SIZE, color);
        ui_start_position.x += ELEMENT_SIZE;
      }
    }

    EndMode2D();
    EndDrawing();
  }


  write_world_to_file(&camera, chunks, "game.data");

  return 0;
}