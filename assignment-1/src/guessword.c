
#include <crypt.h>
#include <ctype.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LEN 128
#define MAX_DICT_ITEM_SIZE 256
#define MAX_DICT_ITEMS 2 << 23
#define MAX_SALT_SIZE 32

#define MAX_USER_ITEM_SIZE 256
#define MAX_USERS 20000
#define NR_THREADS 4

// Ugly global, but all the same for every file of users
char salt[MAX_SALT_SIZE];

// Crash program after 24M has been reached
// atomic_int crack_count = 0;
// #define MAX_CRACK_COUNT 24000000

//
// Data structures used to keep track of dictionaries and users
//

typedef struct {
  char plain[MAX_DICT_ITEM_SIZE];
  char hashed[MAX_DICT_ITEM_SIZE];
} dict_item;

typedef struct {
  dict_item *items;
  int size;
} dictionary;

typedef struct {
  char username[MAX_USER_ITEM_SIZE];
  char full_name[MAX_USER_ITEM_SIZE];
  char hash[MAX_USER_ITEM_SIZE];
  int8_t cracked;
} user_item;

typedef struct {
  user_item *items;
  int size;
} user_collection;

// Useful to divide non-overlapping areas of a dict or user section when
// multithreading
typedef struct {
  int start; // inclusive
  int end;   // exclusive
} selection;

//
// Utility functions
//

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

int count_occurence(char c, char *str) {
  int i = 0;
  int count = 0;

  while (str[i] != '\0') {
    if (str[i] == c) {
      count++;
    }

    i++;
  }

  return count;
}

// Reads a plain file from path and then puts all lines in the dictionary,
// returning the amount of lines inserted or -1 on error
int load_dictionary(char *path, dictionary *dict) {
  FILE *dict_file = NULL;
  dict_file = fopen(path, "r");
  if (dict == NULL) {
    printf("Failed to open dictionary from file %s\n", path);
    return -1;
  }

  int i = dict->size;
  char line[MAX_LINE_LEN + 1];

  char *plain = fgets(line, MAX_LINE_LEN, dict_file);
  while (plain != NULL && i < MAX_DICT_ITEMS) {
    line[MAX_LINE_LEN] = '\0';
    clean(line);

    // Copy to correct index
    strcpy(dict->items[i].plain, line);
    dict->size++;

    // Keep loopin
    plain = fgets(line, MAX_LINE_LEN, dict_file);
    i++;
  }

  fclose(dict_file);
  return i;
}

// Don't waste time going over users that are already cracked
void compress_users(user_collection *users) {
  int i = 0;
  int j = users->size;

  while (i < j) {
    // Copy a non-cracked user to this position
    if (users->items[i].cracked != 0) {
      j--;
      // Find a non-cracked user starting from the end
      while (users->items[j].cracked != 0) {
        j--;
      }

      // Copy over
      if (j > i) {
        memcpy(&users->items[i], &users->items[j], sizeof(user_item));
      }
    }
    i++;
  }

  // Ignore duplicates
  users->size = i + 1;
}

typedef struct {
  dictionary *dict;
  selection select;
} hash_dict_args;

void *hash_dict(void *raw_args) {
  hash_dict_args *args = (hash_dict_args *)raw_args;
  if (args->select.end > args->dict->size) {
    args->select.end = args->dict->size;
  }
  // printf("Started hashing from %d to %d\n", args->select.start,
  //        args->select.end);

  int a = 0;
  for (int i = args->select.start; i < args->select.end; i++) {
    struct crypt_data datastore = {0};
    char *res = crypt_r(args->dict->items[i].plain, salt, &datastore);
    strncpy(args->dict->items[i].hashed, res, MAX_LINE_LEN - 1);

    a++;
  }

  return NULL;
}

void wait_threads(pthread_t *threads, int count) {
  for (int i = 0; i < count; i++) {
    pthread_join(threads[i], NULL);
  }
}

// Returns 1 if crack successful
int compare_and_print(user_item *user, char *plain, char *hash) {
  if (strcmp(hash, user->hash) == 0) {
    printf("%s:%s\n", user->username, plain);
    fflush(stdout);
    user->cracked = 1;
    return 1;
  }

  return 0;
}

// Returns 1 if crack successfull
int crack_compare_and_print(user_item *user, char *plain) {
  struct crypt_data datastore[1] = {0};
  char *res = crypt_r(plain, salt, datastore);
  if (res == NULL) {
    return 0;
  }

  return compare_and_print(user, plain, res);
}

