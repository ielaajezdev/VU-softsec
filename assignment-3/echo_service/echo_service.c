#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ADMIN_COMMAND "/bin/sh -p"
#define EXIT_COMMAND "exit\n"
#define HANDLER_TIMEOUT 10
#define ECHO_BUFFER_SIZE 0x200
#define LISTEN_BACKLOG 20

void admin_console(void) {
  if (system(ADMIN_COMMAND) == -1) {
    perror("system");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

static int send_banner_message(int conn_fd) {
  char banner_message[] = "Welcome to the echo service\n\n"
                          "This service echoes back all the data you send.\n"
                          "(use 'exit' to close the connection)\n\n";
  if (send(conn_fd, banner_message, sizeof(banner_message), MSG_NOSIGNAL) ==
      -1) {
    perror("send");
    return -1;
  }

  return 0;
}

static int run_echo_loop(int conn_fd) {
  char echo_buffer[0x10];
  while (true) {
    char prompt[] = "> ";
    if (send(conn_fd, prompt, sizeof(prompt), MSG_NOSIGNAL) == -1) {
      perror("send");
      return -1;
    }

    memset(echo_buffer, 0, sizeof(echo_buffer));
    int num_bytes = recv(conn_fd, echo_buffer, ECHO_BUFFER_SIZE, 0);
    if (num_bytes == -1) {
      perror("recv");
      return -1;
    }

    if (!strncmp(echo_buffer, EXIT_COMMAND, sizeof(EXIT_COMMAND))) {
      break;
    }

    if (send(conn_fd, echo_buffer, num_bytes, MSG_NOSIGNAL) == -1) {
      perror("send");
      return -1;
    }
  }

  return 0;
}

static int send_closing_message(int conn_fd) {
  char closing_message[] = "Exiting.\n";
  if (send(conn_fd, closing_message, sizeof(closing_message), MSG_NOSIGNAL) ==
      -1) {
    perror("send");
    return -1;
  }

  return 0;
}

void wait_child(__attribute__((unused)) int signal) {
  while (waitpid(-1, 0, WNOHANG) > 0)
    ;
}

int main(void) {
  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  puts("Initializing echo service.");

  struct rlimit core_lim = {
      .rlim_cur = 0,
      .rlim_max = 0,
  };
  if (setrlimit(RLIMIT_CORE, &core_lim) == -1) {
    perror("setrlimit");
    exit(1);
  }

  int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_fd == -1) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)},
  };
  if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(sock_fd, LISTEN_BACKLOG) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in real_addr;
  socklen_t real_addr_len = sizeof(real_addr);
  if (getsockname(sock_fd, (struct sockaddr *)&real_addr, &real_addr_len) ==
      -1) {
    perror("getsockname");
    exit(EXIT_FAILURE);
  }

  printf("Connect to port %d on loopback interface.\n",
         ntohs(real_addr.sin_port));

  if (signal(SIGCHLD, wait_child) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  while (true) {
    int conn_fd = accept(sock_fd, NULL, NULL);
    if (conn_fd == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    puts("Handling new connection.");

    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if (pid) {
      // Parent
      close(conn_fd);
    } else {
      // Child
      close(sock_fd);

      alarm(HANDLER_TIMEOUT);
      send_banner_message(conn_fd);
      run_echo_loop(conn_fd);
      send_closing_message(conn_fd);
      close(conn_fd);

      exit(EXIT_SUCCESS);
    }
  }

  close(sock_fd);

  return EXIT_SUCCESS;
}