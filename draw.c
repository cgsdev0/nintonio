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

typedef char u8;
typedef unsigned int u32;
int main() {
    int fb = open("/dev/fb0", O_RDWR);

struct upd {                              // 24-byte header, data follows inline
    u32 x, y, w, h;
    u32 mode_a, mode_b;
    u8  data[600*800];
} *buf = malloc(480024);                  // 0x75318 = 24 + 480000
memset(buf->data, 0, 480000);             // 0x75300

buf->x = 0; buf->y = 0; buf->w = 600; buf->h = 800;
buf->mode_a = 3; buf->mode_b = 1;
ioctl(fb, 0x4702, buf);                   // blank screen w/ the zeroed payload
ioctl(fb, 0x4528, 0);
ioctl(fb, 0x4529, 0);

for (int n = 0; n != 480000; ) {              // rsb #479232 + add #768 == 480000 - n
    int r = read(0, buf->data + n, 480000 - n);   // fd 0 == stdin
    n += r;
    if (r <= 0) return 0;                 // short input: bail, exit code 0
}

buf->x = 0; buf->y = 0; buf->w = 600; buf->h = 800;
buf->mode_a = 2; buf->mode_b = 2;
ioctl(fb, 0x4539, buf);                   // the actual draw
ioctl(fb, 0x4528, 0); ioctl(fb, 0x4529, 0);
ioctl(fb, 0x4528, 0); ioctl(fb, 0x4529, 0);

free(buf); sleep(1); close(fb);
return 1;
}