//
// Actual cracking functions
//

// Reusable
typedef struct {
  dictionary *dict;
  user_collection *users;
  selection dict_select;
  selection user_select;
} crack_user_args;

int crack_user_basic_username(user_item *user, char *name) {
  int name_len = strlen(name);

  // Try as is
  if (crack_compare_and_print(user, name) == 1) {
    return 1;
  }

  // Try in uppercase
  for (int i = 0; i < name_len; i++) {
    name[i] = toupper(name[i]);
  }
  if (crack_compare_and_print(user, name) == 1) {
    return 1;
  }

  // Try with a single letter lowercased
  for (int i = 0; i < name_len; i++) {
    if (i > 0) {
      name[i - 1] = toupper(name[i - 1]);
    }

    name[i] = tolower(name[i]);
    if (crack_compare_and_print(user, name) == 1) {
      return 1;
    }
  }

  // Try the username in lowercase
  for (int i = 0; i < name_len; i++) {
    name[i] = tolower(name[i]);
  }
  if (crack_compare_and_print(user, name) == 1) {
    return 1;
  }

  // Try the username with a single letter uppercased
  for (int i = 0; i < name_len; i++) {
    if (i > 0) {
      name[i - 1] = tolower(name[i - 1]);
    }

    name[i] = toupper(name[i]);
    if (crack_compare_and_print(user, name) == 1) {
      return 1;
    }
  }

  return 0;
}

// Replace
char *replace_char(char input) {
  switch (input) {
  case 'w':
    return "\\/\\/";
  case 'a':
    return "^";
  case 't':
    return "-|-";
  case 'm':
    return "(v)";
  case 'o':
    return "0";
  case 'h':
    return "#";
  case 'c':
    return "(";
  case 's':
    return "5";
  case 'y':
    return "i'";
  case 'g':
    return "C-";
  case 'b':
    return "8";
  case 'u':
    return "L|";
  case 'p':
    return "|*";
  case 'k':
    return "|(";
  default:
    return NULL;
  }
}

// Permutates a string in place, recursively
// returns 1 is a permutation cracked a password
int permutate(user_item *user, char *str, int start, int level) {
  if (level > 2) {
    return 0;
  }

  for (size_t i = start; i < strlen(str); i++) {
    char replace = str[i];
    char *res = replace_char(replace);
    if (res != NULL) {
      // Create a copy and try with this
      char dup[strlen(str) * 4 + 1];
      strncpy(dup, str, i);
      strcpy(&dup[i], res);
      if (i + 1 < strlen(str)) {
        strcpy(&dup[i + strlen(res)], &str[i + 1]);
      }

      // Try to crack first
      if (crack_compare_and_print(user, dup) == 1) {
        return 1;
      }
      if (permutate(user, dup, 0, level + 1) == 1) {
        return 1;
      }
    }
  }

  return 0;
}

// More advanced variations
int crack_user_advanced_username(user_item *user, char *name) {
  int len = strlen(name);
  int safe_len = len * 4 + 1;
  if (len <= 0) {
    return 0;
  }

  // Buffer for safe experiments
  char name_dup[safe_len];
  strcpy(name_dup, name);

  // Lowercase the name
  for (int i = 0; i < safe_len; i++) {
    name_dup[i] = tolower(name_dup[i]);
  }

  // Try with year suffix
  char name_experiment[safe_len];
  for (int i = 40; i < 100; i++) {
    sprintf(name_experiment, "%s%d", name_dup, i);
    if (crack_compare_and_print(user, name_experiment) == 1) {
      return 1;
    }
    sprintf(name_experiment, "%s%d", name_dup, 1900 + i);
    if (crack_compare_and_print(user, name_experiment) == 1) {
      return 1;
    }
  }

  // Try with zorz and xor suffix
  sprintf(name_experiment, "%sxor", name_dup);
  if (crack_compare_and_print(user, name_experiment) == 1) {
    return 1;
  }
  sprintf(name_experiment, "%szorz", name_dup);
  if (crack_compare_and_print(user, name_experiment) == 1) {
    return 1;
  }

  strcpy(name_experiment, name_dup);

  // Try with permutations
  return permutate(user, name_experiment, 0, 0);
}

