/* gpsp_kobo.c -- minimal libretro frontend driving gpSP directly.
 * No dlopen: we link libgpsp.a and call retro_* as ordinary functions. */

#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include "libretro.h"

static int kbd_fd = -1;
extern uint16_t keystate;
uint16_t keystate;

static char sysdir[] = ".";

static int fbFd;

static time_t dirty_at = 0;
extern uint32_t backup_dirty;
static bool   pending  = false;
static bool   first_save  = false;

typedef char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define RESX 240
#define RESY 160

typedef struct FBIOUpdate {
		u32 x, y, w, h;
		u32 mode_a, mode_b;
		u8  data[RESX * RESY * 9];
} FBIOUpdate;

FBIOUpdate fb = {};

int amode = 2;
int bmode = 2;

#define FRAME_POLL 15
static int frame_idx = 0;

static uint8_t shadow[0x20000];
static bool sram_changed(void) {
   void *mem = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
   if (!memcmp(shadow, mem, 0x20000)) return false;
   memcpy(shadow, mem, 0x20000);
   return true;
}

/* Fill in from the dump above. Left column = kernel KEY_* code. */
static const struct { uint16_t code; uint8_t id; } keymap[] = {
   { KEY_RIGHT,  RETRO_DEVICE_ID_JOYPAD_UP    },
   { KEY_F8,   RETRO_DEVICE_ID_JOYPAD_RIGHT },
   { KEY_F7,   RETRO_DEVICE_ID_JOYPAD_DOWN  },
   { KEY_F1,   RETRO_DEVICE_ID_JOYPAD_LEFT    },
   { KEY_ENTER,   RETRO_DEVICE_ID_JOYPAD_A       },
   { KEY_F4,   RETRO_DEVICE_ID_JOYPAD_B       },
   { KEY_F3,   RETRO_DEVICE_ID_JOYPAD_START   },
   { KEY_F2,   RETRO_DEVICE_ID_JOYPAD_SELECT  },
};

int input_open(const char *path)
{
   kbd_fd = open(path, O_RDONLY | O_NONBLOCK);
   if (kbd_fd < 0) return -1;
   // stop keys reaching the console/Nickel
   ioctl(kbd_fd, EVIOCGRAB, 1);
   return 0;
}

static void save_sram(const char *path)
{
   void  *mem  = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
   size_t size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
   char tmp[512];
   FILE *f;

   if (!mem || !size) return;

   if (!first_save) {
      first_save = true;
      return;
   }
   printf("we are saving\n");

   /* write-then-rename: a half-written .srm loses the save */
   snprintf(tmp, sizeof tmp, "%s.tmp", path);
   if (!(f = fopen(tmp, "wb"))) return;
   if (fwrite(mem, 1, size, f) == size) {
      fflush(f);
      fsync(fileno(f));
      fclose(f);
      if(rename(tmp, path) != 0) {
         printf("Rename failed!\n");
      }
   } else {
      printf("Write failed!\n");
      fclose(f);
      remove(tmp);
   }
}

static void load_sram(const char *path)   /* call AFTER retro_load_game */
{
   void  *mem  = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
   size_t size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
   FILE *f = fopen(path, "rb");
   if (!f) return;
   fread(mem, 1, size, f);
   fclose(f);
}

void input_close(void)
{
   if (kbd_fd >= 0) { ioctl(kbd_fd, EVIOCGRAB, 0); close(kbd_fd); kbd_fd = -1; }
}

static void fb_init() {
	fbFd = open("/dev/fb0", O_RDWR);
	if (fbFd < 0)
		printf("Failed to open /dev/fb0: %s", strerror(errno));
	{
		// clear the screen
		struct upd {
				u32 x, y, w, h;
				u32 mode_a, mode_b;
				u8  data[600 * 800];
		} *buf = malloc(sizeof(struct upd));
		memset(buf->data, 0xFF, 800*600);
		buf->x = 0; buf->y = 0; buf->w = 600; buf->h = 800;
		buf->mode_a = 3; buf->mode_b = 1;
		ioctl(fbFd, 0x4702, buf);
		ioctl(fbFd, 0x4528, 0);
		ioctl(fbFd, 0x4529, 0);
		free(buf);
	}
}

