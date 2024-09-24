#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mymalloc.h"
#include "noaslr.h"

struct pixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

struct image {
  size_t width;
  size_t height;
  struct pixel *pixels;
};

struct raw_data {
  char *data;
  size_t size;
};

static void validate_magic_number(char *data) {
  if (data[0] != 'P' && data[1] != '6')
    errx(EXIT_FAILURE, "Failed to parse magic number");
}

static size_t skip_comments(char *data, size_t cursor) {
  while (!isspace(data[cursor]))
    cursor++;

  while (isspace(data[cursor]))
    cursor++;

  if (data[cursor] == '#') {
    while (data[cursor] != '\n' && data[cursor] != '\r')
      cursor++;
    while (data[cursor] == '\n' || data[cursor] == '\r')
      cursor++;
  }

  while (isspace(data[cursor]))
    cursor++;

  return cursor;
}

static struct image parse_image(struct raw_data *raw_data) {
  char *data = raw_data->data;

  struct image image = {0};
  validate_magic_number(data);
  size_t cursor = 2;

  cursor = skip_comments(data, cursor);

  if (sscanf(&data[cursor], "%lu", &image.width) != 1)
    errx(EXIT_FAILURE, "Invalid width");

  cursor = skip_comments(data, cursor);

  if (sscanf(&data[cursor], "%lu", &image.height) != 1)
    errx(EXIT_FAILURE, "Invalid height");

  cursor = skip_comments(data, cursor);

  int color_value;
  if (sscanf(&data[cursor], "%u", &color_value) != 1)
    errx(EXIT_FAILURE, "No color value provided");

  if (color_value != 255)
    errx(EXIT_FAILURE, "Only 8 bit colors are supported");

  cursor = skip_comments(data, cursor);

  image.pixels = mymalloc(image.width * image.height * sizeof(struct pixel));
  if (!image.pixels)
    err(EXIT_FAILURE, "Failed to allocate pixel array");

  for (int x = 0; x < image.height; ++x) {
    struct pixel *row = &image.pixels[x * image.width];
    for (int y = 0; y < image.width; ++y) {
      struct pixel *pixel = &row[y];
      if (cursor + 2 >= raw_data->size)
        break;
      pixel->red = data[cursor];
      pixel->green = data[cursor + 1];
      pixel->blue = data[cursor + 2];
      cursor += 3;
    }
  }

  return image;
}

// Lookup table for Base64 characters
static const char lookup_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,
    ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
    ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35,
    ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53,
    ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63};

// Helper function to decode a 4-character Base64 block into 3 bytes
static void base64_decode_block(const char *base64_encoded, char *decoded,
                                size_t *output_index) {
  uint32_t block = (lookup_table[base64_encoded[0]] << 18) |
                   (lookup_table[base64_encoded[1]] << 12) |
                   (lookup_table[base64_encoded[2]] << 6) |
                   lookup_table[base64_encoded[3]];

  decoded[(*output_index)++] = (block >> 16) & 0xFF;
  if (base64_encoded[2] != '=')
    decoded[(*output_index)++] = (block >> 8) & 0xFF;
  if (base64_encoded[3] != '=')
    decoded[(*output_index)++] = block & 0xFF;
}

// Function to handle the final block with padding
static void base64_decode_final_block(const char *base64_encoded, size_t length,
                                      char *decoded, size_t *output_index) {
  uint32_t block = (lookup_table[base64_encoded[0]] << 18) |
                   (lookup_table[base64_encoded[1]] << 12);

  decoded[(*output_index)++] = (block >> 16) & 0xFF;

  if (length > 2 && base64_encoded[2] != '=') {
    block |= (lookup_table[base64_encoded[2]] << 6);
    decoded[(*output_index)++] = (block >> 8) & 0xFF;
  }

  if (length > 3 && base64_encoded[3] != '=') {
    block |= lookup_table[base64_encoded[3]];
    decoded[(*output_index)++] = block & 0xFF;
  }
}

struct raw_data base64_decode(size_t length, char base64_encoded[length]) {
  if (length < 4)
    errx(EXIT_FAILURE, "base64 string is to short");

  size_t required_size = (length / 4) * 3;
  if (base64_encoded[length - 1] == '=')
    required_size--;
  if (base64_encoded[length - 2] == '=')
    required_size--;

  struct raw_data decoded = {.size = required_size,
                             .data = mymalloc(required_size)};

  for (size_t i = 0; i < length; ++i) {
    switch (base64_encoded[i]) {
    case '0' ... '9':
    case 'A' ... 'Z':
    case 'a' ... 'z':
    case '+':
    case '/':
      continue;
    case '=':
      if (i == length - 1)
        continue;
      if (i == length - 2 && base64_encoded[length - 1] == '=')
        continue;
    default:
      errx(EXIT_FAILURE, "Invalid base64 string provided");
    }
  }

  size_t output_index = 0;
  size_t i = 0;
  // Decode complete 4-character blocks
  for (; i + 4 <= length; i += 4)
    base64_decode_block(&base64_encoded[i], decoded.data, &output_index);

  // Decode any remaining characters
  if (i < length)
    base64_decode_final_block(&base64_encoded[i], length - i, decoded.data,
                              &output_index);

  return decoded;
}

