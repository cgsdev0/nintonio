// write_test.c — no stdio at all, raw syscall
#include <unistd.h>
int main(void) {
    write(1, "hello via write\n", 16);
    return 0;
}
