#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "show_auxv.h"

#define MAX_STR_LEN 256

struct quote *list = NULL;

struct quote {
  char txt[MAX_STR_LEN];
  char author[MAX_STR_LEN];
  struct quote *next;
};

void insert_ll(struct quote *q) {
  struct quote **ptr = &list;
  while (*ptr != NULL)
    ptr = &((*ptr)->next);

  *ptr = q;
}

void add_quote(const char *quote, const char *author) {
  struct quote *q = malloc(sizeof(*q));

  strncpy(q->txt, quote, sizeof(q->txt));
  q->txt[sizeof(q->txt) - 1] = '\0';

  strncpy(q->author, author, sizeof(q->author));
  q->author[sizeof(q->author) - 1] = '\0';

  q->next = NULL;

  insert_ll(q);
}

void display_quotes() {
  struct quote *ptr = list;
  while (ptr != NULL) {
    printf("%s\n", ptr->txt);
    printf("\t- %s\n", ptr->author);
    ptr = ptr->next;
  }
}

void new_user_quote() {
  char quote[512];
  char author[512];

  printf("Enter quote: ");
  fgets(quote, 2048, stdin);
  quote[strcspn(quote, "\n")] = 0;

  printf("Enter author: ");
  fgets(author, 2048, stdin);
  author[strcspn(author, "\n")] = 0;

  add_quote(quote, author);
}

void add_initial_quotes() {
  add_quote("Nobody actually creates perfect code the first time around, "
            "except me. But there's only one of me.",
            "Linus Torvalds");
  add_quote("Kip of Lamb?",
            "The dude who works at the kebab stand across De Boelelaan");
  add_quote(
      "Since glibc 2.3.4, LD_SHOW_AUXV is ignored in secure-execution mode.",
      "ld.so man page");
  add_quote("There are no bugs in our kernel modules.",
            "Someone in the Crowdstrike Slack workspace, July 18, 2024");
  add_quote("There is absolutely no way anyone can coerce my web application "
            "into calling phpinfo().",
            "Ross Ulbrict");
  add_quote("Do you get to the cloud district very often? Oh, what am I "
            "saying, of course you don't.",
            "Nazeem");
  add_quote("92 is half of 99.", "Zezima");
  add_quote("Oops.", "Robert Tappan Morris, November 3, 1988");
  add_quote("I'd spell creat with an e.",
            "Ken Thompson, commenting on what he would design differently if "
            "redesigning Unix");
  add_quote("The most effective debugging tool is still careful thought, "
            "coupled with judiciously placed print statements.",
            "Brian Kernighan");
}

int main() {
  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  add_initial_quotes();
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);

  printf("====== Quote Server ======");
  while (1) {
    char choice;
    printf("\n");
    printf("a - Add quote\n");
    printf("d - Display all quotes\n");
    printf("q - Quit this program\n");
    printf(">>> ");
    fflush(stdout);
    if (scanf("%c%*c", &choice) != 1) {
      goto err;
    }

    switch (choice) {
    case 'a':
      new_user_quote();
      break;
    case 'd':
      display_quotes();
      break;
    case 'q':
      return 0;
    default:
      goto err;
    }
  }

  return 0;

err:
  fprintf(stderr, "Error: Invalid Input\n");
  return 1;
}