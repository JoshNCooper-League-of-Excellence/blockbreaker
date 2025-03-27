#include "dirent.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include "serialize.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

#define fn static inline

fn bool select_filename(char **filename) {
  DIR *dir = opendir("./worlds/");
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

  if (index == 0) {
    return false;
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    Vector2 begin = {250, 150};

    DrawText("press the number to load the file.\npress '-' for a new world.",
             128, 100, 24, WHITE);
    for (int i = 0; i < index; ++i) {
      char buffer[1024];
      snprintf(buffer, 1024, "#%d : %s", i + 1, files[i]);
      DrawText(buffer, begin.x, begin.y + (i * 24), 24, WHITE);
    }

    if (IsKeyPressed(KEY_MINUS)) {
      *filename = nullptr;
      return true;
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
  return false;
}

fn void save_new_world(Camera2D *camera, Chunk *chunks,
                                  char **filename) {
  char buffer[1024] = {0};
  int index = 0;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("type a new world name, then press enter to save.", 150, 200, 24,
             WHITE);
    DrawText(buffer, 350, 250, 24, WHITE);
    if (IsKeyPressed(KEY_ENTER)) {
      buffer[index] = '\0';
      char *copy = strdup(buffer);
      snprintf(buffer, 1024, "worlds/%s.data", copy);
      free(copy);
      if (*filename) {
        free(*filename);
      }
      *filename = strdup(buffer);
      write_world_to_file(camera, chunks, buffer);
      return;
    } else if (IsKeyPressed(KEY_BACKSPACE) && index >= 0) {
      buffer[index--] = '\0';
      if (index < 0) {
        index = 0;
      }
    } else {
      char ch = GetCharPressed();
      if (ch != 0) {
        buffer[index++] = ch;
      }
    }
    EndDrawing();
  }
}

fn void chunk_generate(Chunk *chunk, float x_offset) {
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
        chunk->blocks[y][x] = BLOCK_TYPE_DIRT;
      } else if (y - start_depth < (GRID_Y)) {
        chunk->blocks[y][x] = (double)((double)rand() / RAND_MAX) > 0.5
                                  ? BLOCK_TYPE_STONE
                                  : BLOCK_TYPE_DIRT;
      }
    }
  }
}

fn void chunk_draw(Chunk *chunk, Texture2D const *textures,
                              int selected_block_type, Vector2 mouse,
                              Sound const *sounds) {
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
          .x = 0,
          .y = 0,
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
          PlaySound(sounds[0]);
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
                   block == BLOCK_TYPE_AIR) {
          PlaySound(sounds[1]);
          chunk->blocks[y][x] = selected_block_type;
        }
      }
    }
  }
}

fn Animation load_animation(char *directory) {
  DIR *dir;
  struct dirent *ent;
  size_t capacity = 10;
  size_t count = 0;
  Texture2D *frames = malloc(capacity * sizeof(Texture2D));

  if ((dir = opendir(directory)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (strstr(ent->d_name, ".png") != NULL) {
        if (count >= capacity) {
          capacity *= 2;
          frames = realloc(frames, capacity * sizeof(Texture2D));
        }
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, ent->d_name);
        frames[count] = LoadTexture(filepath);
        count++;
      }
    }
    closedir(dir);
  } else {
    perror("Could not open directory");
  }

  Animation animation = {frames, count, 0};
  return animation;
}

fn Texture2D animation_step(Animation *animation) {
  size_t frame = animation->frame;
  Texture2D texture = animation->frames[frame];
  animation->frame = (animation->frame + 1) % animation->n_frames;
  return texture;
}