// Attempt cracking by advanced username variations in a selection
void *crack_users_advanced_username(void *raw_args) {
  crack_user_args *args = (crack_user_args *)raw_args;
  if (args->user_select.end > args->users->size) {
    args->user_select.end = args->users->size;
  }
  // printf("Starting cracking advanced usernames from %d to %d\n",
  //        args->user_select.start, args->user_select.end);

  for (int i = args->user_select.start; i < args->user_select.end; i++) {
    user_item *user = &args->users->items[i];

    // First try with first name
    int first_name_end = nth_occurrence(' ', user->full_name, 1);
    int first_name_len = first_name_end;
    if (first_name_len > 0) {
      char first_name[first_name_len + 1];
      strncpy(first_name, user->full_name, first_name_len);
      first_name[first_name_len] = '\0';

      if (crack_user_advanced_username(user, first_name) == 1) {
        continue;
      }
    }

    // See how many spaces there are, indicates last name/middle name
    int spaces = count_occurence(' ', user->full_name);

    // Get the last name
    // Then (if uncracked still and middle name present, try with middle name)
    int last_name_start = nth_occurrence(' ', user->full_name, spaces) + 1;
    int last_name_end = strlen(user->full_name);
    int last_name_len = last_name_end - last_name_start;
    if (last_name_len > 0) {
      char last_name[last_name_len + 1];
      strncpy(last_name, &user->full_name[last_name_start], last_name_len);
      last_name[last_name_len] = '\0';

      if (crack_user_advanced_username(user, last_name) == 1) {
        continue;
      }
    }

    // Then (if uncracked still and middle name present, try with middle
    // name)
    if (spaces == 2) {
      int middle_name_start = first_name_end + 1;
      int middle_name_end = nth_occurrence(' ', user->full_name, 2);
      int middle_name_len = middle_name_end - middle_name_start;
      if (middle_name_len > 0) {
        char middle_name[middle_name_len + 1];
        strncpy(middle_name, &user->full_name[middle_name_start],
                middle_name_len);
        middle_name[middle_name_len] = '\0';

        crack_user_advanced_username(user, middle_name);
      }
    }
  }

  return NULL;
}

// Attempt cracking by basic username variations in a selection
void *crack_users_basic_username(void *raw_args) {
  crack_user_args *args = (crack_user_args *)raw_args;
  if (args->user_select.end > args->users->size) {
    args->user_select.end = args->users->size;
  }
  // printf("Starting cracking basic usernames from %d to %d\n",
  //        args->user_select.start, args->user_select.end);

  for (int i = args->user_select.start; i < args->user_select.end; i++) {
    user_item *user = &args->users->items[i];

    // First try with first name
    int first_name_end = nth_occurrence(' ', user->full_name, 1);
    int first_name_len = first_name_end;
    if (first_name_len > 0) {
      char first_name[first_name_len + 1];
      strncpy(first_name, user->full_name, first_name_len);
      first_name[first_name_len] = '\0';

      if (crack_user_basic_username(user, first_name) == 1) {
        continue;
      }
    }

    // See how many spaces there are, indicates last name/middle name
    int spaces = count_occurence(' ', user->full_name);

    // Get the last name
    // Then (if uncracked still and middle name present, try with middle name)
    int last_name_start = nth_occurrence(' ', user->full_name, spaces) + 1;
    int last_name_end = strlen(user->full_name);
    int last_name_len = last_name_end - last_name_start;
    if (last_name_len > 0) {
      char last_name[last_name_len + 1];
      strncpy(last_name, &user->full_name[last_name_start], last_name_len);
      last_name[last_name_len] = '\0';

      if (crack_user_basic_username(user, last_name) == 1) {
        continue;
      }
    }

    // Then (if uncracked still and middle name present, try with middle
    // name)
    if (spaces == 2) {
      int middle_name_start = first_name_end + 1;
      int middle_name_end = nth_occurrence(' ', user->full_name, 2);
      int middle_name_len = middle_name_end - middle_name_start;
      if (middle_name_len > 0) {
        char middle_name[middle_name_len + 1];
        strncpy(middle_name, &user->full_name[middle_name_start],
                middle_name_len);
        middle_name[middle_name_len] = '\0';

        crack_user_basic_username(user, middle_name);
      }
    }
  }

  return NULL;
}

