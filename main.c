#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "Missing file\n");
    return 1;
  }

  FILE *fptr;
  if ((fptr = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "Failed opening file.\n");
    return 1;
  }

  int fd;
  if ((fd = fileno(fptr)) == -1) {
    fprintf(stderr, "Failed to get file descriptor");
    return 1;
  }

  struct stat filestat;

  if (fstat(fd, &filestat) == -1) {
    fprintf(stderr, "Failed to get filestat\n");
    return 1;
  }

  // printf("filesize: %.3fMB\n", get_filesize_MB(filestat));

  void *mappedregion =
      mmap(NULL, filestat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (mappedregion == MAP_FAILED) {
    fprintf(stderr, "Failed mapping region\n");
    return 1;
  }

  unsigned char *regionvals = (unsigned char *)mappedregion;

  int num_pics = 0;
  // off_t offsets[1000];

  for (off_t i = 0; i < filestat.st_size - 3; i++) {
    if (regionvals[i] == 0xFF && regionvals[i + 1] == 0xD8 &&
        regionvals[i + 2] == 0xFF &&
        (regionvals[i + 3] == 0xE0 || regionvals[i + 3] == 0xE1)) {
      num_pics++;
    }
  }

  printf("Num pics: %d\n", num_pics);

  munmap(mappedregion, filestat.st_size);
  fclose(fptr);

  printf("ALL GOOD!\n");
  return 0;
}
