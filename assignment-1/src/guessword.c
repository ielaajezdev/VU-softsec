
#include <crypt.h>
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_LEN 512
#define DICT_MAX_LINE_LEN 128
#define DICT_LINE_COUNT (2 << 23)
#define USERS_LINE_LEN 128
#define USERS_LINE_COUNT 10000

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

  while (str[i] != '\0') {
    if (str[i] == c) {
      occurred++;
    }

    if (occurred == n) {
      return i;
    }

    i++;
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
typedef struct {
  char *plain;
  char *salt;
  char *hashed;
  int start;
  int end;
} hash_dict_args;

void *hash_dictionary(void *raw_args) {
  hash_dict_args *args = (hash_dict_args *)raw_args;
  int i = args->start;
  int a = 0;

  printf("Started hashing from %d to %d\n", args->start, args->end);

  while (i < args->end) {
    struct crypt_data datastore[1] = {0};
    char *res =
        crypt_r(&args->plain[DICT_MAX_LINE_LEN * i], args->salt, datastore);
    strncpy(&args->hashed[DICT_MAX_LINE_LEN * i], res, DICT_MAX_LINE_LEN - 1);

    if (a % 100000 == 0) {
      printf("Hashed %d words\n", a);
    }

    // Keep loopin
    i++;
    a++;
  }

  return NULL;
}

// Hashes, checks if a hash matches, prints the required user:plain combo and
// returns 1 if it was a match
int try_hash(char *plain, char *salt, char *hash, char *user) {
  struct crypt_data datastore[1] = {0};
  char *res = crypt_r(plain, salt, datastore);

  if (res != NULL && strncmp(res, hash, 256) == 0) {
    printf("%s:%s\n", user, plain);
    fflush(stdout);
    return 1;
  }

  return 0;
}

// Returns 1 if a username variation was found matching
int try_username_variations(char *src, char *salt, char *hash, char *user) {
  int len = strlen(src) + 64; // leave room for variations
  char dup[len + 1];
  if (len > 0) {
    strcpy(dup, src);
    dup[len] = '\0';
  }

  // Try as is
  if (try_hash(dup, salt, hash, user) == 1) {
    return 1;
  }

  // Try the username in uppercase
  int i = 0;
  while (dup[i] != '\0') {
    dup[i] = toupper(dup[i]);
    // Keep loopin
    i++;
  }
  if (try_hash(dup, salt, hash, user) == 1) {
    return 1;
  }

  // Try the username with a single letter lowercased
  i = 0;
  while (dup[i] != '\0') {
    if (i > 0) {
      dup[i - 1] = toupper(dup[i - 1]);
    }

    dup[i] = tolower(dup[i]);
    if (try_hash(dup, salt, hash, user) == 1) {
      return 1;
    }

    // Keep loopin
    i++;
  }

  // Try the username in lowercase
  i = 0;
  while (dup[i] != '\0') {
    dup[i] = tolower(dup[i]);
    // Keep loopin
    i++;
  }
  if (try_hash(dup, salt, hash, user) == 1) {
    return 1;
  }

  // Try the username with a single letter uppercased
  i = 0;
  while (dup[i] != '\0') {
    if (i > 0) {
      dup[i - 1] = tolower(dup[i - 1]);
    }

    dup[i] = toupper(dup[i]);
    if (try_hash(dup, salt, hash, user) == 1) {
      return 1;
    }
    // Keep loopin
    i++;
  }

  return 0;
}

void crack_and_print_single(char *user, char *hash, int hash_len, int dict_size,
                            char *dict_hashed, char *dict_plain,
                            char *user_passwd_line, char *salt) {
  // Try all pre-computed hashes from the wordlist first
  for (int j = 0; j < dict_size; j++) {
    char *word = &dict_hashed[DICT_MAX_LINE_LEN * j];
    if (strncmp(word, hash, hash_len) == 0) {
      printf("%s:%s\n", user, &dict_plain[DICT_MAX_LINE_LEN * j]);
      fflush(stdout);
      return;
    }
  }

  // Get the full name of a user
  int full_name_start = nth_occurrence(':', user_passwd_line, 4) + 1;
  int full_name_end = nth_occurrence(',', user_passwd_line, 1);
  int full_name_len = full_name_end - full_name_start;

  char full_name[full_name_len];
  if (full_name_len > 0) {
    strncpy(full_name, &user_passwd_line[full_name_start], full_name_len);
    full_name[full_name_len] = '\0';

    int first_name_end = nth_occurrence(' ', full_name, 1);
    int first_name_len = first_name_end;

    char first_name[first_name_len + 1];
    if (first_name_len > 0) {
      strncpy(first_name, full_name, first_name_len);
      first_name[first_name_len] = '\0';

      int match = try_username_variations(first_name, salt, hash, user);
      if (match == 1) {
        return;
      }
    }
  }
}

typedef struct {
  char *users_shadow;
  char *users_passwd;
  char *dict_hashed;
  char *dict_plain;
  char *salt;
  int dict_size;
  int start;
  int end;
} crack_print_args;

// Prints when it found a match in the pre-computed passwords and
// creates variations with usernames to try and hash
void *crack_and_print(void *raw_args) {
  crack_print_args *args = (crack_print_args *)raw_args;

  for (int i = args->start; i < args->end; i++) {
    // The hash that we need to compare to (as in the shadow file line)
    int hash_start =
        nth_occurrence('$', &args->users_shadow[USERS_LINE_LEN * i], 1);
    int hash_end =
        nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 2);
    int hash_len = hash_end - hash_start;

    char hash[hash_len + 1];
    if (hash_len > 0) {
      strncpy(hash, &args->users_shadow[USERS_LINE_LEN * i + hash_start],
              hash_len);
      hash[hash_len] = '\0';
    }

    // The user (user id) is always at the start of the line
    int user_end =
        nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 1);
    int user_len = user_end;

    char user[user_len + 1];
    if (user_len > 0) {
      strncpy(user, &args->users_shadow[USERS_LINE_LEN * i], user_len);
      user[user_len] = '\0';
    }

    crack_and_print_single(user, hash, hash_len, args->dict_size,
                           args->dict_hashed, args->dict_plain,
                           &args->users_passwd[USERS_LINE_LEN * i], args->salt);
  }

  return NULL;
}

