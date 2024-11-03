
#include "buffer/include/buffer/io_buffer.h"
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define DATA_SIZE (4096 * sizeof(float))
IOBuffer *data;

#define MAX_SCREEN_DATA_SIZE (128 * sizeof(float))

#define DEFAULT_FPS 1000L
#define DEFAULT_PORT "6969"

typedef void (*renderDataFunc_t)(void const *const data,
                                 size_t const dataLenght, float const deltaX,
                                 int const screenWidth, int const screenHeight,
                                 int const yMin, int const yMax);

void *recvTask(void *);

size_t decode_screen_data_size(float screen_data_size);

int get_fps_from_argv(int argc, char *argv[], int const defaultVal);
char const *get_port_from_argv(int argc, char *argv[],
                               char const *const defaultVal);

void renderByPoints(float const *const data, size_t const dataLenght,
                    float const deltaX, int const screenWidth,
                    int const screenHeight, int const yMin, int const yMax);

void renderByLines(float const *const data, size_t const dataLenght,
                   float const deltaX, int const screenWidth,
                   int const screenHeight, int const yMin, int const yMax);

void renderByLinesComplex(float const *const data, size_t const dataLenght,
                          float const deltaX, int const screenWidth,
                          int const screenHeight, int const yMin,
                          int const yMax);

int main(int argc, char *argv[]) {
  int const fps = get_fps_from_argv(argc, argv, DEFAULT_FPS);
  char const *port = get_port_from_argv(argc, argv, DEFAULT_PORT);
  data = IOBuffer_create(DATA_SIZE);
  assert(data);

  pthread_t recvThread;
  pthread_create(&recvThread, NULL, recvTask, (void *)port);

  SetTraceLogLevel(LOG_WARNING);
  const int screenWidth = 800;
  const int screenHeight = 450;
  InitWindow(screenWidth, screenHeight, "Data Visualizer");

  int yMax = 1;
  int yMin = -1;
  // renderDataFunc_t renderFunc = (renderDataFunc_t)renderByPoints;
  renderDataFunc_t renderFunc = (renderDataFunc_t)renderByLines;
  // renderDataFunc_t renderFunc = (renderDataFunc_t)renderByLinesComplex;

  SetTargetFPS(fps);

  float samplesPerWindow = MAX_SCREEN_DATA_SIZE / (2 * (float)sizeof(float));
  uint8_t internalBuffer[MAX_SCREEN_DATA_SIZE];
  memset(internalBuffer, 0, sizeof(internalBuffer));
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    size_t screen_data_size = (size_t)samplesPerWindow * sizeof(float);
    float delta = screenWidth / ((float)screen_data_size / sizeof(float));
    BufferError err =
        IOBuffer_nextAsync(data, internalBuffer, screen_data_size);
    //   Draw
    BeginDrawing();

    ClearBackground(RAYWHITE);

    GuiSpinner((Rectangle){680, 40, 105, 20}, "yMax ", &yMax, 1, 10, false);
    GuiSpinner((Rectangle){680, 70, 105, 20}, "yMin ", &yMin, -10, -1, false);
    GuiSlider((Rectangle){110, 40, 105, 20}, "SamplesPerWindow", NULL,
              &samplesPerWindow, 1,
              MAX_SCREEN_DATA_SIZE / (float)sizeof(float));
    int samplesPerWindowGuiValue = (int)samplesPerWindow;
    GuiValueBox((Rectangle){220, 40, 50, 20}, NULL, &samplesPerWindowGuiValue,
                1, MAX_SCREEN_DATA_SIZE / sizeof(float), false);

    renderFunc(internalBuffer, screen_data_size / sizeof(float), delta,
               screenWidth, screenHeight, yMin, yMax);

    EndDrawing();
  }

  CloseWindow(); // Close window and OpenGL context

  return 0;
}

