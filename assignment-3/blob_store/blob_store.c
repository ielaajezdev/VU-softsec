#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum command {
  exit_program,
  list_keys,
  add_entry,
  show_blob,
  modify_blob,
  remove_entry,
  command_num,
};

static char const *const command_description[command_num] = {
    [exit_program] = "Exit this program",
    [list_keys] = "List all valid keys",
    [add_entry] = "Add a key-blob entry",
    [show_blob] = "Show blob by key",
    [modify_blob] = "Modify blob by key",
    [remove_entry] = "Remove key-blob entry by key",
};

static void print_banner(void) {
  printf("Welcome to the blob store service!\n\n");
  printf("This service lets you store, read and modify blobs by key.\n");
}

static void print_menu(void) {
  printf("\n");

  for (size_t idx = 0; idx < command_num; ++idx) {
    printf(" - %zu: %s\n", idx, command_description[idx]);
  }

  printf("> ");
}

static int get_string_from_user(size_t len, char buffer[len]) {
  if (!fgets(buffer, len, stdin)) {
    return -1;
  }

  char *newline = strrchr(buffer, '\n');
  if (newline) {
    *newline = '\0';
  }

  return 0;
}

static int get_blob_from_user(size_t len, unsigned char buffer[len]) {
  size_t nmemb_read = fread(buffer, sizeof(unsigned char), len, stdin);
  if (nmemb_read < len) {
    return -1;
  }

  return 0;
}

#define INT_BUFFER_SIZE 8

static int get_integer_from_user(unsigned long *value) {
  char buffer[INT_BUFFER_SIZE];
  if (get_string_from_user(sizeof(buffer), buffer) < 0) {
    return -1;
  }

  char *endptr = NULL;
  *value = strtoul(buffer, &endptr, 0);
  if (*endptr != '\0') {
    return -1;
  }

  return 0;
}

#define MAX_KEY_SIZE 0x10

struct entry {
  char key[MAX_KEY_SIZE];
  unsigned char *blob;
  size_t blob_size;
};

#define ENTRIES_COUNT 10

static void list_keys_handler(struct entry entries[ENTRIES_COUNT]) {
  for (size_t idx = 0; idx < ENTRIES_COUNT; ++idx) {
    if (strlen(entries[idx].key)) {
      printf(" - %s\n", entries[idx].key);
    }
  }
}

static int add_entry_handler(struct entry entries[ENTRIES_COUNT]) {
  size_t idx;
  for (idx = 0; idx < ENTRIES_COUNT; ++idx) {
    if (strlen(entries[idx].key) == 0) {
      break;
    }
  }

  if (idx == ENTRIES_COUNT) {
    puts("All entries are full");
    goto out;
  }

  printf("Insert key > ");
  if (get_string_from_user(sizeof(entries[idx].key), entries[idx].key) < 0) {
    puts("Could not read key");
    goto clean_key;
  }

  size_t blob_size = 0;
  printf("Insert blob size > ");
  if (get_integer_from_user(&blob_size) < 0) {
    puts("Could not read blob size");
    goto clean_key;
  }

  unsigned char *blob = malloc(blob_size);
  if (!blob) {
    puts("Not enough memory for blob");
    goto clean_key;
  }

  memset(blob, 0, blob_size);

  printf("Insert blob > ");
  if (get_blob_from_user(blob_size, blob) < 0) {
    puts("Could not read blob");
    goto free;
  }

  entries[idx].blob = blob;
  entries[idx].blob_size = blob_size;

  return 0;

free:
  free(blob);

clean_key:
  entries[idx].key[0] = '\0';

out:
  return -1;
}

static struct entry *search_entry(struct entry entries[ENTRIES_COUNT]) {
  printf("Insert key > ");
  char key[MAX_KEY_SIZE];
  if (get_string_from_user(sizeof(key), key) < 0) {
    puts("Could not read key");
    return NULL;
  }

  for (size_t idx = 0; idx < ENTRIES_COUNT; ++idx) {
    if (!strncmp(entries[idx].key, key, MAX_KEY_SIZE)) {
      return &entries[idx];
    }
  }

  return NULL;
}

static int show_blob_handler(struct entry entries[ENTRIES_COUNT]) {
  struct entry *ent = search_entry(entries);
  if (!ent) {
    puts("Entry not found");
    return -1;
  }

  puts("Blob:");
  fwrite(ent->blob, sizeof(unsigned char), ent->blob_size, stdout);

  return 0;
}

static int modify_blob_handler(struct entry entries[ENTRIES_COUNT]) {
  struct entry *ent = search_entry(entries);
  if (!ent) {
    puts("Entry not found");
    return -1;
  }

  printf("Insert new blob > ");
  if (get_blob_from_user(ent->blob_size, ent->blob) < 0) {
    puts("Could not read new blob");
    return -1;
  }

  return 0;
}

static int remove_entry_handler(struct entry entries[ENTRIES_COUNT]) {
  struct entry *ent = search_entry(entries);
  if (!ent) {
    puts("Entry not found");
    return -1;
  }

  free(ent->blob);
  ent->key[0] = '\0';

  return 0;
}

int main(void) {
  setbuf(stdout, NULL);

  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  print_banner();

  struct entry entries[ENTRIES_COUNT];
  memset(entries, 0, sizeof(entries));

  unsigned long choice = 0;
  do {
    print_menu();
    if (get_integer_from_user(&choice) < 0) {
      printf("Input is not a valid number\n");
      break;
    }

    switch (choice) {
    case list_keys:
      list_keys_handler(entries);
      break;

    case add_entry:
      add_entry_handler(entries);
      break;

    case show_blob:
      show_blob_handler(entries);
      break;

    case modify_blob:
      modify_blob_handler(entries);
      break;

    case remove_entry:
      remove_entry_handler(entries);
      break;

    case exit_program:
      break;

    default:
      printf("Invalid choice: %lu\n", choice);
      break;
    }
  } while (choice != exit_program);

  return EXIT_SUCCESS;
}