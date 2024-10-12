
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

void *recvTask(void *);

size_t decode_screen_data_size(float screen_data_size);

int get_fps_from_argv(int argc, char *argv[], int const defaultVal);
char const *get_port_from_argv(int argc, char *argv[],
                               char const *const defaultVal);

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

  SetTargetFPS(fps);

  float screen_data_size_slider = MAX_SCREEN_DATA_SIZE;
  uint8_t internalBuffer[MAX_SCREEN_DATA_SIZE];
  memset(internalBuffer, 0, sizeof(internalBuffer));
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    size_t screen_data_size = decode_screen_data_size(screen_data_size_slider);
    float delta = screenWidth / ((float)screen_data_size / sizeof(float));
    BufferError err =
        IOBuffer_nextAsync(data, internalBuffer, screen_data_size);
    //   Draw
    BeginDrawing();

    ClearBackground(RAYWHITE);

    GuiSpinner((Rectangle){640, 40, 105, 20}, "yMax ", &yMax, 1, 10, false);
    GuiSpinner((Rectangle){640, 70, 105, 20}, "yMin ", &yMin, -10, -1, false);
    GuiSliderBar((Rectangle){640, 100, 105, 20}, "Boh ", NULL,
                 &screen_data_size_slider, MAX_SCREEN_DATA_SIZE / 2.0f,
                 MAX_SCREEN_DATA_SIZE);

    for (size_t i = 0; i < screen_data_size; i += sizeof(float)) {
      float d = *(float *)(internalBuffer + i);
      float xPos = i * delta / sizeof(d);
      float yPos = (d - yMin) / (yMax - yMin) * screenHeight;
      yPos = screenHeight - yPos;
      DrawCircle(xPos, yPos, 2, DARKBLUE);
    }

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
  if (getaddrinfo(NULL, port, &addrHints, &addrInfo) != 0) {
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
  printf("[UDP] - waiting for data on port %s\n", port);
  size_t const internalBufferSize = 1024 * sizeof(float);
  uint8_t internalBuffer[internalBufferSize];
  while (true) {
    size_t read = recv(s, &internalBuffer, sizeof(internalBuffer), 0);
    // printf("[UDP] - recv %d\n", ++c);
    assert(read % sizeof(float) == 0 && "receive failed");
    for (size_t i = 0; i < read; i += sizeof(float)) {
      *(uint32_t *)(internalBuffer + i) =
          ntohl(*(uint32_t *)(internalBuffer + i));
    }
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
