
#include <crypt.h>
#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #define LINE_LEN 512
// #define DICT_MAX_LINE_LEN 128
// #define DICT_LINE_COUNT (2 << 23)
// #define USERS_LINE_LEN 128
// #define USERS_LINE_COUNT 10000

#define MAX_LINE_LEN 128

#define MAX_DICT_ITEM_SIZE 256
#define MAX_DICT_ITEMS 2 << 23
#define MAX_SALT_SIZE 32

#define MAX_USER_ITEM_SIZE 256
#define MAX_USERS 20000
#define NR_THREADS 4

// Ugly global, but all the same for every file of users
char salt[MAX_SALT_SIZE];

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
  // the abc123 combo
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
  printf("Started hashing from %d to %d\n", args->select.start,
         args->select.end);

  int a = 0;
  for (int i = args->select.start; i < args->select.end; i++) {
    struct crypt_data datastore = {0};
    char *res = crypt_r(args->dict->items[i].plain, salt, &datastore);
    strncpy(args->dict->items[i].hashed, res, MAX_LINE_LEN - 1);

    if (a % 100000 == 0) {
      printf("Hashed %d words\n", a);
    }
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

// // At a memory location with plain lines loaded, try to hash each line and
// copy
// // the hashed result to the "hashed" memory location
// typedef struct {
//   char *plain;
//   char *salt;
//   char *hashed;
//   int start;
//   int end;
// } hash_dict_args;

// void *hash_dictionary(void *raw_args) {
//   hash_dict_args *args = (hash_dict_args *)raw_args;
//   int i = args->start;
//   int a = 0;

//   printf("Started hashing from %d to %d\n", args->start, args->end);

//   while (i < args->end) {
//     struct crypt_data datastore[1] = {0};
//     char *res =
//         crypt_r(&args->plain[DICT_MAX_LINE_LEN * i], args->salt, datastore);
//     strncpy(&args->hashed[DICT_MAX_LINE_LEN * i], res, DICT_MAX_LINE_LEN -
//     1);

//     if (a % 100000 == 0) {
//       printf("Hashed %d words\n", a);
//     }

//     // Keep loopin
//     i++;
//     a++;
//   }

//   return NULL;
// }

// // Hashes, checks if a hash matches, prints the required user:plain combo and
// // returns 1 if it was a match
// int try_hash(char *plain, char *salt, char *hash, char *user) {
//   struct crypt_data datastore[1] = {0};
//   char *res = crypt_r(plain, salt, datastore);

//   if (res != NULL && strncmp(res, hash, 256) == 0) {
//     printf("%s:%s\n", user, plain);
//     fflush(stdout);
//     return 1;
//   }

//   return 0;
// }

// // Returns 1 if a username variation was found matching
// int try_username_variations(char *src, char *salt, char *hash, char *user) {
//   int len = strlen(src) + 64; // leave room for variations
//   char dup[len + 1];
//   if (len > 0) {
//     strcpy(dup, src);
//     dup[len] = '\0';
//   }

//   // Try as is
//   if (try_hash(dup, salt, hash, user) == 1) {
//     return 1;
//   }

//   // Try the username in uppercase
//   int i = 0;
//   while (dup[i] != '\0') {
//     dup[i] = toupper(dup[i]);
//     // Keep loopin
//     i++;
//   }
//   if (try_hash(dup, salt, hash, user) == 1) {
//     return 1;
//   }

//   // Try the username with a single letter lowercased
//   i = 0;
//   while (dup[i] != '\0') {
//     if (i > 0) {
//       dup[i - 1] = toupper(dup[i - 1]);
//     }

//     dup[i] = tolower(dup[i]);
//     if (try_hash(dup, salt, hash, user) == 1) {
//       return 1;
//     }

//     // Keep loopin
//     i++;
//   }

//   // Try the username in lowercase
//   i = 0;
//   while (dup[i] != '\0') {
//     dup[i] = tolower(dup[i]);
//     // Keep loopin
//     i++;
//   }
//   if (try_hash(dup, salt, hash, user) == 1) {
//     return 1;
//   }

//   // Try the username with a single letter uppercased
//   i = 0;
//   while (dup[i] != '\0') {
//     if (i > 0) {
//       dup[i - 1] = tolower(dup[i - 1]);
//     }

//     dup[i] = toupper(dup[i]);
//     if (try_hash(dup, salt, hash, user) == 1) {
//       return 1;
//     }
//     // Keep loopin
//     i++;
//   }

//   return 0;
// }

// void crack_and_print_single(char *user, char *hash, int hash_len, int
// dict_size,
//                             char *dict_hashed, char *dict_plain,
//                             char *user_passwd_line, char *salt) {
//   // Try all pre-computed hashes from the wordlist first
//   for (int j = 0; j < dict_size; j++) {
//     char *word = &dict_hashed[DICT_MAX_LINE_LEN * j];
//     if (strncmp(word, hash, hash_len) == 0) {
//       printf("%s:%s\n", user, &dict_plain[DICT_MAX_LINE_LEN * j]);
//       fflush(stdout);
//       return;
//     }
//   }

//   // Get the full name of a user
//   int full_name_start = nth_occurrence(':', user_passwd_line, 4) + 1;
//   int full_name_end = nth_occurrence(',', user_passwd_line, 1);
//   int full_name_len = full_name_end - full_name_start;

//   char full_name[full_name_len];
//   if (full_name_len > 0) {
//     strncpy(full_name, &user_passwd_line[full_name_start], full_name_len);
//     full_name[full_name_len] = '\0';

//     int first_name_end = nth_occurrence(' ', full_name, 1);
//     int first_name_len = first_name_end;

//     char first_name[first_name_len + 1];
//     if (first_name_len > 0) {
//       strncpy(first_name, full_name, first_name_len);
//       first_name[first_name_len] = '\0';

//       int match = try_username_variations(first_name, salt, hash, user);
//       if (match == 1) {
//         return;
//       }
//     }
//   }
// }

// typedef struct {
//   char *users_shadow;
//   char *users_passwd;
//   char *dict_hashed;
//   char *dict_plain;
//   char *salt;
//   int dict_size;
//   int start;
//   int end;
// } crack_print_args;

// // Prints when it found a match in the pre-computed passwords and
// // creates variations with usernames to try and hash
// void *crack_and_print(void *raw_args) {
//   crack_print_args *args = (crack_print_args *)raw_args;

//   for (int i = args->start; i < args->end; i++) {
//     // The hash that we need to compare to (as in the shadow file line)
//     int hash_start =
//         nth_occurrence('$', &args->users_shadow[USERS_LINE_LEN * i], 1);
//     int hash_end =
//         nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 2);
//     int hash_len = hash_end - hash_start;

//     char hash[hash_len + 1];
//     if (hash_len > 0) {
//       strncpy(hash, &args->users_shadow[USERS_LINE_LEN * i + hash_start],
//               hash_len);
//       hash[hash_len] = '\0';
//     }

//     // The user (user id) is always at the start of the line
//     int user_end =
//         nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 1);
//     int user_len = user_end;

//     char user[user_len + 1];
//     if (user_len > 0) {
//       strncpy(user, &args->users_shadow[USERS_LINE_LEN * i], user_len);
//       user[user_len] = '\0';
//     }

//     crack_and_print_single(user, hash, hash_len, args->dict_size,
//                            args->dict_hashed, args->dict_plain,
//                            &args->users_passwd[USERS_LINE_LEN * i],
//                            args->salt);
//   }

//   return NULL;
// }

// typedef struct {
//   // The dictionaries to read/write
//   char *dict_plain;
//   char *dict_hashed;
//   // For placing generated words IN the dict
//   int dict_start;
//   int dict_end;
//   // For reading plain words FROM the dict
//   // (combine 11-letterwords with 13-letterwords)
//   int plain_11_start;
//   int plain_11_end;
//   int plain_13_start;
//   int plain_13_end;
//   // Always needed
//   char *salt;
// } generate_dict_args;

// // Cannot be preloaded due to size constraints
// void *generate_24_letter_dictionary(void *raw_args) {
//   generate_dict_args *args = (generate_dict_args *)raw_args;

//   char plain_word[25];
//   int curr_index = 0;

//   // Pick an 11 and 13-letteword, merge them and store the hashed result
//   for (int i = args->plain_11_start; i < args->plain_11_end; i++) {
//     for (int j = args->plain_13_start; j < args->plain_13_end; j++) {
//       // Merge the words
//       strncpy(plain_word, &args->dict_plain[DICT_MAX_LINE_LEN * i], 11);
//       strncpy(&plain_word[11], &args->dict_plain[DICT_MAX_LINE_LEN * j], 13);
//       plain_word[24] = '\0';

//       // Convert to lowercase
//       // todo: check why uppercase is selected
//       for (int k = 0; k < 25; k++) {
//         plain_word[k] = tolower(plain_word[k]);
//       }

//       int dict_index = args->dict_start + curr_index;

//       struct crypt_data datastore[1] = {0};
//       char *res = crypt_r(plain_word, args->salt, datastore);
//       // copy hash
//       strncpy(&args->dict_hashed[DICT_MAX_LINE_LEN * dict_index], res,
//               DICT_MAX_LINE_LEN - 1);
//       // copy plain
//       strncpy(&args->dict_plain[DICT_MAX_LINE_LEN * dict_index], plain_word,
//               25);

//       curr_index++;
//     }
//   }

//   return NULL;
// }

// typedef struct {
//   char *users_shadow;
//   int users_start;
//   int users_end;
//   // The dictionary that can be accessed
//   char *dict_hashed;
//   char *dict_plain;
//   int dict_start;
//   int dict_end;
// } compare_and_print_args;

// void *compare_and_print(void *raw_args) {
//   compare_and_print_args *args = (compare_and_print_args *)raw_args;

//   for (int i = args->users_start; i < args->users_end; i++) {
//     // The hash that we need to compare to (as in the shadow file line)
//     int hash_start =
//         nth_occurrence('$', &args->users_shadow[USERS_LINE_LEN * i], 1);
//     int hash_end =
//         nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 2);
//     int hash_len = hash_end - hash_start;

//     char hash[hash_len + 1];
//     if (hash_len > 0) {
//       strncpy(hash, &args->users_shadow[USERS_LINE_LEN * i + hash_start],
//               hash_len);
//       hash[hash_len] = '\0';
//     }

//     // The user (user id) is always at the start of the line
//     int user_end =
//         nth_occurrence(':', &args->users_shadow[USERS_LINE_LEN * i], 1);
//     int user_len = user_end;

//     char user[user_len + 1];
//     if (user_len > 0) {
//       strncpy(user, &args->users_shadow[USERS_LINE_LEN * i], user_len);
//       user[user_len] = '\0';
//     }

//     compare_and_print_single(user, hash, args->dict_hashed, args->dict_plain,
//                              args->dict_start, args->dict_end);
//   }

//   return NULL;
// }

//
// Actual cracking functions
//

// Reusable for many cracking functions
typedef struct {
  dictionary *dict;
  user_collection *users;
  selection dict_select;
  selection user_select;
} crack_user_args;

void crack_user_basic_username(user_item *user) {
  // Use the first name of the user for variations
  int first_name_end = nth_occurrence(' ', user->full_name, 1);
  int first_name_len = first_name_end;
  if (first_name_len <= 0) {
    return;
  }

  char first_name[first_name_len + 1];
  strncpy(first_name, user->full_name, first_name_len);
  first_name[first_name_len] = '\0';

  // Try as is
  if (crack_compare_and_print(user, first_name) == 1) {
    return;
  }

  // Try in uppercase
  for (int i = 0; i < first_name_len; i++) {
    first_name[i] = toupper(first_name[i]);
  }
  if (crack_compare_and_print(user, first_name) == 1) {
    return;
  }

  // Try with a single letter lowercased
  for (int i = 0; i < first_name_len; i++) {
    if (i > 0) {
      first_name[i - 1] = toupper(first_name[i - 1]);
    }

    first_name[i] = tolower(first_name[i]);
    if (crack_compare_and_print(user, first_name) == 1) {
      return;
    }
  }

  // Try the username in lowercase
  for (int i = 0; i < first_name_len; i++) {
    first_name[i] = tolower(first_name[i]);
  }
  if (crack_compare_and_print(user, first_name) == 1) {
    return;
  }

  // Try the username with a single letter uppercased
  for (int i = 0; i < first_name_len; i++) {
    if (i > 0) {
      first_name[i - 1] = tolower(first_name[i - 1]);
    }

    first_name[i] = toupper(first_name[i]);
    if (crack_compare_and_print(user, first_name) == 1) {
      return;
    }
  }
}

// Attempt cracking by basic username variations in a selection
void *crack_users_basic_username(void *raw_args) {
  crack_user_args *args = (crack_user_args *)raw_args;
  if (args->user_select.end > args->users->size) {
    args->user_select.end = args->users->size;
  }
  printf("Starting cracking basic usernames from %d to %d\n",
         args->user_select.start, args->user_select.end);

  for (int i = args->user_select.start; i < args->user_select.end; i++) {
    crack_user_basic_username(&args->users->items[i]);
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
  printf("Starting cracking usernames with dict from %d to %d in dict from %d "
         "to %d\n",
         args->user_select.start, args->user_select.end,
         args->dict_select.start, args->dict_select.end);

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

  printf("Generating 24-letterwords from 11words (%d to %d) and 13words (%d to "
         "%d)\n",
         args->read_11.start, args->read_11.end, args->read_13.start,
         args->read_13.end);

  int write_offset = 0;
  char word[25];

  for (int i = args->read_11.start; i < args->read_11.end; i++) {
    for (int j = args->read_13.start; j < args->read_13.end; j++) {
      char *word_11 = args->dict->items[i].plain;
      char *word_13 = args->dict->items[j].plain;

      // Something is seriously wrong..
      if (strlen(word_11) != 11) {
        printf("Expected 11-letter string but received %lu '%s' for thread "
               "that started from %d to %d\n",
               strlen(word_11), word_11, args->read_11.start,
               args->read_11.end);
        write_offset++;
        return NULL;
      } else {
        strncpy(word, word_11, 11);
      }
      if (strlen(word_13) != 13) {
        printf("Expected 13-letter string but received %lu '%s'\n",
               strlen(word_13), word_13);
        write_offset++;
        continue;
      } else {
        strncpy(&word[11], word_13, 13);
      }

      if (write_offset % 10000 == 0) {
        printf("Generated %d 24-letterwords from %d total (%fpct)\n",
               write_offset,
               (args->read_11.end - args->read_11.start) *
                   (args->read_13.end - args->read_13.start),
               100.0 * write_offset /
                   (float)((args->read_11.end - args->read_11.start) *
                           (args->read_13.end - args->read_13.start)));
      }

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

  printf("Attempting to run basic variations\n");

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
  printf("All basic variations complete\n");
  int size_before = users.size;
  compress_users(&users);
  int size_now = users.size;
  int total_found = size_before - size_now;
  printf("(cracked %d with this)\n", total_found);

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
    printf("All hashing complete\n");
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
  size_before = users.size;
  compress_users(&users);
  size_now = users.size;
  total_found = size_before - size_now;
  printf("(cracked %d with this)\n", total_found);

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

  printf("Found first11: %d-%d and first13: %d-%d '%s'-'%s' '%s'-'%s'\n",
         words_11.start, words_11.end, words_13.start, words_13.end,
         dict.items[words_11.start].plain, dict.items[words_11.end].plain,
         dict.items[words_13.start].plain, dict.items[words_13.end].plain);
  // return 2;

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
    printf("Generated and hashed %d 24-letterwords\n",
           words_24.end - words_24.start);

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

  // Split up the plain word dictionary into 4, so that it can be parallelized
  // int frac = dict_curr / 4;

  // // Create 4 threads and wait for them to finish
  // pthread_t dict_thread[4];
  // hash_dict_args args[4];

  // args[0].start = 0;
  // args[0].end = frac;
  // args[1].start = frac;
  // args[1].end = frac * 2;
  // args[2].start = frac * 2;
  // args[2].end = frac * 3;
  // args[3].start = frac * 3;
  // args[3].end = dict_curr;

  // for (int i = 0; i < 4; i++) {
  //   args[i].plain = dict_plain;
  //   args[i].hashed = dict_hashed;
  //   args[i].salt = salt;
  //   pthread_create(&dict_thread[i], NULL, hash_dictionary, &args[i]);
  // }

  // // Wait for all threads to finish
  // for (int i = 0; i < 4; i++) {
  //   pthread_join(dict_thread[i], NULL);
  // }

  // printf("Done hashing!\n");

  // // Split up the users directory into 4, so that it can be parallelized
  // frac = user_curr / 4;

  // // Create 4 threads and wait for them to finish
  // pthread_t user_thread[4];
  // crack_print_args user_args[4];

  // user_args[0].start = 0;
  // user_args[0].end = frac;
  // user_args[1].start = frac;
  // user_args[1].end = frac * 2;
  // user_args[2].start = frac * 2;
  // user_args[2].end = frac * 3;
  // user_args[3].start = frac * 3;
  // user_args[3].end = user_curr;

  // for (int i = 0; i < 4; i++) {
  //   user_args[i].users_passwd = user_pswd;
  //   user_args[i].users_shadow = user_shdw;
  //   user_args[i].dict_hashed = dict_hashed;
  //   user_args[i].dict_plain = dict_plain;
  //   user_args[i].dict_size = dict_curr;
  //   user_args[i].salt = salt;
  //   pthread_create(&user_thread[i], NULL, crack_and_print, &user_args[i]);
  // }

  // // Wait for all threads to finish
  // for (int i = 0; i < 4; i++) {
  //   pthread_join(user_thread[i], NULL);
  // }

  // //
  // // Complex: try the 24-letterword variations (should be around ~56%)
  // //

  // // Find where the 11 and 13 letterwords sections start in the gutenberg
  // unique
  // // dict
  // int gutenberg_11_start = -1;
  // int gutenberg_11_end = -1;
  // int gutenberg_13_start = -1;
  // int gutenberg_13_end = -1;

  // for (int i = 0; i < unique_gutenberg_end; i++) {
  //   int len = strlen(&dict_plain[DICT_MAX_LINE_LEN * i]);
  //   if (len == 11) {
  //     gutenberg_11_start = i;
  //   } else if (len == 12) {
  //     gutenberg_11_end = i;
  //   } else if (len == 13) {
  //     gutenberg_13_start = i;
  //   } else if (len == 14) {
  //     gutenberg_13_end = i;
  //   }
  // }

  // if (gutenberg_11_start > 0 && gutenberg_11_end > 0 &&
  //     gutenberg_13_start > 0 && gutenberg_13_end > 0) {
  //   int word_11_count = gutenberg_11_end - gutenberg_11_start;
  //   int word_13_count = gutenberg_13_end - gutenberg_13_start;

  //   // Split the 11 letters up in 4 equal partitions
  //   frac = (word_11_count / 4) + 1;

  //   // Create 4 threads and wait for them to finish
  //   pthread_t generate_thread[4];
  //   generate_dict_args generate_args[4];

  //   // Each thread gets a block of memory from the total dict that they can
  //   // insert into, based on how many variations there are
  //   generate_args[0].plain_11_start = 0;
  //   generate_args[0].plain_11_end = frac;
  //   generate_args[0].dict_start = dict_curr;
  //   generate_args[0].dict_end = frac * word_13_count;

  //   generate_args[1].plain_11_start = frac;
  //   generate_args[1].plain_11_end = frac * 2;
  //   generate_args[1].dict_start = frac * word_13_count;
  //   generate_args[1].dict_end = frac * 2 * word_13_count;

  //   generate_args[2].plain_11_start = frac * 2;
  //   generate_args[2].plain_11_end = frac * 3;
  //   generate_args[2].dict_start = frac * 2 * word_13_count;
  //   generate_args[2].dict_end = frac * 3 * word_13_count;

  //   generate_args[3].plain_11_start = frac * 3;
  //   generate_args[3].plain_11_end = frac * 4;
  //   generate_args[3].dict_start = frac * 3 * word_13_count;
  //   generate_args[3].dict_end = frac * 4 * word_13_count;

  //   for (int i = 0; i < 4; i++) {
  //     generate_args[i].dict_plain = dict_plain;
  //     generate_args[i].dict_hashed = dict_hashed;
  //     generate_args[i].salt = salt;

  //     pthread_create(&generate_thread[i], NULL,
  //     generate_24_letter_dictionary,
  //                    &generate_args[i]);
  //   }

  //   // Wait for all threads to finish
  //   for (int i = 0; i < 4; i++) {
  //     pthread_join(generate_thread[i], NULL);
  //   }

  //   int dict_24_word_start = dict_curr;
  //   dict_curr += word_11_count * word_13_count;

  //   printf("Generated all %d 24-letter words!\n",
  //          dict_curr - dict_24_word_start);

  //   // Now create 4 threads for cracking again
  //   pthread_t compare_threads[4];
  //   compare_and_print_args compare_args[4];

  //   // Each thread checks 1/4th of the users
  //   frac = user_curr / 4;
  //   compare_args[0].users_start = 0;
  //   compare_args[0].users_end = frac;
  //   compare_args[1].users_start = frac;
  //   compare_args[1].users_end = frac * 2;
  //   compare_args[2].users_start = frac * 2;
  //   compare_args[2].users_end = frac * 3;
  //   compare_args[3].users_start = frac * 3;
  //   compare_args[3].users_end = user_curr;

  //   for (int i = 0; i < 4; i++) {
  //     compare_args[i].dict_hashed = dict_hashed;
  //     compare_args[i].dict_plain = dict_plain;
  //     compare_args[i].dict_start = dict_24_word_start;
  //     compare_args[i].dict_end = dict_curr;
  //     compare_args[i].users_shadow = user_shdw;

  //     pthread_create(&compare_threads[i], NULL, compare_and_print,
  //                    &compare_args[i]);
  //   }

  //   // Wait for them all to finish
  //   for (int i = 0; i < 4; i++) {
  //     pthread_join(compare_threads[i], NULL);
  //   }

  // } else {
  //   printf("Failed dynamic gutenberg\n");
  // }

  return 0;
}
