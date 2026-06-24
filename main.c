#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  if (mkdir("recovered", 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "Failed to create recovered/: %s\n", strerror(errno));
    return 1;
  }

  int recovered = 0;
  for (int i = 0; i < num_pics; i++) {
    off_t start = offsets[i];
    off_t window_end = (i == num_pics - 1) ? filestat.st_size : offsets[i + 1];

    off_t last_eoi = -1;
    for (off_t p = start; p + 1 < window_end; p++) {
      if (regionvals[p] == 0xFF && regionvals[p + 1] == 0xD9) {
        last_eoi = p;
      }
    }

    if (last_eoi < 0) {
      fprintf(stderr, "photo %d: no EOI found in window, skipping\n", i + 1);
      continue;
    }

    off_t end = last_eoi + 2;
    size_t length = (size_t)(end - start);

    char filename[64];
    snprintf(filename, sizeof(filename), "recovered/photo_%02d.jpg", i + 1);

    FILE *out = fopen(filename, "wb");
    if (out == NULL) {
      fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
      continue;
    }
    if (fwrite(regionvals + start, 1, length, out) != length) {
      fprintf(stderr, "Failed to write all bytes to %s\n", filename);
      fclose(out);
      continue;
    }
    fclose(out);
    recovered++;
  }

  printf("Found %d JPEGs, recovered %d to ./recovered/\n", num_pics, recovered);
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
