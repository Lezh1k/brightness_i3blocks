#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/inotify.h>

#define INOTIFY_EVENT_SIZE (sizeof (struct inotify_event))
#define INOTIFY_BUFF_SIZE 64

static bool get_float_value_from_file(const char *path,
                                      float *dst_val);

static void print_brightness_percent(const char *actual_path,
                                     const char *max_path);

static void usage(void) {
  fprintf(stderr, "brightness %lf - read actual brightness value in non-blocking style.\n\n", VERSION);
  fprintf(stderr,
          "Usage: brightness [options]\n\
          \n\
          Options:\n\
          -a, --actual_brightness_path  \tlist devices with available brightness controls.\n\
          -m, --max_brightness_path     \tsuppress output.\n\
          -h, --help                    \tprint this help.\n\
          -V, --version                 \tprint version and exit.\n\
          \n");
}
//////////////////////////////////////////////////////////////

int
main(int argc, char *argv[]) {
  char *actual_brightness_path = "/sys/class/backlight/intel_backlight/actual_brightness";
  char *max_brightness_path = "/sys/class/backlight/intel_backlight/max_brightness";

  char buff[INOTIFY_BUFF_SIZE] = {0};
  fd_set read_descriptors;
  struct timeval time_to_wait;
  int  ifd, wd, read_len, rc;
  // ifd - inotify_file_descriptor
  // wd - inotify wait descriptor
  // rc - result code

  const struct option lopts[] = {
  {"actual_brightness_path", required_argument,  NULL, 'a'},
  {"max_brightness_path", required_argument,  NULL, 'm'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {NULL, 0, NULL, 0},
};

  size_t opt_len;
  while (getopt_long(argc, argv, "a:m:hV", lopts, &rc) != -1) {
    switch (rc) {
      case 'a':
        opt_len = strlen(optarg);
        actual_brightness_path = malloc(opt_len + 1);
        strncpy(actual_brightness_path, optarg, opt_len);
        break;
      case 'm':
        opt_len = strlen(optarg);
        max_brightness_path = malloc(opt_len + 1);
        strncpy(max_brightness_path, optarg, opt_len);
        break;
      case 'h':
        usage();
        return 0;
      case 'V':
        printf("%lf\n", VERSION);
        return 0;
      default:
        printf("something bad is happened. option index is out of bounds (%d)\n", rc);
        break;
    }
  }

  print_brightness_percent(actual_brightness_path,
                           max_brightness_path);

  ifd = inotify_init();
  if (ifd == -1) {
    perror("inotify init failed");
    return -1;
  }

  wd = inotify_add_watch(ifd, actual_brightness_path , IN_MODIFY);
  FD_ZERO ( &read_descriptors );
  FD_SET (ifd, &read_descriptors);
  time_to_wait.tv_sec = 10;
  time_to_wait.tv_usec = 0;

  while (1) {
    rc = select(ifd+1, &read_descriptors, NULL, NULL, &time_to_wait);
    if (rc < 0) {
      perror("select failed");
      break;
    }

    if (rc == 0) { //timeout
      continue;
    }

    if (!FD_ISSET(ifd, &read_descriptors)) {
      continue;
    }

    read_len = read(ifd, (void*)buff, INOTIFY_BUFF_SIZE);
    if (read_len < 0) {
      perror("read failed");
      continue;
    }

    for (int eix = 0; eix < read_len; ) {
      struct inotify_event *event = (struct inotify_event*) &buff[eix];
      eix += INOTIFY_EVENT_SIZE + event->len;
      if (!(event->mask & IN_MODIFY))
        continue;

      print_brightness_percent(actual_brightness_path,
                               max_brightness_path);
    }
  }

  //actually when we receive some signal (like sigterm or something like that)
  //we will be here.

  inotify_rm_watch(ifd, wd);
  close(ifd);

  // cause we used strdup
  free(actual_brightness_path);
  free(max_brightness_path);
  return 0;
}
//////////////////////////////////////////////////////////////

bool
get_float_value_from_file(const char *path,
                          float *dst_val) {
#define BUFF_SIZE 16
  char buff[BUFF_SIZE] = {0};
  FILE *f = fopen(path, "r");
  bool result = false;
  if (f == NULL) {
    perror("failed to open file");
    return false;
  }

  do {
    if (!(fread((void*) buff, 1, BUFF_SIZE, f))) {
      perror("failed to read file");
      break;
    }

    char *unused;
    *dst_val = strtof(buff, &unused);
    result = true;
  } while (0);

  fclose(f);
  return result;
}
//////////////////////////////////////////////////////////////

void
print_brightness_percent(const char *actual_path,
                         const char *max_path) {
  float curr, max;
  const char *parr[] = {actual_path, max_path};
  float *farr[] = {&curr, &max};

  for (size_t i = 0; i < 2; ++i) {
    bool success = get_float_value_from_file(parr[i], farr[i]);\
    if (!success) {
      printf("\xF0\x9F\x94\x86"); //brightness symbol 🔆
      printf(": NA\n");
      fflush(stdout); // because we use select(), ant it blocks stdout
      return;
    }
  }

  printf("\xF0\x9F\x94\x86"); //brightness symbol 🔆
  printf(": %02.f%%\n", (curr / max) * 100.f);
  fflush(stdout); // because we use select(), ant it blocks stdout
}
//////////////////////////////////////////////////////////////
