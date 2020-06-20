#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define GETTIMEOFDAY_SYSCALL_NR 96

int main() {
  struct timeval tv;

  syscall(GETTIMEOFDAY_SYSCALL_NR, &tv, NULL);

  printf("Unix timestamp: %ld\n", tv.tv_sec);

  return 0;
}
