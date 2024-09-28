#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USERNAME_BUFFER_SIZE 0x40
#define PASSWORD_BUFFER_SIZE 0x10

enum command {
  exit_program,
  lights_on,
  lights_off,
  admin_console,
  command_num,
};

static char const *const command_description[command_num] = {
    [exit_program] = "Exit this program",
    [lights_on] = "Turn on the lights",
    [lights_off] = "Turn off the lights",
    [admin_console] = "Open admin console",
};

static void print_banner(void) {
  printf("Welcome to the smart lighting management service!\n\n");
  printf("This service lets you remotely control your lights, if the "
         "appropriate plugin is provided");
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

struct auth_info {
  char username[USERNAME_BUFFER_SIZE];
  char password[PASSWORD_BUFFER_SIZE];
};

struct control_plugin {
  void (*switch_lights)(bool);
  bool (*authenticate_admin)(struct auth_info *info);
  void (*start_admin_console)(void);
};

static void test_switch_lights(bool status) {
  if (status) {
    puts("Lights on!");
  } else {
    puts("Lights off!");
  }
}

#define ADMIN_COMMAND "/bin/sh -p"

static bool
test_authenticate_admin(__attribute__((unused)) struct auth_info *info) {
  // TODO: Implement admin authentication
  return false;
}

static void test_start_admin_console(void) {
  if (system(ADMIN_COMMAND) == -1) {
    perror("system");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

static int lights_on_handler(struct control_plugin plugin) {
  plugin.switch_lights(true);
  return 0;
}

static int lights_off_handler(struct control_plugin plugin) {
  plugin.switch_lights(false);
  return 0;
}

static int start_admin_console_handler(struct control_plugin plugin) {
  struct auth_info info;

  printf("Insert username > ");
  if (get_string_from_user(USERNAME_BUFFER_SIZE, info.password) < 0) {
    puts("Could not read username");
    return -1;
  }

  printf("Insert password > ");
  if (get_string_from_user(PASSWORD_BUFFER_SIZE, info.password) < 0) {
    puts("Could not read password");
    return -1;
  }

  if (plugin.authenticate_admin(&info)) {
    plugin.start_admin_console();
    return 0;
  } else {
    puts("Authentication failed!");
    return -1;
  }
}

int main(void) {
  setbuf(stdout, NULL);

  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  print_banner();

  struct control_plugin test_plugin = {
      .switch_lights = test_switch_lights,
      .authenticate_admin = test_authenticate_admin,
      .start_admin_console = test_start_admin_console,
  };

  unsigned long choice = 0;
  do {
    print_menu();
    if (get_integer_from_user(&choice) < 0) {
      printf("Input is not a valid number\n");
      break;
    }

    switch (choice) {
    case lights_on:
      lights_on_handler(test_plugin);
      break;

    case lights_off:
      lights_off_handler(test_plugin);
      break;

    case admin_console:
      start_admin_console_handler(test_plugin);
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