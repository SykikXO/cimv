#include "async.h"
#include "stb/stb_image.h"
#include "xdg-shell-client-protocol.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct xdg_wm_base *sh;
struct xdg_toplevel *top;
struct wl_surface *surf;
struct wl_compositor *comp;
struct wl_buffer *bfr;
struct wl_shm *shm;
char *imgpath;

uint8_t *pixl;
int imgw, imgh; // dimensions of the loaded image

#include <pthread.h>
#include <stdio.h>
#include <string.h>

// Function running in a separate thread continuously asking for image paths
void *input_thread_func(void *arg) {
  char path[256];
  while (1) {
    printf("Enter image path: ");
    if (fgets(path, sizeof(path), stdin) == NULL) {
      fprintf(stderr, "Failed to read image path\n");
      continue;
    }

    // Strip newline from fgets
    path[strcspn(path, "\n")] = 0;

    if (strlen(path) == 0) {
      continue; // skip empty inputs
    }

    // Call your async loader start function for new input path
    start_async_image_load(path);
  }
  return NULL;
}

int32_t allocate_shm(uint64_t size) {
  static int counter = 0;
  char name[32];
  snprintf(name, sizeof(name), "/wayland_shm_%d_%d", getpid(), counter++);
  int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
  if (fd < 0) {
    perror("shm_open");
    exit(1);
  }
  shm_unlink(name); // remove name so it auto deletes when closed
  if (ftruncate(fd, size) < 0) {
    perror("ftruncate");
    close(fd);
    exit(1);
  }
  return fd;
}

void create_buffer(int w, int h) {
  int size = w * h * 4;
  int32_t fd = allocate_shm(size);
  pixl = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (pixl == MAP_FAILED) {
    perror("mmap");
    close(fd);
    exit(1);
  }
  struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
  bfr = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);
}

void start_async_image_load(const char *path) {
  pthread_t thread;
  ImageLoadArgs *args = malloc(sizeof(ImageLoadArgs));
  if (!args) {
    fprintf(stderr, "Failed to allocate memory for image load args.\n");
    return;
  }
  strncpy(args->imgpath, path, sizeof(args->imgpath) - 1);
  args->imgpath[sizeof(args->imgpath) - 1] = '\0';

  if (pthread_create(&thread, NULL, load_image_thread, args) != 0) {
    fprintf(stderr, "Failed to create image load thread.\n");
    free(args);
    return;
  }
  pthread_detach(thread);
}
void draw_img() {
  // const char *imgpath = "/home/sykik/.config/walls/catpuccin_samurai.png";
  char imgpath[256];
  printf("Enter image path: ");
  if (scanf("%255s", imgpath) != 1) {
    fprintf(stderr, "Failed to read image path\n");
    return;
  }
  int channels;
  unsigned char *img = stbi_load(imgpath, &imgw, &imgh, &channels, 4);
  if (!img) {
    fprintf(stderr, "Image failed to load: %s\n", imgpath);
    fprintf(stderr, "STB image failure reason: %s\n", stbi_failure_reason());
    return;
  }

  // allocate buffer for image size if not already done
  if (!pixl) {
    create_buffer(imgw, imgh);
  }

  // blit pixels with ARGB8888 conversion
  for (int y = 0; y < imgh; y++) {
    for (int x = 0; x < imgw; x++) {
      uint8_t *s = img + 4 * (y * imgw + x); // RGBA from stb_image
      uint8_t *d = pixl + 4 * (y * imgw + x);

      d[0] = s[2]; // B
      d[1] = s[1]; // G
      d[2] = s[0]; // R
      d[3] = s[3]; // A
    }
  }

  stbi_image_free(img);

  wl_surface_attach(surf, bfr, 0, 0);
  wl_surface_damage_buffer(surf, 0, 0, imgw, imgh);
  wl_surface_commit(surf);
}

struct wl_callback_listener cb_list;

