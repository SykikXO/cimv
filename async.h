
#ifndef ASYNC_H
#define ASYNC_H

#include <pthread.h>

// Structure to pass image path to thread
typedef struct {
  char imgpath[256];
} ImageLoadArgs;

// Shared variables accessed across files
extern unsigned char *loaded_img;
extern int loaded_imgw, loaded_imgh, loaded_channels;
extern pthread_mutex_t img_mutex;

// Thread function to load image asynchronously
void *load_image_thread(void *args);

// Function to start the async loading thread
void start_async_image_load(const char *path);

#endif // ASYNC_H
