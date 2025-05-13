#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define min(a, b) ((a > b) ? b : a)

extern int errno;

int tun_alloc(char *dev) {
  struct ifreq ifr;
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("failed create tun");
    _exit(-1);
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_ifru.ifru_flags = IFF_TUN;

  if (*dev)
    strncpy(ifr.ifr_ifrn.ifrn_name, dev, min(strlen(dev), IFNAMSIZ));

  if ((ioctl(fd, TUNSETIFF, (void *)&ifr)) == -1) {
    perror("failed ioctl");
    _exit(-1);
  }

  strcpy(dev, ifr.ifr_ifrn.ifrn_name);
  return fd;
}

void print_hex_array(const unsigned char *arr, size_t size) {
  for (size_t i = 0; i < size; i++) {
    printf("%02x ", arr[i]);
  }
  printf("\n");
}

int main() {
  int fd;
  char dev[IFNAMSIZ] = "tun114514";

  if ((fd = tun_alloc(dev)) <= 0)
    _exit(-1);

  int nsize;
  char buf[1024];

  do {
    nsize = read(fd, buf, sizeof(buf));
    printf("recv %d byte:\n", nsize);
    print_hex_array(buf, nsize);
  } while (1);

  return 0;
}