static void print_menu(void) {
  puts("\n    ____                              _____ __                       "
       "  "
       "   ");
  puts("   /  _/___ ___  ____ _____ ____     / ___// /_____  _________ _____ "
       "____ ");
  puts("   / // __ `__ \\/ __ `/ __ `/ _ \\    \\__ \\/ __/ __ \\/ ___/ __ `/ "
       "__ `/ _ \\");
  puts(" _/ // / / / / / /_/ / /_/ /  __/   ___/ / /_/ /_/ / /  / /_/ / /_/ /  "
       "__/");
  puts("/___/_/ /_/ /_/\\__,_/\\__, /\\___/   /____/\\__/\\____/_/   "
       "\\__,_/\\__, /\\___/ ");
  puts("                    /____/                                   /____/    "
       "   ");
  puts("1. Add image");
  puts("2. Remove image");
  puts("3. Show list of images");
  puts("4. Display image");
  puts("5. Exit");
  printf("> ");
}

#define NR_OF_ENTRIES (8)
struct entry {
  bool used;
  char *name;
  struct image image;
};
static struct entry entries[NR_OF_ENTRIES] = {0};

static void add_image(void) {
  size_t idx = -1;
  for (size_t i = 0; i < NR_OF_ENTRIES; ++i)
    if (!entries[i].used) {
      idx = i;
      break;
    }

  if (idx == -1)
    errx(EXIT_FAILURE, "No empty slot available");

  struct entry *entry = &entries[idx];
  entry->used = true;

  char *line = NULL;
  size_t n = 0;
  size_t line_length = 0;

  printf("Name of the image: ");
  if (line_length = getline(&line, &n, stdin), line_length == -1)
    errx(EXIT_FAILURE, "Failed to read line for name");

  assert(line_length > 0);
  entry->name = mymalloc(line_length);
  memcpy(entry->name, line, line_length);
  entry->name[line_length - 1] = 0;

  printf("Base64 encoded image data: ");
  char *data = NULL;
  size_t data_length = 0;
  while (line_length = getline(&line, &n, stdin), line_length != -1) {
    if (line_length == 1 && line[0] == '\n')
      break;

    assert(line_length > 1);
    data = myrealloc(data, data_length + line_length);
    for (size_t i = 0; i < line_length; ++i) {
      if (line[i] != '\n')
        data[data_length++] = line[i];
    }
  }

  if (!data)
    errx(EXIT_FAILURE, "No data provided");
  assert(data_length > 0);
  data[data_length] = 0;

  struct raw_data raw_data = base64_decode(strlen(data), data);
  myfree(data);

  entry->image = parse_image(&raw_data);
  myfree(raw_data.data);

  printf("Succesfully added the image into slot %ld\n", idx);
  free(line);
}

static void remove_image(void) {
  printf("Index: ");
  int c = getchar();
  if (c < '0' || c > '8')
    errx(EXIT_FAILURE, "Index out of bounds");
  int index = c - 0x30;
  do
    c = getchar();
  while (c != '\r' && c != '\n');

  if (!entries[index].used)
    errx(EXIT_FAILURE, "Trying to free a non existing entry");

  myfree(entries[index].name);
  myfree(entries[index].image.pixels);
  memset(&entries[index], 0, sizeof(entries[index]));

  printf("Removed image at index %d!", index);
}

static void list_images(void) {
  size_t printed = 0;
  for (size_t i = 0; i < NR_OF_ENTRIES; ++i) {
    struct entry entry = entries[i];
    if (entry.used) {
      ++printed;
      printf("Index: %ld\n", i);
      printf("Name: %s\n", entry.name);
      printf("Dimensions: %ld x %ld\n", entry.image.width, entry.image.height);
    }
  }
  if (!printed)
    puts("No images available");
}

const unsigned char rgb_to_grayscale(const unsigned char r,
                                     const unsigned char g,
                                     const unsigned char b) {
  return (((unsigned int)r * 299) + ((unsigned int)g * 587) +
          ((unsigned int)b * 114)) /
         1000;
}

const char *grayscale_to_character(const unsigned char x) {
  if (x > 204)
    return "██";
  else if (x > 153)
    return "▓▓";
  else if (x > 102)
    return "▒▒";
  else if (x > 51)
    return "░░";
  else
    return "  ";
}

static void color_escape_code(size_t length, char out[length], unsigned char r,
                              unsigned char g, unsigned char b) {
  const unsigned char x = rgb_to_grayscale(r, g, b);
  snprintf(out, length, "\x1b[48;2;%d;%d;%dm\x1b[38;2;%d;%d;%dm%s", r, g, b, r,
           g, b, grayscale_to_character(x));
}

static void display_image(void) {
  printf("Index: ");
  int c = getchar();
  if (c < '0' || c > '8')
    errx(EXIT_FAILURE, "Index out of bounds");
  int index = c - 0x30;
  do
    c = getchar();
  while (c != '\r' && c != '\n');

  struct entry entry = entries[index];
  if (!entry.used)
    errx(EXIT_FAILURE, "Trying to display a non existing entry");

  struct image image = entry.image;

  char output[128] = {0};
  for (int x = 0; x < image.height; ++x) {
    struct pixel *row = &image.pixels[x * image.width];
    for (int y = 0; y < image.width; ++y) {
      struct pixel *pixel = &row[y];

      color_escape_code(128, output, pixel->red, pixel->green, pixel->blue);
      printf("%s", output);
    }
    printf("\x1b[0m\n");
  }
}

int main_no_aslr(int argc, char **argv, char **envp) {
  int choice = 0;
  char *line = NULL;
  size_t n = 0;

  setbuf(stdout, NULL);

  do {
    print_menu();

    int scanned = getline(&line, &n, stdin);
    if (scanned == -1)
      errx(EXIT_FAILURE, "Failed to read line");

    if (sscanf(line, "%d", &choice) == 1)
      switch (choice) {
      case 1:
        add_image();
        break;
      case 2:
        remove_image();
        break;
      case 3:
        list_images();
        break;
      case 4:
        display_image();
        break;
      default:
        break;
      }
  } while (choice != 5);

  return 0;
}
