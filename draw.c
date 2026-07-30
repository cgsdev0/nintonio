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

#define WIDTH  600
#define HEIGHT 800

struct ioctl_command {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned char buf[HEIGHT * WIDTH];
};

int main() {
    // Simple test: fill with a mid-gray checkerboard
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fb[y * WIDTH + x] = ((x / 20 + y / 20) % 2) ? 0x00 : 0xFF;
        }
    }

    struct ioctl_command *cmd = malloc(sizeof(*cmd));

    cmd->x = 0;
    cmd->y = 0;
    cmd->width = 600;
    cmd->height = 800;
    memset(cmd->buf, 0xff, sizeof(cmd->buf));

    if (ioctl(fd, FBIO_UPDATE, cmd) < 0) perror("update ioctl");
    if (ioctl(fd, FBIO_WAIT1, 0) < 0) perror("wait1 ioctl");
    if (ioctl(fd, FBIO_WAIT2, 0) < 0) perror("wait2 ioctl");

    free(cmd);
    close(fd);
    return 0;
}