fn void check_chunk_collision(Character *character, Chunk *chunks, Rectangle *new_bounds) {
  int start_x = floor(Clamp(new_bounds->x / BLOCK_SIZE_X, 0, N_CHUNKS * GRID_X));
  int end_x = ceil(Clamp((new_bounds->x + new_bounds->width) / BLOCK_SIZE_X, 0, N_CHUNKS * GRID_X));
  int start_y = floor(Clamp(new_bounds->y / BLOCK_SIZE_Y, 0, GRID_Y));
  int end_y = ceil(Clamp((new_bounds->y + new_bounds->height) / BLOCK_SIZE_Y, 0, GRID_Y));

  for (int abs_x = start_x; abs_x < end_x; abs_x++) {
    Chunk *chunk = &chunks[abs_x / GRID_X];
    int x = abs_x - chunk->bounds.x / BLOCK_SIZE_X;
    for (int abs_y = start_y; abs_y < end_y; abs_y++) {
      int y = abs_y - chunk->bounds.y;
      if (chunk->blocks[y][x] == BLOCK_TYPE_AIR) {
        continue;
      }
      Rectangle block_rect = {
          .x = chunk->bounds.x + x * BLOCK_SIZE_X,
          .y = chunk->bounds.y + y * BLOCK_SIZE_Y,
          .width = BLOCK_SIZE_X,
          .height = BLOCK_SIZE_Y,
      };

      Vector2 char_center = {new_bounds->x + new_bounds->width / 2,
                              new_bounds->y + new_bounds->height / 2};
      Vector2 block_center = {block_rect.x + block_rect.width / 2,
                              block_rect.y + block_rect.height / 2};

      Vector2 collision_normal = Vector2Subtract(char_center, block_center);
      float h_offset = (block_rect.width + new_bounds->width) / 2;
      float v_offset = (block_rect.height + new_bounds->height) / 2;
      float overlap_x = h_offset - fabs(collision_normal.x);
      float overlap_y = v_offset - fabs(collision_normal.y);

      if (overlap_y < overlap_x) {
        if (collision_normal.y > 0) {
          new_bounds->y = block_rect.y + v_offset;
        } else {
          new_bounds->y = block_rect.y - v_offset;
        }
        character->velocity.y = 0;
      } else {
        if (collision_normal.x > 0) {
          new_bounds->x = block_rect.x + h_offset;
        } else {
          new_bounds->x = block_rect.x - h_offset;
        }
        character->velocity.x = 0;
      }
    }
  }
}

fn void character_physics(Character *character, Chunk *chunks, Vector2 world_size) {
  character->velocity.y += GRAVITY;

  Rectangle new_bounds = {
    .x = character->position.x + character->velocity.x,
    .y = character->position.y + character->velocity.y,
    .width = character->size.x,
    .height = character->size.y
  };

  check_chunk_collision(character, chunks, &new_bounds);

  character->position = Vector2Clamp((Vector2){new_bounds.x, new_bounds.y}, Vector2Zero(), world_size);
  character->velocity = Vector2Scale(character->velocity, .98f);
}

fn Rectangle character_get_bounds(Character *character) {
  return (Rectangle){.x = character->position.x,
                     .y = character->position.y,
                     .width = character->size.x,
                     .height = character->size.y};
}

fn void character_draw(Character *character) {
  Texture2D texture = animation_step(&character->animation);
  Rectangle texture_rect = {
      .x = 0,
      .y = 0,
      .width = texture.width,
      .height = texture.height,
  };
  Rectangle character_rect =
      (Rectangle){character->position.x, character->position.y,
                  character->size.x, character->size.y};
  DrawTexturePro(texture, texture_rect, character_rect, Vector2Zero(), 0.0,
                 WHITE);

  if (IsKeyDown(KEY_W) && character->velocity.y == 0) {
    character->velocity.y -= 10;
  }
  if (IsKeyDown(KEY_A)) {
    character->velocity.x -= 0.15;
  } else if (IsKeyDown(KEY_D)) {
    character->velocity.x += 0.15;
  }
}

