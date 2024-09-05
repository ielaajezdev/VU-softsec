
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_LEN 512
#define DICT_MAX_LINE_LEN 128
#define DICT_LINE_COUNT (2 << 22)

// Remove a trailing newline
void clean(char *str) {
  int i = 0;

  char curr = str[i];
  while (curr != '\0') {
    if (curr == '\n') {
      str[i] = '\0';
    }

    curr = str[++i];
  }
}

// Get the index at which a character occurs for the nth time (if it does)
int nth_occurrence(char c, char *str, int n) {
  int i = 0;
  int occurred = 0;

  char curr = str[i];
  while (curr != '\0') {
    if (curr == c) {
      occurred++;
    }

    if (occurred == n) {
      return i;
    }

    curr = str[++i];
  }

  return -1;
}

// Reads a file from path and then puts all lines of the file in memory starting
// at location curr returns the memory position it left at or -1 on error
int load_dictionary(char *path, char *plain, int curr) {
  FILE *dict = NULL;
  dict = fopen(path, "r");
  if (dict == NULL) {
    printf("Failed to open dictionary from file %s\n", path);
    return -1;
  }

  int i = curr;
  char line[DICT_MAX_LINE_LEN + 1];
  char *word_res = fgets(line, DICT_MAX_LINE_LEN, dict);
  while (word_res != NULL && i < DICT_LINE_COUNT) {
    line[DICT_MAX_LINE_LEN] = '\0';
    clean(line);

    // Copy to correct index
    strcpy(&plain[DICT_MAX_LINE_LEN * i], line);

    // Keep loopin
    word_res = fgets(line, DICT_MAX_LINE_LEN, dict);
    i++;
  }

  fclose(dict);
  return i;
}

// At a memory location with plain lines loaded, try to hash each line and copy
// the hashed result to the "hashed" memory location
void hash_dictionary(char *plain, char *salt, char *hashed, int start,
                     int end) {
  int i = start;
  // Reused buffer
  // char line[DICT_MAX_LINE_LEN + 1];

  while (i < end) {
    // strncpy(line, &plain[DICT_MAX_LINE_LEN * i], DICT_MAX_LINE_LEN);
    char *res = crypt(&plain[DICT_MAX_LINE_LEN * i], salt);
    strncpy(&hashed[DICT_MAX_LINE_LEN * i], res, DICT_MAX_LINE_LEN - 1);

    char *abc;
    struct crypt_data datastore[1] = {0};

    abc = crypt_r("password", "QX", datastore);

    if (i % 100000 == 0) {
      printf("Hashed %d words\n", i);
    }

    // Keep loopin
    i++;
  }
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Incorrect number of arguments. Provide a passwd_path and "
           "shadow_path\n");
    return 1;
  }

  // Allocate to store all dictionary words both hashed and plain
  char *dict_plain = calloc(DICT_LINE_COUNT, DICT_MAX_LINE_LEN);
  char *dict_hashed = calloc(DICT_LINE_COUNT, DICT_MAX_LINE_LEN);
  if (dict_plain == NULL || dict_hashed == NULL) {
    printf("Could not allocate memory for dictionaries\n");
    return 1;
  }
  int dict_curr = 0;

  // Load dictionaries
  dict_curr = load_dictionary("./unique-gutenberg.txt", dict_plain, dict_curr);
  dict_curr = load_dictionary("./unique-top250.txt", dict_plain, dict_curr);
  dict_curr =
      load_dictionary("./variations-gutenberg.txt", dict_plain, dict_curr);
  dict_curr = load_dictionary("./variations-top250.txt", dict_plain, dict_curr);
  // ...

  char *passwd_path = argv[1];
  char *shadow_path = argv[2];

  // Open both files
  FILE *pswd = NULL;
  FILE *shdw = NULL;

  pswd = fopen(passwd_path, "r");
  shdw = fopen(shadow_path, "r");
  if (pswd == NULL || shdw == NULL) {
    printf("fopen failed\n");
    return 1;
  }

  char pswd_line[LINE_LEN];
  char shdw_line[LINE_LEN];

  // Read and execute on all lines one by one
  char *pswd_res = fgets(pswd_line, LINE_LEN, pswd);
  char *shdw_res = fgets(shdw_line, LINE_LEN, shdw);

  // Take the salt from the first line (all salts are the same per file)
  int salt_start = nth_occurrence('$', shdw_res, 1);
  int salt_end = nth_occurrence('$', shdw_res, 3) + 1;
  int salt_len = salt_end - salt_start;
  char salt[salt_len + 1];
  strncpy(salt, &shdw_res[salt_start], salt_len);
  salt[salt_len] = '\0';

  // printf("Got salt '%s'\n", salt);

  // We have the salt, so now hash the dicts
  hash_dictionary(dict_plain, salt, dict_hashed, 0, dict_curr);

  while (pswd_res != NULL && shdw_res != NULL) {
    // The complete hash, as needs to be compared to the one generated based on
    // the wordlist
    int hash_start = nth_occurrence('$', shdw_res, 1);
    int hash_end = nth_occurrence(':', shdw_res, 2);
    int hash_len = hash_end - hash_start;

    char hash[hash_len + 1];
    if (hash_len > 0) {
      strncpy(hash, &shdw_res[hash_start], hash_len);
      hash[hash_len] = '\0';
    }

    // The user is always at the start of the line
    int user_end = nth_occurrence(':', shdw_res, 1);
    int user_len = user_end;

    char user[user_len + 1];
    if (user_len > 0) {
      strncpy(user, shdw_res, user_len);
      user[user_len] = '\0';
    }

    // Try all hashes and print if it is a match
    for (int i = 0; i < dict_curr; i++) {
      char *word = &dict_hashed[DICT_MAX_LINE_LEN * i];
      if (strncmp(word, hash, hash_len) == 0) {
        printf("%s:%s\n", user, &dict_plain[DICT_MAX_LINE_LEN * i]);
        fflush(stdout);
        break;
      }
    }

    // Keep loopin
    pswd_res = fgets(pswd_line, LINE_LEN, pswd);
    shdw_res = fgets(shdw_line, LINE_LEN, shdw);
  }

  // Clean up
  fclose(pswd);
  fclose(shdw);

  return 0;
}