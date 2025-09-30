#include "stb/stb_image.h"
#include <pthread.h>
#include <stdio.h> // for fprintf

typedef struct {
  char imgpath[256];
} ImageLoadArgs;

unsigned char *loaded_img = NULL;
int loaded_imgw, loaded_imgh, loaded_channels;
pthread_mutex_t img_mutex = PTHREAD_MUTEX_INITIALIZER;

void *load_image_thread(void *args) {
  ImageLoadArgs *load_args = (ImageLoadArgs *)args;

  unsigned char *img = stbi_load(load_args->imgpath, &loaded_imgw, &loaded_imgh,
                                 &loaded_channels, 4);
  if (!img) {
    fprintf(stderr, "Failed to load image in thread: %s\n",
            stbi_failure_reason());
    free(load_args);
    return NULL;
  }

  pthread_mutex_lock(&img_mutex);
  loaded_img = img;
  pthread_mutex_unlock(&img_mutex);

  free(load_args);
  return NULL;
}
