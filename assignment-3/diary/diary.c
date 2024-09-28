#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_ENTRIES (8)
#define ENTRY_SIZE (60 + 1)

struct Entry {
  size_t timestamp; // 8 bytes
  char content[ENTRY_SIZE];
} __attribute__((packed));

void print_menu() {
  printf("\n");
  printf("E-Diary\n");
  printf("1. Add Entry\n");
  printf("2. Read Entry\n");
  printf("3. Exit\n");
  printf("> ");
}

bool read_int(int *result) {
  if (scanf("%d", result) != 1) {
    fprintf(stderr, "Error: Invalid input\n");
    return false;
  }

  while (getchar() != '\n')
    ;
  return true;
}

bool read_long(size_t *result) {
  if (scanf("%lu", result) != 1) {
    fprintf(stderr, "Error: Invalid input\n");
    return false;
  }

  while (getchar() != '\n')
    ;
  return true;
}

void add_entry(struct Entry diary[MAX_ENTRIES], int max_size) {
  volatile int read_size = max_size;
  short int page_no = 0;
  size_t timestamp = 0;

  printf("Please select a page (0 - %d): ", MAX_ENTRIES - 1);
  if (!read_int((int *)&page_no)) {
    return;
  }

  if (page_no < 0) {
    printf("Invalid page number!\n");
    return;
  }

  if (read_size == 0) {
    read_size = ENTRY_SIZE;
  }
  printf("Please enter the content below:\n");
  printf("Dear diary, ");
  if (!fgets(diary[page_no].content, read_size, stdin)) {
    fprintf(stderr, "Error: Invalid input\n");
    return;
  }

  printf("Please enter a timestamp (0 to autogenerate): ");
  if (!read_long((size_t *)&timestamp) || timestamp == 0) {
    diary[page_no].timestamp = time(NULL);
  } else {
    diary[page_no].timestamp = timestamp;
  }

  strtok(diary[page_no].content, "\n");
  if (diary[page_no].content[0] == '\n') {
    diary[page_no].content[0] = 0x00;
  }
}

void view_entry(struct Entry diary[MAX_ENTRIES]) {
  int page_no = 0;

  printf("\n");
  printf("+------+-------------------+\n");
  printf("| page | preview           |\n");
  printf("+------+-------------------+\n");
  for (size_t id = 0; id < MAX_ENTRIES; id++) {
    printf("|   %02lu | %-18.18s|\n", id, diary[id].content);
  }
  printf("+------+-------------------+\n\n");

  printf("Please select a page (0 - %d): ", MAX_ENTRIES - 1);
  if (!read_int(&page_no)) {
    return;
  }

  if (page_no < 0) {
    printf("Invalid page number!\n");
    return;
  }

  printf("Timestamp: 0x%08lx\n", diary[page_no].timestamp);
  printf("Content: %s\n", diary[page_no].content);
}

int main() {
  int choice = 0;
  struct Entry diary[MAX_ENTRIES];
  memset(diary, 0, sizeof(diary));
  setbuf(stdout, NULL);

  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  do {
    print_menu();
    if (!read_int(&choice)) {
      break;
    }

    switch (choice) {
    case 1:
      add_entry(diary, ENTRY_SIZE);
      break;

    case 2:
      view_entry(diary);
      break;

    case 3:
      break;

    default:
      printf("Invalid input!\n");
    }

  } while (choice != 3);

  return 0;
}