#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void write_world_to_file(Camera2D *camera, Chunk *chunks, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    printf("failed to open file %s\n", filename);
    exit(1);
  }
  fprintf(file, "Camera %f ", camera->offset.x);
  for (int i = 0; i < N_CHUNKS; ++i) {
    Chunk *chunk = &chunks[i];
    fprintf(file, "Chunk { %f, %f, %f, %f } = {\n", chunk->bounds.x, chunk->bounds.y, chunk->bounds.width, chunk->bounds.height);
    for (int y = 0; y < GRID_Y; ++y) {
      for (int x = 0; x < GRID_X; ++x) {
        fprintf(file, "%d, ", chunk->blocks[y][x]);
      }
    }
    fprintf(file, "\n}\n");
  }
  fclose(file);
}


void read_world_from_file(Camera2D *camera, Chunk *chunks, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("failed to open file %s\n", filename);
    exit(1);
  }
  fscanf(file, "Camera %f ", &camera->offset.x);
  for (int i = 0; i < N_CHUNKS; ++i) {
    Chunk *chunk = &chunks[i];
    fscanf(file, "Chunk { %f, %f, %f, %f } = {\n", &chunk->bounds.x, &chunk->bounds.y, &chunk->bounds.width, &chunk->bounds.height);
    for (int y = 0; y < GRID_Y; ++y) {
      for (int x = 0; x < GRID_X; ++x) {
        fscanf(file, "%d, ", &chunk->blocks[y][x]);
      }
    }
    fscanf(file, "\n}\n");
  }
  fclose(file);
}


#endif