static void cb_video(const void *data, unsigned w, unsigned h, size_t pitch)
{
   const uint8_t *src = (const uint8_t *)data;
   unsigned y;
   if (!data) return;

   frame_idx += 1;
   if (frame_idx >= FRAME_POLL) {
      frame_idx = 0;
   } else {
      return;
   }

     for (int row = 0; row < RESY; row++) {
       for (int col = 0; col < RESX; col++) {
         u8 *start = ((u8*)data) + (((RESY - row - 1) * RESX + col) * 2);
         u16 color = *((u16*)start);
         u8 r5 = (color >> 11) & 0x1F;
         u8 g6 = (color >>  5) & 0x3F;
         u8 b5 =  color        & 0x1F;
         u8 r = (r5 << 3) | (r5 >> 2);
         u8 g = (g6 << 2) | (g6 >> 4);
         u8 b = (b5 << 3) | (b5 >> 2);
         float fr = (float)r;
         float fg = (float)g;
         float fblue = (float)b;
         float mix = 0.2126 * fr + 0.7152 * fg + 0.0722 * fblue;
         // float mix = max(fr, fg);
         // mix = max(mix, fblue);
         for (int x=0; x<3; x++) {
            fb.data[(col * 3 + x) * RESY * 3 + row * 3 + 0]=(u8)mix;
            fb.data[(col * 3 + x) * RESY * 3 + row * 3 + 1]=(u8)mix;
            fb.data[(col * 3 + x) * RESY * 3 + row * 3 + 2]=(u8)mix;
         }
       }
     }

     fb.x = 0; fb.y = 0; fb.w = RESY * 3; fb.h = RESX * 3;
     fb.mode_a = amode; fb.mode_b = bmode;
     ioctl(fbFd, 0x4539, &fb);
}

static void cb_audio_sample(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t cb_audio_batch(const int16_t *d, size_t frames) { (void)d; return frames; }

static void cb_input_poll(void)
{
   struct input_event ev;
   unsigned i;

   if (kbd_fd < 0) return;

   /* Drain everything queued; O_NONBLOCK makes the last read return -1/EAGAIN. */
   while (read(kbd_fd, &ev, sizeof ev) == (ssize_t)sizeof ev)
   {
      if (ev.type != EV_KEY || ev.value == 2)   /* 2 = autorepeat, ignore */
         continue;

      for (i = 0; i < sizeof keymap / sizeof keymap[0]; i++)
         if (keymap[i].code == ev.code)
         {
            if (ev.value)  keystate |=  (uint16_t)(1u << keymap[i].id);
            else           keystate &= (uint16_t)~(1u << keymap[i].id);
            break;
         }
   }
   /* read /dev/input/eventN here and update keystate */
}

static int16_t cb_input_state(unsigned port, unsigned dev, unsigned idx, unsigned id)
{
   (void)dev; (void)idx;
   if (port != 0) return 0;
   if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return keystate;
   return (keystate >> id) & 1;
}

static bool cb_environment(unsigned cmd, void *data)
{
   switch (cmd)
   {
   case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
      return *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;

   case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
      *(const char **)data = sysdir;
      return true;

   case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
      return true;

   default:
      return false;
   }
}

int main(int argc, char **argv)
{
   struct retro_game_info game;
   struct retro_system_av_info av;

   if (argc < 3) { fprintf(stderr, "usage: %s rom.gba save.srm\n", argv[0]); return 1; }

   printf("SRAM path: %s\n", argv[2]);

   fb_init();
   input_open("/dev/input/event0");

   retro_set_environment(cb_environment);
   retro_set_video_refresh(cb_video);
   retro_set_audio_sample(cb_audio_sample);
   retro_set_audio_sample_batch(cb_audio_batch);
   retro_set_input_poll(cb_input_poll);
   retro_set_input_state(cb_input_state);

   retro_init();

   memset(&game, 0, sizeof(game));
   game.path = argv[1];

   if (!retro_load_game(&game)) {
      fprintf(stderr, "retro_load_game failed\n");
      retro_deinit();
      return 1;
   }

   printf("loading SRAM from disk\n");
   load_sram(argv[2]);

   retro_get_system_av_info(&av);
   fprintf(stderr, "%ux%u @ %.2f fps\n",
           av.geometry.base_width, av.geometry.base_height, av.timing.fps);

   for (;;) {
      retro_run();
      if (backup_dirty) {
         backup_dirty = 0;
         dirty_at = time(NULL);
         pending  = true;
      }
      if (pending && time(NULL) - dirty_at >= 2) {
         if (sram_changed()) save_sram(argv[2]);
         pending = false;
      }
   }

   retro_unload_game();
   retro_deinit();
   input_close();
   return 0;
}