int main(void) {
  srand(GetTime());

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Break");
  InitAudioDevice();
  SetTargetFPS(60);

  const Texture2D textures[] = {
      LoadTexture("assets/grass.jpg"),
      LoadTexture("assets/dirt.jpg"),
      LoadTexture("assets/stone.jpg"),
  };

  const Sound sounds[] = {
      LoadSound("assets/crunch.wav"),
      LoadSound("assets/place.wav"),
  };

  int selected_block_type = BLOCK_TYPE_STONE;
  double ui_action_last_time = 0.0f;
  Camera2D camera = {0};
  camera.rotation = 0.0;
  camera.target = (Vector2){0, 0};
  camera.zoom = 1.0;

  Chunk chunks[24];

  char *filename = nullptr;

  Character character = {
    .size = {
      (float)BLOCK_SIZE_X,
      BLOCK_SIZE_Y,
    },
    .animation = load_animation("assets/character_animation")
  };

  Vector2 world_size = {
      BLOCK_SIZE_X * GRID_X * N_CHUNKS,
      BLOCK_SIZE_Y * GRID_Y,
  };

reset_world:
  character.position = (Vector2){0,0};
  if (filename) {
    free(filename);
  }
  bool result = select_filename(&filename);

  if (result) {
    for (int i = 0; i < 24; ++i) {
      float x_offset = i * (BLOCK_SIZE_X * GRID_X);
      chunk_generate(&chunks[i], x_offset);
    }
    save_new_world(&camera, chunks, &filename);
  } else {
    if (filename && FileExists(filename)) {
      read_world_from_file(&camera, chunks, filename);
    } else {
      for (int i = 0; i < 24; ++i) {
        float x_offset = i * (BLOCK_SIZE_X * GRID_X);
        chunk_generate(&chunks[i], x_offset);
      }
    }
  }

  while (!WindowShouldClose()) {
    BeginDrawing();
    BeginMode2D(camera);
    ClearBackground(SKYBLUE);

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
      int x = 0;
    }

    // Update game.
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    character_physics(&character, chunks, world_size);
    character_draw(&character);
    for (int i = 0; i < N_CHUNKS; ++i) {
      Chunk *chunk = &chunks[i];
      chunk_draw(chunk, textures, selected_block_type, mouse, sounds);
    }
    camera.target = character.position;
    camera.offset = (Vector2){GetScreenWidth() / 2.0, GetScreenHeight() / 2.0};

    { // Input stuff.

      if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
        EndMode2D();
        EndDrawing();

        while (true) {
          BeginDrawing();
          ClearBackground(BLACK);
          if (WindowShouldClose()) {
            CloseWindow();
            return 0;
          }
          DrawText("are you sure you want to reset the world?\npress [y/n] to "
                   "confirm/cancel",
                   250, 350, 25, RED);
          if (IsKeyPressed(KEY_Y)) {
            goto reset_world;
          } else if (IsKeyPressed(KEY_N)) {
            break;
          }
          EndDrawing();
        }
        continue;
      }

      if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        EndMode2D();
        EndDrawing();
        save_new_world(&camera, chunks, &filename);
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
    }

    { // UI
      double delta = GetTime() - ui_action_last_time;

      const float ELEMENT_SIZE = BLOCK_SIZE_Y;

      Vector2 ui_start_position = GetScreenToWorld2D(
          (Vector2){
              (float)SCREEN_WIDTH / 2 - (2 * ELEMENT_SIZE),
              SCREEN_HEIGHT - ELEMENT_SIZE,
          },
          camera);

      if (delta < 1.0) {
        for (int i = 0; i < 3; ++i) {
          Texture2D texture = textures[i];
          Rectangle texture_rect = {
              .x = 0,
              .y = 0,
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
    }

    EndMode2D();
    EndDrawing();
  }

  if (filename) {
    char buffer[1024];
    snprintf(buffer, 1024, "worlds/%s", filename);
    write_world_to_file(&camera, chunks, buffer);
  }

  return 0;
}