void frame_new(void *data, struct wl_callback *cb, uint32_t time) {
  wl_callback_destroy(cb);
  cb = wl_surface_frame(surf);
  wl_callback_add_listener(cb, &cb_list, 0);

  pthread_mutex_lock(&img_mutex);
  if (loaded_img != NULL) {
    // if needed, create or resize buffer for loaded_imgw, loaded_imgh
    if (!pixl) {
      create_buffer(loaded_imgw, loaded_imgh);
    }

    // copy pixels in ARGB format
    for (int y = 0; y < loaded_imgh; y++) {
      for (int x = 0; x < loaded_imgw; x++) {
        uint8_t *s = loaded_img + 4 * (y * loaded_imgw + x);
        uint8_t *d = pixl + 4 * (y * loaded_imgw + x);

        d[0] = s[2]; // B
        d[1] = s[1]; // G
        d[2] = s[0]; // R
        d[3] = s[3]; // A
      }
    }

    wl_surface_attach(surf, bfr, 0, 0);
    wl_surface_damage_buffer(surf, 0, 0, loaded_imgw, loaded_imgh);
    wl_surface_commit(surf);

    stbi_image_free(loaded_img);
    loaded_img = NULL; // mark as drawn
  }
  pthread_mutex_unlock(&img_mutex);
}
struct wl_callback_listener cb_list = {.done = frame_new};

void xsurf_conf(void *data, struct xdg_surface *xsurf, uint32_t ser) {
  xdg_surface_ack_configure(xsurf, ser);
  if (!pixl) {
    draw_img();
  }
}

void sh_ping(void *data, struct xdg_wm_base *sh, uint32_t ser) {
  xdg_wm_base_pong(sh, ser);
}

struct xdg_wm_base_listener sh_list = {.ping = sh_ping};
struct xdg_surface_listener xsurf_list = {.configure = xsurf_conf};

void reg_glob(void *data, struct wl_registry *reg, uint32_t name,
              const char *intf, uint32_t v) {
  if (!strcmp(intf, wl_compositor_interface.name)) {
    comp = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
  } else if (!strcmp(intf, wl_shm_interface.name)) {
    shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
  } else if (!strcmp(intf, xdg_wm_base_interface.name)) {
    sh = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(sh, &sh_list, 0);
  }
}

void top_conf(void *data, struct xdg_toplevel *top, int32_t w, int32_t h,
              struct wl_array *s) {}
void top_cls(void *data, struct xdg_toplevel *top) {}

struct xdg_toplevel_listener top_list = {.configure = top_conf,
                                         .close = top_cls};

void reg_glob_rem(void *data, struct wl_registry *reg, uint32_t name) {}

struct wl_registry_listener reg_list = {.global = reg_glob,
                                        .global_remove = reg_glob_rem};

int main() {
  struct wl_display *disp = wl_display_connect(0);
  if (!disp) {
    fprintf(stderr, "Failed to connect to Wayland display.\n");
    return 1;
  }

  struct wl_registry *reg = wl_display_get_registry(disp);
  wl_registry_add_listener(reg, &reg_list, 0);
  wl_display_roundtrip(disp);

  surf = wl_compositor_create_surface(comp);
  struct wl_callback *cb = wl_surface_frame(surf);
  wl_callback_add_listener(cb, &cb_list, 0);

  struct xdg_surface *xsurf = xdg_wm_base_get_xdg_surface(sh, surf);
  xdg_surface_add_listener(xsurf, &xsurf_list, 0);

  top = xdg_surface_get_toplevel(xsurf);
  xdg_toplevel_add_listener(top, &top_list, 0);
  xdg_toplevel_set_title(top, "Wayland Image Viewer");
  wl_surface_commit(surf);
  pthread_t input_thread;
  if (pthread_create(&input_thread, NULL, input_thread_func, NULL) != 0) {
    fprintf(stderr, "Failed to create input thread\n");
    return 1;
  }

  while (wl_display_dispatch(disp)) {
  }

  if (bfr) {
    wl_buffer_destroy(bfr);
  }
  if (top)
    xdg_toplevel_destroy(top);
  xdg_surface_destroy(xsurf);
  wl_surface_destroy(surf);
  wl_display_disconnect(disp);
  return 0;
}
