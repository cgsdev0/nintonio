#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#define FBIO_UPDATE   0x4539
#define FBIO_WAIT1    0x4528
#define FBIO_WAIT2    0x4529

#define FB_W   600
#define FB_H   800
#define IMG_W  360
#define IMG_H  480
#define BMPROW ((IMG_W * 3 + 3) & ~3)

typedef char u8;
typedef unsigned int u32;

static int read_full(int fd, void *p, size_t n) {
    unsigned char *q = p;
    while (n) {
        ssize_t r = read(fd, q, n);
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        if (r == 0) return -1;                 /* truncated file */
        q += r; n -= r;
    }
    return 0;
}


int main() {
    int fb = open("/dev/fb0", O_RDWR);

		for (int bmp_idx = 1; bmp_idx <= 659; bmp_idx++) {
		char fname[30];
		sprintf(fname, "%04d.bmp", bmp_idx);
		printf("reading file: %s\n", fname);
		int bmp = open(fname, O_RDONLY);
		lseek(bmp, 138, SEEK_SET);
		struct upd {                              // 24-byte header, data follows inline
				u32 x, y, w, h;
				u32 mode_a, mode_b;
				u8  data[FB_W*FB_H];
		} *buf = malloc(sizeof(struct upd));                  // 0x75318 = 24 + 480000
		memset(buf->data, 0, FB_W*FB_H);             // 0x75300

		buf->x = 0; buf->y = 0; buf->w = FB_W; buf->h = FB_H;
		buf->mode_a = 3; buf->mode_b = 1;
		ioctl(fb, 0x4702, buf);                   // blank screen w/ the zeroed payload
		ioctl(fb, 0x4528, 0);
		ioctl(fb, 0x4529, 0);

		unsigned char row[BMPROW];
		const int x0 = (FB_W - IMG_W) / 2;             /* 120 */
		const int y0 = (FB_H - IMG_H) / 2;             /* 160 */

		for (int i = 0; i < IMG_H; i++) {
				if (read_full(bmp, row, BMPROW) < 0) {
				    return -1;
				}

				int y = y0 + (IMG_H - 1 - i);              /* BMP rows are bottom-up */
				unsigned char *dst = buf->data + y * FB_W + x0;

				for (int x = 0; x < IMG_W; x++)
						dst[x] = row[x * 3 + 2];               /* +2 = red */
		}

		buf->x = 0; buf->y = 0; buf->w = FB_W; buf->h = FB_H;
		buf->mode_a = 2; buf->mode_b = 2;
		ioctl(fb, 0x4539, buf);                   // the actual draw
		ioctl(fb, 0x4528, 0); ioctl(fb, 0x4529, 0);
		ioctl(fb, 0x4528, 0); ioctl(fb, 0x4529, 0);

		free(buf);
		close(bmp);
		sleep(1);
		}
		close(fb);
		return 0;
}
