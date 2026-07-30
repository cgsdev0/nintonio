#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#define FBIO_UPDATE   0x4702
#define FBIO_WAIT1    0x4528
#define FBIO_WAIT2    0x4529

#define WIDTH  600
#define HEIGHT 800

struct mxcfb_rect {
    unsigned int top;
    unsigned int left;
    unsigned int width;
    unsigned int height;
};

struct mxcfb_update_data {
    struct mxcfb_rect update_region;
    unsigned int waveform_mode;
    unsigned int update_mode;
    unsigned int update_marker;
    int temperature;
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

    struct mxcfb_update_data args = {
        .update_region = {
            .top = 0,
            .left = 0,
            .width = 600,
            .height = 800,
        },
        .waveform_mode = 3,
        .update_mode = 1,
        .update_marker = 0,
        .temperature = 25,
    };

    msync(fb, fb_size, MS_SYNC);

    if (ioctl(fd, FBIO_UPDATE, &args) < 0) perror("update ioctl");
    if (ioctl(fd, FBIO_WAIT1, 0) < 0) perror("wait1 ioctl");
    if (ioctl(fd, FBIO_WAIT2, 0) < 0) perror("wait2 ioctl");

    munmap(fb, fb_size);
    close(fd);
    return 0;
}