// Attempt cracking by comparing to precomputed hashes
void *crack_users_with_dict(void *raw_args) {
  crack_user_args *args = (crack_user_args *)raw_args;
  if (args->user_select.end > args->users->size) {
    args->user_select.end = args->users->size;
  }
  if (args->dict_select.end > args->dict->size) {
    args->dict_select.end = args->dict->size;
  }
  // printf("Starting cracking usernames with dict from %d to %d in dict from %d
  // "
  //        "to %d\n",
  //        args->user_select.start, args->user_select.end,
  //        args->dict_select.start, args->dict_select.end);

  for (int i = args->user_select.start; i < args->user_select.end; i++) {
    for (int j = args->dict_select.start; j < args->dict_select.end; j++) {
      int res =
          compare_and_print(&args->users->items[i], args->dict->items[j].plain,
                            args->dict->items[j].hashed);
      if (res == 1) {
        break;
      }
    }
  }

  return NULL;
}

// Generate dynamic hashes for 24-letterwords
typedef struct {
  dictionary *dict;
  selection read_11;
  selection read_13;
  selection write;
} generate_hash_24_args;

void *generate_hash_24_letterwords(void *raw_args) {
  generate_hash_24_args *args = (generate_hash_24_args *)raw_args;
  if (args->read_11.end > args->dict->size) {
    args->read_11.end = args->dict->size;
  }
  if (args->read_13.end > args->dict->size) {
    args->read_13.end = args->dict->size;
  }

  // printf("Generating 24-letterwords from 11words (%d to %d) and 13words (%d
  // to "
  //        "%d)\n",
  //        args->read_11.start, args->read_11.end, args->read_13.start,
  //        args->read_13.end);

  int write_offset = 0;
  char word[25];

  for (int i = args->read_11.start; i < args->read_11.end; i++) {
    for (int j = args->read_13.start; j < args->read_13.end; j++) {
      char *word_11 = args->dict->items[i].plain;
      char *word_13 = args->dict->items[j].plain;

      // Something is seriously wrong..
      if (strlen(word_11) != 11) {
        // printf("Expected 11-letter string but received %lu '%s' for thread "
        //        "that started from %d to %d\n",
        //        strlen(word_11), word_11, args->read_11.start,
        //        args->read_11.end);
        write_offset++;
        return NULL;
      } else {
        strncpy(word, word_11, 11);
      }
      if (strlen(word_13) != 13) {
        // printf("Expected 13-letter string but received %lu '%s'\n",
        //        strlen(word_13), word_13);
        write_offset++;
        continue;
      } else {
        strncpy(&word[11], word_13, 13);
      }

      // if (write_offset % 10000 == 0) {
      // printf("Generated %d 24-letterwords from %d total (%fpct)\n",
      //        write_offset,
      //        (args->read_11.end - args->read_11.start) *
      //            (args->read_13.end - args->read_13.start),
      //        100.0 * write_offset /
      //            (float)((args->read_11.end - args->read_11.start) *
      //                    (args->read_13.end - args->read_13.start)));
      // }

      word[24] = '\0';

      struct crypt_data datastore[1] = {0};
      char *res = crypt_r(word, salt, datastore);
      if (res != NULL) {
        strcpy(args->dict->items[args->write.start + write_offset].hashed, res);
        strcpy(args->dict->items[args->write.start + write_offset].plain, word);
      }
      write_offset++;
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Incorrect number of arguments. Provide a passwd_path and "
           "shadow_path\n");
    return 1;
  }

  // Store all users
  user_collection users = {.items = calloc(MAX_USERS, sizeof(user_item)),
                           .size = 0};
  if (users.items == NULL) {
    printf("Could not allocate memory to store users\n");
    return 1;
  }

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

  // Buffers to store each line in
  char user_pswd[MAX_LINE_LEN + 1];
  char user_shdw[MAX_LINE_LEN + 1];

  // Read all lines, one by one
  char *pswd_res = fgets(user_pswd, MAX_LINE_LEN, pswd);
  char *shdw_res = fgets(user_shdw, MAX_LINE_LEN, shdw);

  // Take the salt from the first line (all salts are the same per file)
  int salt_start = nth_occurrence('$', shdw_res, 1);
  int salt_end = nth_occurrence('$', shdw_res, 3) + 1;
  int salt_len = salt_end - salt_start;
  strncpy(salt, &shdw_res[salt_start], salt_len);
  salt[salt_len] = '\0';

  // Continue reading lines
  while (pswd_res != NULL && shdw_res != NULL && users.size < MAX_USERS) {
    // The hash from the shadow line
    int hash_start = nth_occurrence('$', user_shdw, 1);
    int hash_end = nth_occurrence(':', user_shdw, 2);
    int hash_len = hash_end - hash_start;
    if (hash_len > 0) {
      strncpy(users.items[users.size].hash, &user_shdw[hash_start], hash_len);
    }

    // The user id from the shadow line (always at the start)
    int username_end = nth_occurrence(':', user_shdw, 1);
    int username_length = username_end;
    if (username_length > 0) {
      strncpy(users.items[users.size].username, user_shdw, username_length);
    }

    // The full name from the passwd line
    int full_name_start = nth_occurrence(':', user_pswd, 4) + 1;
    int full_name_end = nth_occurrence(',', user_pswd, 1);
    int full_name_len = full_name_end - full_name_start;
    if (full_name_len > 0) {
      strncpy(users.items[users.size].full_name, &user_pswd[full_name_start],
              full_name_len);
    }

    // Keep loopin, putting everything in memory
    pswd_res = fgets(user_pswd, MAX_LINE_LEN, pswd);
    shdw_res = fgets(user_shdw, MAX_LINE_LEN, shdw);
    users.size++;
  }

  // We don't need the files anymore
  fclose(pswd);
  fclose(shdw);

  //
  // Crack part 1: low-hanging fruit, basic username variations
  //

  // printf("Attempting to run basic variations\n");

  {
    // Run the username variations in parallel
    pthread_t crack_basic_threads[NR_THREADS];
    crack_user_args crack_basic_thread_args[NR_THREADS];
    for (int i = 0; i < NR_THREADS; i++) {
      int frac = (users.size / NR_THREADS) + 1;
      selection s = {
          .start = i * frac,
          .end = (i + 1) * frac,
      };

      crack_basic_thread_args[i].users = &users;
      crack_basic_thread_args[i].user_select = s;
      pthread_create(&crack_basic_threads[i], NULL, crack_users_basic_username,
                     &crack_basic_thread_args[i]);
    }
    wait_threads(crack_basic_threads, NR_THREADS);
  }
  compress_users(&users);

  //
  // Crack part 2: use precomputer non-dynamic dictionaries with basic
  // variations
  //

  // Store all dict words
  dictionary dict = {.items = calloc(MAX_DICT_ITEMS, sizeof(dict_item)),
                     .size = 0};
  if (dict.items == NULL) {
    printf("Could not allocate memory to load dictionary\n");
    free(users.items);
    return 1;
  }

  // Load the binaries (no hashing yet)
  int res = load_dictionary("./unique-gutenberg.txt", &dict);
  if (res > 0) {
    res = load_dictionary("./unique-top250.txt", &dict);
  }
  if (res > 0) {
    res = load_dictionary("./variations-gutenberg.txt", &dict);
  }
  if (res > 0) {
    res = load_dictionary("./variations-top250.txt", &dict);
  }
  if (res > 0) {
    res = load_dictionary("./unique-plain.txt", &dict);
  }
  // ..

  if (res <= 0) {
    printf("Failed to load dictionaries... %d\n", res);
    return 1;
  }

  {
    // Hash the binaries in parallel
    pthread_t hash_thread[NR_THREADS];
    hash_dict_args hash_thread_args[NR_THREADS];
    for (int i = 0; i < NR_THREADS; i++) {
      int frac = (dict.size / NR_THREADS) + 1;
      selection s = {
          .start = i * frac,
          .end = (i + 1) * frac,
      };

      hash_thread_args[i].dict = &dict;
      hash_thread_args[i].select = s;
      pthread_create(&hash_thread[i], NULL, hash_dict, &hash_thread_args[i]);
    }
    wait_threads(hash_thread, NR_THREADS);
  }
  {
    // Run the comparisions in parallel
    pthread_t crack_comp_threads[NR_THREADS];
    crack_user_args crack_comp_args[NR_THREADS];
    for (int i = 0; i < NR_THREADS; i++) {
      int frac = (users.size / NR_THREADS) + 1;
      selection s = {
          .start = i * frac,
          .end = (i + 1) * frac,
      };
      selection d = {.start = 0, .end = dict.size};

      crack_comp_args[i].users = &users;
      crack_comp_args[i].user_select = s;
      crack_comp_args[i].dict = &dict;
      crack_comp_args[i].dict_select = d;

      pthread_create(&crack_comp_threads[i], NULL, crack_users_with_dict,
                     &crack_comp_args[i]);
    }
    wait_threads(crack_comp_threads, NR_THREADS);
  }
  compress_users(&users);

  //
  // Crack part 3: try all 24 (11 + 13)-letterwords
  //

  selection words_11 = {.start = -1, .end = -1};
  selection words_13 = {.start = -1, .end = -1};

  // gutenberg unique is loaded first and is sorted in increasing length, so
  // take the sections where 11 and 13 letter words start
  int i = 0;
  while (words_13.end < 0 && i < dict.size) {
    int len = strlen(dict.items[i].plain);
    if (len == 11 && words_11.start < 0) {
      words_11.start = i;
    } else if (len == 12 && words_11.end < 0) {
      words_11.end = i;
    } else if (len == 13 && words_13.start < 0) {
      words_13.start = i;
    } else if (len == 14 && words_13.end < 0) {
      words_13.end = i;
    }

    i++;
  }

  selection words_24 = {.start = dict.size, .end = dict.size};
  if (words_11.start > 0 && words_11.end > words_11.start &&
      words_13.start > 0 && words_13.end > words_13.start) {
    // Run the generations in parallel
    pthread_t generate_24_threads[NR_THREADS];
    generate_hash_24_args generate_24_thread_args[NR_THREADS];
    for (int i = 0; i < NR_THREADS; i++) {
      // Divide all 11-words that need to be enumerated in equal parts
      int word_11_count = (words_11.end - words_11.start);
      int frac = (word_11_count / NR_THREADS) + 1;
      selection read_11 = {
          .start = words_11.start + (i * frac),
          .end = words_11.start + ((i + 1) * frac),
      };
      if (i == NR_THREADS - 1) {
        read_11.end = words_11.end;
      }

      int word_13_count = words_13.end - words_13.start;
      selection write = {.start = dict.size + (i * frac * word_13_count),
                         .end = dict.size + ((1 + i) * frac * word_13_count)};

      generate_24_thread_args[i].dict = &dict;
      generate_24_thread_args[i].read_11 = read_11;
      generate_24_thread_args[i].read_13 = words_13;
      generate_24_thread_args[i].write = write;

      pthread_create(&generate_24_threads[i], NULL,
                     generate_hash_24_letterwords, &generate_24_thread_args[i]);
    }
    wait_threads(generate_24_threads, NR_THREADS);

    dict.size +=
        (words_11.end - words_11.start) * (words_13.end - words_13.start);
    words_24.end = dict.size;

    // Try the 24-letterwords with the remaining users
    compress_users(&users);
    {
      // Run the comparisions in parallel
      pthread_t crack_comp_threads[NR_THREADS];
      crack_user_args crack_comp_args[NR_THREADS];
      for (int i = 0; i < NR_THREADS; i++) {
        int frac = (users.size / NR_THREADS) + 1;
        selection s = {
            .start = i * frac,
            .end = (i + 1) * frac,
        };
        selection d = {.start = words_24.start, .end = dict.size};

        crack_comp_args[i].users = &users;
        crack_comp_args[i].user_select = s;
        crack_comp_args[i].dict = &dict;
        crack_comp_args[i].dict_select = d;

        pthread_create(&crack_comp_threads[i], NULL, crack_users_with_dict,
                       &crack_comp_args[i]);
      }
      wait_threads(crack_comp_threads, NR_THREADS);
    }
  }

  //
  // Crack part 4: try more advanced username variations (with replacements,
  // years of birth)
  //

  {
    // Run the username variations in parallel
    pthread_t crack_basic_threads[NR_THREADS];
    crack_user_args crack_basic_thread_args[NR_THREADS];
    for (int i = 0; i < NR_THREADS; i++) {
      int frac = (users.size / NR_THREADS) + 1;
      selection s = {
          .start = i * frac,
          .end = (i + 1) * frac,
      };

      crack_basic_thread_args[i].users = &users;
      crack_basic_thread_args[i].user_select = s;
      pthread_create(&crack_basic_threads[i], NULL,
                     crack_users_advanced_username,
                     &crack_basic_thread_args[i]);
    }
    wait_threads(crack_basic_threads, NR_THREADS);
  }

  free(users.items);
  free(dict.items);

  return 0;
}
