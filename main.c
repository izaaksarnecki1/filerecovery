#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

off_t *array_add_elem(off_t *arr, int num_elems, int *capacity,
                      off_t elem_to_add);

void print_arr(off_t *arr, int num_elems);

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

  int pics_capacity = 64;
  int num_pics = 0;
  off_t *offsets = malloc(pics_capacity * sizeof(off_t));

  if (offsets == NULL) {
    fprintf(stderr, "Failed allocating.\n");
    return 1;
  }

  for (off_t i = 0; i < filestat.st_size - 3; i++) {
    if (regionvals[i] == 0xFF && regionvals[i + 1] == 0xD8 &&
        regionvals[i + 2] == 0xFF &&
        (regionvals[i + 3] == 0xE0 || regionvals[i + 3] == 0xE1)) {
      offsets = array_add_elem(offsets, num_pics, &pics_capacity, i);
      num_pics++;
    }
  }

  printf("Num pics: %d\n", num_pics);
  print_arr(offsets, num_pics);

  free(offsets);
  munmap(mappedregion, filestat.st_size);
  fclose(fptr);
  printf("ALL GOOD!\n");
  return 0;
}

off_t *array_add_elem(off_t *arr, int num_elems, int *capacity,
                      off_t elem_to_add) {
  if (num_elems == *capacity - 1) {
    *capacity *= 2;
    off_t *tmp = realloc(arr, *capacity * sizeof(off_t));
    if (tmp == NULL) {
      fprintf(stderr, "Failed increasing size of array\n");
      return arr;
    }
    arr = tmp;
  }

  arr[num_elems] = elem_to_add;
  return arr;
}

void print_arr(off_t *arr, int num_elems) {
  for (int i = 0; i < num_elems; i++) {
    printf("offset %d: %lld\n", i, arr[i]);
  }
}