typedef struct {
  // The dictionaries to read/write
  char *dict_plain;
  char *dict_hashed;
  // For placing generated words IN the dict
  int dict_start;
  int dict_end;
  // For reading plain words FROM the dict
  // (combine 11-letterwords with 13-letterwords)
  int plain_11_start;
  int plain_11_end;
  int plain_13_start;
  int plain_13_end;
  // Always needed
  char *salt;
} generate_dict_args;

// Cannot be preloaded due to size constraints
void *generate_24_letter_dictionary(void *raw_args) {
  generate_dict_args *args = (generate_dict_args *)raw_args;

  char plain_word[25];
  int curr_index = 0;

  // Pick an 11 and 13-letteword, merge them and store the hashed result
  for (int i = args->plain_11_start; i < args->plain_11_end; i++) {
    for (int j = args->plain_13_start; j < args->plain_13_end; j++) {
      // Merge the words
      strncpy(plain_word, &args->dict_plain[DICT_MAX_LINE_LEN * i], 11);
      strncpy(&plain_word[11], &args->dict_plain[DICT_MAX_LINE_LEN * j], 13);
      plain_word[24] = '\0';

      // Convert to lowercase
      // todo: check why uppercase is selected
      for (int k = 0; k < 25; k++) {
        plain_word[k] = tolower(plain_word[k]);
      }

      int dict_index = args->dict_start + curr_index;

      struct crypt_data datastore[1] = {0};
      char *res = crypt_r(plain_word, args->salt, datastore);
      // copy hash
      strncpy(&args->dict_hashed[DICT_MAX_LINE_LEN * dict_index], res,
              DICT_MAX_LINE_LEN - 1);
      // copy plain
      strncpy(&args->dict_plain[DICT_MAX_LINE_LEN * dict_index], plain_word,
              25);

      curr_index++;
    }
  }

  return NULL;
}

