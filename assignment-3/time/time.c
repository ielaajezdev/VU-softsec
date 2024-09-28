#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum command {
  exit_program,
  get_time,
  set_timezone,
  admin_console,
  command_num,
};

static char const *const command_description[command_num] = {
    [exit_program] = "Exit this program",
    [get_time] = "Return current time",
    [set_timezone] = "Set timezone for current time",
    [admin_console] = "Open admin console",
};

static void print_banner(void) {
  printf("Welcome to the time service!\n\n");
  printf("This service tells you what time it is, based on the appropriate "
         "timezone");
  printf("\n");
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

#define USERNAME_BUFFER_SIZE 0x10
#define PASSWORD_BUFFER_SIZE 0x10

static int authenticate_admin() {
  printf("Insert username > ");
  char username[USERNAME_BUFFER_SIZE];
  if (get_string_from_user(sizeof(username), username) < 0) {
    puts("Could not read username");
    return -1;
  }

  printf("Insert password > ");
  char password[PASSWORD_BUFFER_SIZE];
  if (get_string_from_user(sizeof(password), password) < 0) {
    puts("Could not read password");
    return -1;
  }

  if (!strcmp(username, "username") && !strcmp(username, "password")) { // XXX
    return 0;
  }

  return -1;
}

static int get_time_handler(void) {
  time_t current_time = time(NULL);
  if (current_time == -1) {
    perror("time");
    return -1;
  }

  char *time_str = ctime(&current_time);
  if (!time_str) {
    perror("ctime");
    return -1;
  }

  printf("Local time: %s", time_str);

  return 0;
}

#define TIMEZONE_BUFFER_SIZE 0x100

static int set_timezone_handler(void) {
  printf("Insert new timezone > ");
  char timezone[TIMEZONE_BUFFER_SIZE];
  if (get_string_from_user(sizeof(timezone), timezone) < 0) {
    puts("Could not read username");
    return -1;
  }

  if (setenv("TZ", timezone, true) == -1) {
    perror("setenv");
    return -1;
  }

  return 0;
}

#define ADMIN_COMMAND "/bin/sh -p"

static void admin_console_handler(void) {
  if (system(ADMIN_COMMAND) == -1) {
    perror("system");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

static void handle_choice(uint8_t cmd) {
  switch (cmd) {
  case get_time:
    get_time_handler();
    break;

  case set_timezone:
    set_timezone_handler();
    break;

  case admin_console:
    admin_console_handler();
    break;

  case exit_program:
    break;

  default:
    printf("Invalid choice: %u\n", cmd);
    break;
  }
}

int main(void) {
  setbuf(stdout, NULL);

  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  print_banner();

  unsigned long choice = 0;
  do {
    print_menu();
    if (get_integer_from_user(&choice) < 0) {
      printf("Input is not a valid number\n");
      break;
    }

    if (choice == set_timezone || choice == admin_console) {
      if (authenticate_admin() < 0) {
        printf("Authentication failed!\n");
        continue;
      }
    }

    handle_choice(choice);
  } while (choice != exit_program);

  return EXIT_SUCCESS;
}