#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#define FBIO_UPDATE   0x4539
#define FBIO_WAIT1    0x4528
#define FBIO_WAIT2    0x4529

#define WIDTH  800
#define HEIGHT 600

struct update_args {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
    unsigned int mode; // try 2 or 3 for full refresh
};

int main() {
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    // Map the framebuffer memory and fill it with a test pattern
    size_t fb_size = WIDTH * HEIGHT; // 1 byte per pixel, adjust if wrong
    unsigned char *fb = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // Simple test: fill with a mid-gray checkerboard
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
                fb[y * WIDTH + x] = 0xFF;
        }
    }

    struct update_args args = {0, 0, WIDTH, HEIGHT, 2};

    if (ioctl(fd, FBIO_UPDATE, &args) < 0) perror("update ioctl");
    if (ioctl(fd, FBIO_WAIT1, 0) < 0) perror("wait1 ioctl");
    if (ioctl(fd, FBIO_WAIT2, 0) < 0) perror("wait2 ioctl");

    munmap(fb, fb_size);
    close(fd);
    return 0;
}
