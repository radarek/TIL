#include <stdio.h>
#include <sys/time.h>

#define GETTIMEOFDAY_SYSCALL_NR 96

int main() {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  printf("Unix timestamp: %ld\n", tv.tv_sec);

  return 0;
}