void *recvTask(void *args) {
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  char const *const port = (char *)args;
  assert(s && "socket fail");
  struct addrinfo addrHints, *addrInfo;
  memset(&addrHints, 0, sizeof(addrHints));
  addrHints.ai_family = AF_INET;
  addrHints.ai_protocol = IPPROTO_UDP;
  if (getaddrinfo("0.0.0.0", port, &addrHints, &addrInfo) != 0) {
    assert(0 && "getaddrinfo failed");
  };
  struct addrinfo *addr;
  for (addr = addrInfo; addr != NULL; addr = addr->ai_next) {
    if (!addr) {
      continue;
    }
    if (bind(s, addr->ai_addr, addr->ai_addrlen) == 0) {
      break;
    }
  }
  freeaddrinfo(addrInfo);
  assert(addr && "bind failed");
  addr = NULL;
  printf("[UDP] - waiting for data on port %s\n", port);
  size_t const internalBufferSize = 32 * sizeof(float);
  uint8_t internalBuffer[internalBufferSize];
  while (true) {
    size_t read = recv(s, &internalBuffer, sizeof(internalBuffer), 0);
    // printf("[UDP] - recv %lu bytes\n", read);
    assert(read % sizeof(float) == 0 && "receive failed");
    //    for (size_t i = 0; i < read; i += sizeof(float)) {
    //      *(uint32_t *)(internalBuffer + i) =
    //          ntohl(*(uint32_t *)(internalBuffer + i));
    //    }
    BufferError err = IOBuffer_write(data, internalBuffer, read);
    // if (err.errorCode == BUFFER_ERROR_OK) {
    //   printf("recv %lu bytes\n", err.result);
    // } else {
    //   printf("ERROR %d\n", err.errorCode);
    // }
  }
  return NULL;
}

size_t decode_screen_data_size(float screen_data_size) {
  return ((int)screen_data_size >> 2) * 4;
}

int get_fps_from_argv(int argc, char *argv[], int const defaultVal) {
  int fps = defaultVal;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--fps") != 0) {
      continue;
    }
    if (i + 1 >= argc) {
      continue;
    }
    errno = 0;
    fps = strtol(argv[i + 1], NULL, 10);
    if (errno != 0) {
      fps = defaultVal;
    }
    break;
  }
  return fps;
}

char const *get_port_from_argv(int argc, char *argv[],
                               char const *const defaultVal) {
  char const *port = defaultVal;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") != 0) {
      continue;
    }
    if (i + 1 >= argc) {
      continue;
    }
    port = argv[i + 1];
    break;
  }
  return port;
}

void renderByPoints(float const *const data, size_t const dataLenght,
                    float const deltaX, int const screenWidth,
                    int const screenHeight, int const yMin, int const yMax) {
  for (size_t i = 0; i < dataLenght; i++) {
    float d = data[i];
    float xPos = i * deltaX;
    float yPos = (d - yMin) / (yMax - yMin) * screenHeight;
    yPos = screenHeight - yPos;
    DrawCircle(xPos, yPos, 2, DARKBLUE);
  }
  return;
}

void renderByLines(float const *const data, size_t const dataLenght,
                   float const deltaX, int const screenWidth,
                   int const screenHeight, int const yMin, int const yMax) {
  Vector2 points[dataLenght];
  for (size_t i = 0; i < dataLenght; i++) {
    float d = data[i];
    points[i] = (Vector2){.x = i * deltaX,
                          .y = screenHeight * (1 - (d - yMin) / (yMax - yMin))};
  }
  DrawLineStrip(points, dataLenght, BLACK);
  return;
}

void renderByLinesComplex(float const *const data, size_t const dataLenght,
                          float const deltaX, int const screenWidth,
                          int const screenHeight, int const yMin,
                          int const yMax) {
  Vector2 pointsReal[dataLenght / 2];
  Vector2 pointsImag[dataLenght / 2];
  for (size_t i = 0; i < dataLenght / 2; i++) {
    float dReal = data[2 * i];
    float dImag = data[2 * i + 1];
    pointsReal[i] =
        (Vector2){.x = 2 * i * deltaX,
                  .y = screenHeight * (1 - (dReal - yMin) / (yMax - yMin))};
    pointsImag[i] =
        (Vector2){.x = (2 * i + 1) * deltaX,
                  .y = screenHeight * (1 - (dImag - yMin) / (yMax - yMin))};
  }
  DrawLineStrip(pointsReal, dataLenght / 2, RED);
  DrawLineStrip(pointsImag, dataLenght / 2, BLUE);
  return;
}