void compare_and_print_single(char *user, char *hash, char *dict_hashed,
                              char *dict_plain, int dict_start, int dict_end) {
  for (int i = dict_start; i < dict_end; i++) {
    // Get the computed hash
    char *try_hash = &dict_hashed[DICT_MAX_LINE_LEN * i];

    if (strcmp(try_hash, hash) == 0) {
      printf("%s:%s\n", user, &dict_plain[DICT_MAX_LINE_LEN * i]);
      fflush(stdout);
      return;
    }
  }
}

typedef struct {
  char *users_shadow;
  int users_start;
  int users_end;
  // The dictionary that can be accessed
  char *dict_hashed;
  char *dict_plain;
  int dict_start;
  int dict_end;
} compare_and_print_args;

void *compare_and_print(void *raw_args) {
  compare_and_print_args *args = (compare_and_print_args *)raw_args;

  for (int i = args->users_start; i < args->users_end; i++) {
    // The hash that we need to compare to (as in the shadow file line)
    int hash_start =
        nth_occurrence('$', &args->users_shadow[USERS_LINE_LEN * i], 1);
    int hash_end =
        nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 2);
    int hash_len = hash_end - hash_start;

    char hash[hash_len + 1];
    if (hash_len > 0) {
      strncpy(hash, &args->users_shadow[USERS_LINE_LEN * i + hash_start],
              hash_len);
      hash[hash_len] = '\0';
    }

    // The user (user id) is always at the start of the line
    int user_end =
        nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 1);
    int user_len = user_end;

    char user[user_len + 1];
    if (user_len > 0) {
      strncpy(user, &args->users_shadow[USERS_LINE_LEN * i], user_len);
      user[user_len] = '\0';
    }

    compare_and_print_single(user, hash, args->dict_hashed, args->dict_plain,
                             args->dict_start, args->dict_end);
  }

  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Incorrect number of arguments. Provide a passwd_path and "
           "shadow_path\n");
    return 1;
  }

  // Allocate to store all dictionary words
  char *dict_plain = calloc(DICT_LINE_COUNT, DICT_MAX_LINE_LEN);
  if (dict_plain == NULL) {
    printf("Could not allocate memory to load dictionaries\n");
    return 1;
  }
  int dict_curr = 0;

  // Load dictionaries
  dict_curr = load_dictionary("./unique-gutenberg.txt", dict_plain, dict_curr);
  int unique_gutenberg_end = dict_curr;
  dict_curr = load_dictionary("./unique-top250.txt", dict_plain, dict_curr);
  // dict_curr =
  //     load_dictionary("./variations-gutenberg.txt", dict_plain, dict_curr);
  // dict_curr = load_dictionary("./variations-top250.txt", dict_plain,
  // dict_curr);
  // ...

  if (dict_curr <= 0) {
    printf("Failed to load dictionaries...\n");
    return 1;
  }

  // Create memory to allocate all hashes. Their indexes correspond with the
  // plain words
  char *dict_hashed = calloc(dict_curr + 1, DICT_MAX_LINE_LEN);

  char *passwd_path = argv[1];
  char *shadow_path = argv[2];

  // Open both user files
  FILE *pswd = NULL;
  FILE *shdw = NULL;
  pswd = fopen(passwd_path, "r");
  shdw = fopen(shadow_path, "r");
  if (pswd == NULL || shdw == NULL) {
    printf("fopen failed\n");
    return 1;
  }

  // Create memory to put all users in
  char *user_pswd = calloc(USERS_LINE_COUNT, USERS_LINE_LEN);
  char *user_shdw = calloc(USERS_LINE_COUNT, USERS_LINE_LEN);
  if (user_pswd == NULL || user_shdw == NULL) {
    printf("Could not allocate memory to load users\n");
    return 1;
  }
  // Keep track of where we are in the user memory
  int user_curr = 0;

  // Read all lines one by one
  char *pswd_res =
      fgets(&user_pswd[USERS_LINE_LEN * user_curr], LINE_LEN, pswd);
  char *shdw_res =
      fgets(&user_shdw[USERS_LINE_LEN * user_curr], LINE_LEN, shdw);
  user_curr++;

  // Take the salt from the first line (all salts are the same per file)
  int salt_start = nth_occurrence('$', shdw_res, 1);
  int salt_end = nth_occurrence('$', shdw_res, 3) + 1;
  int salt_len = salt_end - salt_start;
  char salt[salt_len + 1];
  strncpy(salt, &shdw_res[salt_start], salt_len);
  salt[salt_len] = '\0';

  // Continue reading lines
  while (pswd_res != NULL && shdw_res != NULL && user_curr < USERS_LINE_COUNT) {
    // Keep loopin, putting everything in memory
    pswd_res = fgets(&user_pswd[USERS_LINE_LEN * user_curr], LINE_LEN, pswd);
    shdw_res = fgets(&user_shdw[USERS_LINE_LEN * user_curr], LINE_LEN, shdw);
    user_curr++;
  }

  // We don't need the files anymore
  fclose(pswd);
  fclose(shdw);

  //
  // Low-hanging fruit: with the pre-computed dictionaries and basic username
  // variations, ~40% of the password file can be discovered
  //

  // Split up the plain word dictionary into 4, so that it can be parallelized
  int frac = dict_curr / 4;

  // Create 4 threads and wait for them to finish
  pthread_t dict_thread[4];
  hash_dict_args args[4];

  args[0].start = 0;
  args[0].end = frac;
  args[1].start = frac;
  args[1].end = frac * 2;
  args[2].start = frac * 2;
  args[2].end = frac * 3;
  args[3].start = frac * 3;
  args[3].end = dict_curr;

  for (int i = 0; i < 4; i++) {
    args[i].plain = dict_plain;
    args[i].hashed = dict_hashed;
    args[i].salt = salt;
    pthread_create(&dict_thread[i], NULL, hash_dictionary, &args[i]);
  }

  // Wait for all threads to finish
  for (int i = 0; i < 4; i++) {
    pthread_join(dict_thread[i], NULL);
  }

  printf("Done hashing!\n");

  // Split up the users directory into 4, so that it can be parallelized
  frac = user_curr / 4;

  // Create 4 threads and wait for them to finish
  pthread_t user_thread[4];
  crack_print_args user_args[4];

  user_args[0].start = 0;
  user_args[0].end = frac;
  user_args[1].start = frac;
  user_args[1].end = frac * 2;
  user_args[2].start = frac * 2;
  user_args[2].end = frac * 3;
  user_args[3].start = frac * 3;
  user_args[3].end = user_curr;

  for (int i = 0; i < 4; i++) {
    user_args[i].users_passwd = user_pswd;
    user_args[i].users_shadow = user_shdw;
    user_args[i].dict_hashed = dict_hashed;
    user_args[i].dict_plain = dict_plain;
    user_args[i].dict_size = dict_curr;
    user_args[i].salt = salt;
    pthread_create(&user_thread[i], NULL, crack_and_print, &user_args[i]);
  }

  // Wait for all threads to finish
  for (int i = 0; i < 4; i++) {
    pthread_join(user_thread[i], NULL);
  }

  //
  // Complex: try the 24-letterword variations (should be around ~56%)
  //

  // Find where the 11 and 13 letterwords sections start in the gutenberg unique
  // dict
  int gutenberg_11_start = -1;
  int gutenberg_11_end = -1;
  int gutenberg_13_start = -1;
  int gutenberg_13_end = -1;

  for (int i = 0; i < unique_gutenberg_end; i++) {
    int len = strlen(&dict_plain[DICT_MAX_LINE_LEN * i]);
    if (len == 11) {
      gutenberg_11_start = i;
    } else if (len == 12) {
      gutenberg_11_end = i;
    } else if (len == 13) {
      gutenberg_13_start = i;
    } else if (len == 14) {
      gutenberg_13_end = i;
    }
  }

  if (gutenberg_11_start > 0 && gutenberg_11_end > 0 &&
      gutenberg_13_start > 0 && gutenberg_13_end > 0) {
    int word_11_count = gutenberg_11_end - gutenberg_11_start;
    int word_13_count = gutenberg_13_end - gutenberg_13_start;

    // Split the 11 letters up in 4 equal partitions
    frac = (word_11_count / 4) + 1;

    // Create 4 threads and wait for them to finish
    pthread_t generate_thread[4];
    generate_dict_args generate_args[4];

    // Each thread gets a block of memory from the total dict that they can
    // insert into, based on how many variations there are
    generate_args[0].plain_11_start = 0;
    generate_args[0].plain_11_end = frac;
    generate_args[0].dict_start = dict_curr;
    generate_args[0].dict_end = frac * word_13_count;

    generate_args[1].plain_11_start = frac;
    generate_args[1].plain_11_end = frac * 2;
    generate_args[1].dict_start = frac * word_13_count;
    generate_args[1].dict_end = frac * 2 * word_13_count;

    generate_args[2].plain_11_start = frac * 2;
    generate_args[2].plain_11_end = frac * 3;
    generate_args[2].dict_start = frac * 2 * word_13_count;
    generate_args[2].dict_end = frac * 3 * word_13_count;

    generate_args[3].plain_11_start = frac * 3;
    generate_args[3].plain_11_end = frac * 4;
    generate_args[3].dict_start = frac * 3 * word_13_count;
    generate_args[3].dict_end = frac * 4 * word_13_count;

    for (int i = 0; i < 4; i++) {
      generate_args[i].dict_plain = dict_plain;
      generate_args[i].dict_hashed = dict_hashed;
      generate_args[i].salt = salt;

      pthread_create(&generate_thread[i], NULL, generate_24_letter_dictionary,
                     &generate_args[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < 4; i++) {
      pthread_join(generate_thread[i], NULL);
    }

    int dict_24_word_start = dict_curr;
    dict_curr += word_11_count * word_13_count;

    printf("Generated all %d 24-letter words!\n",
           dict_curr - dict_24_word_start);

    // Now create 4 threads for cracking again
    pthread_t compare_threads[4];
    compare_and_print_args compare_args[4];

    // Each thread checks 1/4th of the users
    frac = user_curr / 4;
    compare_args[0].users_start = 0;
    compare_args[0].users_end = frac;
    compare_args[1].users_start = frac;
    compare_args[1].users_end = frac * 2;
    compare_args[2].users_start = frac * 2;
    compare_args[2].users_end = frac * 3;
    compare_args[3].users_start = frac * 3;
    compare_args[3].users_end = user_curr;

    for (int i = 0; i < 4; i++) {
      compare_args[i].dict_hashed = dict_hashed;
      compare_args[i].dict_plain = dict_plain;
      compare_args[i].dict_start = dict_24_word_start;
      compare_args[i].dict_end = dict_curr;
      compare_args[i].users_shadow = user_shdw;

      pthread_create(&compare_threads[i], NULL, compare_and_print,
                     &compare_args[i]);
    }

    // Wait for them all to finish
    for (int i = 0; i < 4; i++) {
      pthread_join(compare_threads[i], NULL);
    }

  } else {
    printf("Failed dynamic gutenberg\n");
  }

  return 0;
}
