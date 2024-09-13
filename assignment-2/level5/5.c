#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "noaslr.h"

#define MAX_ACCOUNTS (10)
#define MAX_NAME_LEN (32)
#define MAX_PASS_LEN (128)

struct account {
  char name[MAX_NAME_LEN];
  char pass[MAX_PASS_LEN];
  bool used;
};

bool read_int(unsigned int *choice) {
  if (scanf("%d", choice) != 1) {
    fprintf(stderr, "Error: Invalid input\n");
    return false;
  }

  while (getchar() != '\n')
    ;
  return true;
}

bool read_string(const char *prompt, char *dst, size_t max_len) {
  printf(prompt);

  if (fgets(dst, max_len, stdin) != dst) {
    return false;
  }

  char *byte_ptr = strchr(dst, '\n');
  if (byte_ptr) {
    *byte_ptr = '\x00';
  }

  return true;
}

bool read_password(struct account *account) {
  char pass[MAX_PASS_LEN] = {0};

  if (!read_string("Password: ", pass, MAX_PASS_LEN)) {
    return false;
  }
  memcpy(account->pass, pass, MAX_PASS_LEN);
  return true;
}

void add_account(struct account accounts[MAX_ACCOUNTS]) {
  for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
    if (!accounts[i].used) {
      if (!read_string("Name: ", accounts[i].name, MAX_NAME_LEN)) {
        break;
      }

      if (!read_password(&accounts[i])) {
        break;
      }

      accounts[i].used = true;
      printf("Added account in slot %ld\n", i);
      return;
    }
  }

  fprintf(stderr, "Failed to add account\n");
}

void delete_account(struct account accounts[MAX_ACCOUNTS]) {
  unsigned int choice = 0;
  printf("\nAccount ID: ");

  if (!read_int(&choice)) {
    fprintf(stderr, "Error: Invalid input\n");
    return;
  }

  if (choice >= MAX_ACCOUNTS || accounts[choice].used == false) {
    fprintf(stderr, "Error: Invalid choice\n");
    return;
  }

  memset(&accounts[choice], 0, sizeof(struct account));
}

void get_pass(struct account accounts[MAX_ACCOUNTS]) {
  unsigned int choice = 0;

  while (true) {
    printf("\nAccount ID: ");
    if (!read_int(&choice)) {
      fprintf(stderr, "Error: Invalid input\n");
      return;
    }

    if (choice >= MAX_ACCOUNTS || accounts[choice].used == false) {
      fprintf(stderr, "Error: Invalid choice\n");
      return;
    }

    printf("Password for account %s:\n>> ", accounts[choice].name);
    printf(accounts[choice].pass);
    printf("\n");

    printf("\nPress 1 to exit, 2 to retrieve another password\n");
    printf("> ");
    if (!read_int(&choice) || choice != 2) {
      return;
    }
  }
}

void list_accounts(struct account accounts[MAX_ACCOUNTS]) {
  puts("\n ID | name                            | password");
  puts("----+---------------------------------+----------"
       "-------");

  for (size_t i = 0; i < MAX_ACCOUNTS; i++) {
    if (accounts[i].used) {
      printf(" %2ld | %-31s | *************** \n", i, accounts[i].name);
    }
  }
}

void print_menu(void) {
  puts("\n== Super Secure Password Manager ==");
  puts("1. Add a new account");
  puts("2. Delete an account");
  puts("3. Retrieve password");
  puts("4. List accounts");
  puts("5. Exit");
  printf("> ");
}

int main(int argc, char **argv, char **envp) {
  setbuf(stdout, NULL);

  struct account accounts[MAX_ACCOUNTS] = {0};
  unsigned int choice = 0;

  while (1) {
    print_menu();
    if (!read_int(&choice)) {
      break;
    }

    switch (choice) {
    case 1:
      add_account(accounts);
      break;
    case 2:
      delete_account(accounts);
      break;
    case 3:
      get_pass(accounts);
      break;
    case 4:
      list_accounts(accounts);
      break;
    default:
      return 0;
    }
  }

  return 0;
}
