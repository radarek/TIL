# Linux syscalls and vDSO

### Links

* https://blog.packagecloud.io/eng/2016/04/05/the-definitive-guide-to-linux-system-calls/
* https://blog.packagecloud.io/eng/2017/03/08/system-calls-are-much-slower-on-ec2/
* https://bert-hubert.blogspot.com/2017/03/on-linux-vdso-and-clockgettime.html
* https://berthub.eu/articles/posts/on-linux-vdso-and-clockgettime/
* https://unix.stackexchange.com/questions/553845/should-i-be-seeing-non-vdso-clock-gettime-syscalls-on-x86-64-using-hpet

### How to list all syscalls for current kernel?

```
$ sudo apt-get install auditd
$ ausyscall --dump
Using x86_64 syscall table:
0	read
1	write
2	open
3	close
4	stat
...
```

### vDSO

* vDSO stands for **virtual dynamic shared object**
* mechanism for exporting a set of kernel space routines to user space so that applications can call them in-process, without incurring the performance penalty of a mode switch from user to kernel mode
* `linux-vdso.so.1` is virtualy linked in every executable
  ```
  $ ldd `which bash`
    linux-vdso.so.1 (0x00007ffd2359f000)
    libtinfo.so.6 => /lib/x86_64-linux-gnu/libtinfo.so.6 (0x00007f8217fcb000)
    libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f8217fc5000)
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f8217dd3000)
    /lib64/ld-linux-x86-64.so.2 (0x00007f821812c000)
  ```

### vDSO in action

Run following program which calls `gettimeofday` syscall through `syscall` function:

`gettimeofday.c`
```c
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
```

To verify that this program indeed calls syscall use strace:

```
$ sudo strace -e gettimeofday ./a.out
gettimeofday({tv_sec=1592674358, tv_usec=86472}, NULL) = 0
Unix timestamp: 1592674358
+++ exited with 0 +++
```

Now do the same with slightly modified program which use `gettimeofday` function directly:

`gettimeofday2.c`
```c
#include <stdio.h>
#include <sys/time.h>

#define GETTIMEOFDAY_SYSCALL_NR 96

int main() {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  printf("Unix timestamp: %ld\n", tv.tv_sec);

  return 0;
}
```

Now verify with `strace` that no system call is made:

```
$ sudo strace -e gettimeofday ./a.out
Unix timestamp: 1592674509
+++ exited with 0 +++
```

This is because `gettimeofday` is called through vDSO which will not switch to kernel mode when calling it.

On some setups you can get system call for last example even if vDSO should kick-in.

For example running last example on docker (mac os) I get:

```
$ strace -e gettimeofday ./a.out
gettimeofday({tv_sec=1592675987, tv_usec=264353}, NULL) = 0
Unix timestamp: 1592675987
+++ exited with 0 +++
```

This is related to how timekeeping is handled in Linux.

```
# ubuntu running in virtual box
$ cat /sys/devices/system/clocksource/clocksource0/available_clocksource
kvm-clock tsc acpi_pm
$ cat /sys/devices/system/clocksource/clocksource0/current_clocksource
kvm-clock
```

```
# ubuntu running in docker container
$ cat /sys/devices/system/clocksource/clocksource0/available_clocksource
hpet acpi_pm
$ cat /sys/devices/system/clocksource/clocksource0/current_clocksource
hpet
```

### List of vDSO functions

```
$ gcc -o dump-vdso dump-vdso.c
$ ./dump-vdso > vdso.s
$ objdump -T vdso.so

vdso.so:     file format elf64-x86-64

DYNAMIC SYMBOL TABLE:
0000000000000a30  w   DF .text	0000000000000305  LINUX_2.6   clock_gettime
0000000000000d40 g    DF .text	00000000000001c2  LINUX_2.6   __vdso_gettimeofday
0000000000000d40  w   DF .text	00000000000001c2  LINUX_2.6   gettimeofday
0000000000000f10 g    DF .text	0000000000000015  LINUX_2.6   __vdso_time
0000000000000f10  w   DF .text	0000000000000015  LINUX_2.6   time
0000000000000a30 g    DF .text	0000000000000305  LINUX_2.6   __vdso_clock_gettime
0000000000000000 g    DO *ABS*	0000000000000000  LINUX_2.6   LINUX_2.6
0000000000000f30 g    DF .text	000000000000002a  LINUX_2.6   __vdso_getcpu
0000000000000f30  w   DF .text	000000000000002a  LINUX_2.6   getcpu
```
