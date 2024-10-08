#define __GNU_SOURCE
#include <asm/prctl.h>
#include <ctype.h>
#include <dlfcn.h>
#include <err.h>
#include <execinfo.h>
#include <fcntl.h>
#include <immintrin.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

__attribute__((no_instrument_function)) uint64_t random64() {
  int fd = open("/dev/random", O_RDONLY);
  if (fd == -1) {
    err(EXIT_FAILURE, "Failed to open /dev/random");
  }

  uint64_t val = 0;
  ssize_t b = read(fd, &val, sizeof(val));
  if (b != sizeof(val)) {
    err(EXIT_FAILURE, "Failed to read random bytes");
  }
  close(fd);
  return val;
}

__attribute__((constructor, no_instrument_function)) void setup_shadow_stack() {
  uintptr_t shadow_stack = (random64() & 0xffffffff000) | 0x400000000000;

  int page_size = getpagesize();
  uintptr_t base_addr = (shadow_stack - page_size) & ~0xfff;

  // Allocate the shadow stack, including the guard pages
  if (mmap((void *)base_addr, page_size * 8, PROT_NONE,
           MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
    err(EXIT_FAILURE, "Failed to allocate shadow stack");
  }
  base_addr = 0;

  if (mprotect((void *)(shadow_stack & ~0xfff), page_size * 6,
               PROT_READ | PROT_WRITE)) {
    err(EXIT_FAILURE, "Failed to give shadow stack R/W permission");
  }

  // Set the GS base to point to the shadow stack. We could use the FSGSBASE
  // instructions for setting and getting the GS register from userland, but
  // those require a kernel version 5.9 or newer.
  syscall(SYS_arch_prctl, ARCH_SET_GS, shadow_stack);
  shadow_stack = 0;
}

// These are wrapper functions to make sure that the external calls to libc
// are also protected by the shadow stack
extern int __real_printf(const char *restrict fmt, ...);
int __wrap_printf(const char *restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int x = vprintf(fmt, args);
  va_end(args);
  return x;
}

extern void __real_errx(int eval, const char *fmt, ...);
void __wrap_errx(int eval, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  verrx(eval, fmt, args);
  va_end(args);
}

int __wrap_sscanf(const char *restrict str, const char *restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int x = vsscanf(str, fmt, args);
  va_end(args);
  return x;
}

void *__wrap_memcpy(char *restrict dst, const char *restrict src, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    dst[i] = src[i];
  }
  return dst;
}

extern void __real_free(void *ptr);
void __wrap_free(void *ptr) { __real_free(ptr); }
extern ssize_t __real_getline(char **restrict lineptr, size_t *restrict n,
                              FILE *restrict stream);
ssize_t __wrap_getline(char **restrict lineptr, size_t *restrict n,
                       FILE *restrict stream) {
  return __real_getline(lineptr, n, stream);
}
extern int __real_rand(void);
int __wrap_rand(void) { return __real_rand(); }

union shadow_stack_entry {
  struct {
    size_t size;
  } metadata;
  struct {
    void *function;
    void *call_site;
  } entry;
};

// Every function prologue will start with __cyg_profile_func_enter.
__attribute__((no_instrument_function, always_inline)) inline void
__cyg_profile_func_enter(void *this_fn, void *call_site) {
  union shadow_stack_entry __seg_gs *entry = 0;
  entry += ++entry->metadata.size;
  entry->entry.function = this_fn;
  entry->entry.call_site = call_site;
  entry = 0;
}

// Every function epilogue will start with __cyg_profile_func_exit,
// which checks if the return pointer has been modified by comparing
// it to the pointer on the shadow stack.
__attribute__((no_instrument_function, always_inline)) inline void
__cyg_profile_func_exit(void *this_fn, void *call_site) {
  union shadow_stack_entry __seg_gs *entry = 0;

  if (entry->metadata.size == 0) {
    __real_errx(EXIT_FAILURE, "Shadow stack is empty");
  }

  entry += entry->metadata.size--;

  if (entry->entry.function != this_fn || entry->entry.call_site != call_site) {
    __real_errx(EXIT_FAILURE,
                "Shadow stack has been compromised!\n"
                "\tthis_fn: %p (expected %p)\n"
                "\tcall_site: %p (expected %p)",
                this_fn, entry->entry.function, call_site,
                entry->entry.call_site);
  }
  entry = 0;
}

#define FORTUNE_LENGTH (128)
#define NR_OF_MESSAGE (8)

struct fortune_message {
  char message[FORTUNE_LENGTH];
  size_t length;
};

struct message {
  union {
    unsigned char raw;
    enum message_tag {
      MSG_RANDOM = 0,
      MSG_READ = 1,
      MSG_SELECT = 2,
      MSG_WRITE = 3,
      MSG_EXIT = 4,
    } tag;
  } tag;
  union {
    size_t random_size;
    size_t select_index;
    struct {
      size_t length;
      char fortune[FORTUNE_LENGTH];
    } write;
  } message;
};

struct state {
  struct fortune_message **messages;
  size_t index;
};

void read_random_message(size_t max_length) {
  char *text = mmap(NULL, max_length, PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  if (text == MAP_FAILED) {
    warn("Failed to allocate memory for random message");
    return;
  }

  for (size_t i = 0; i < max_length; ++i) {
    char new_char;
    do {
      new_char = random64() & 0xff;
    } while (!islower(new_char) && new_char != ' ' && new_char != '.');
    text[i] = new_char;

    // Make sure that the random message is at least 20 characters, and ends
    // with a dot.
    if (i > 20 && new_char == '.')
      break;
  }

  printf("%s\n", text);

  if (munmap(text, max_length)) {
    err(EXIT_FAILURE, "Failed to unmap previously allocated memory");
  }
}

bool dispatch(struct state *state, const struct message *message) {
  switch (message->tag.tag) {
  case MSG_RANDOM:
    read_random_message(message->message.random_size);
    break;
  case MSG_READ:
    printf("Your message of fortune reads: %s\n",
           state->messages[state->index]->message);
    break;
  case MSG_SELECT:
    state->index = message->message.select_index;
    break;
  case MSG_WRITE:
    memcpy(state->messages[state->index]->message,
           message->message.write.fortune, message->message.write.length);
    break;
  case MSG_EXIT:
    return false;
  default:
    errx(EXIT_FAILURE, "Unknown message type");
    break;
  }
  return true;
}

size_t parse_sizet(const char *line) {
  size_t value;
  if (sscanf(line + 1, " %ld\n", &value) != 1)
    errx(EXIT_FAILURE, "Invalid usage");
  return value;
}

struct message parse_line(const char *line) {
  struct message message = {0};

  sscanf(line, "%hhd", &message.tag.raw);
  int matched = 0;
  switch (message.tag.tag) {
  case MSG_RANDOM:
    message.message.random_size = parse_sizet(line + 1);
    break;
  case MSG_SELECT:
    message.message.select_index = parse_sizet(line + 1);
    break;
  case MSG_WRITE:
    matched =
        sscanf(line + 1, " %ld %127[^\n]\n", &message.message.write.length,
               message.message.write.fortune);
    if (matched != 2) {
      errx(EXIT_FAILURE, "Invalid usage");
    }
    if (message.message.write.length >= FORTUNE_LENGTH) {
      errx(EXIT_FAILURE, "Message is too long");
    }
    break;
  default:
    break;
  }

  return message;
}

void fill_string_buffer(struct fortune_message **buffer, size_t idx,
                        const char *str) {
  size_t length = strlen(str);
  buffer[idx] = calloc(1, sizeof(struct fortune_message));
  buffer[idx]->length = length;
  strncpy(buffer[idx]->message, str, FORTUNE_LENGTH - 1);
}

int main() {
  if (setregid(getegid(), -1) == -1) {
    perror("setregid");
    exit(1);
  }

  setbuf(stdout, NULL);

  struct fortune_message *fortune_messages[NR_OF_MESSAGE] = {};

  size_t idx = 0;
#define FILL(str) fill_string_buffer(fortune_messages, idx++, str);
  FILL("In the binary realm, knowledge is your most powerful exploit.");
  FILL("The best defenses come from understanding the attacker's mind.");
  FILL("A well-crafted payload can turn the tide of any binary battle.");
  FILL("Stack frames may crumble, but your knowledge must stay solid.");
  FILL("A single byte out of place can change the course of history.");
  FILL("Success in exploitation lies in the details most overlook.");
  FILL("In the battle of binaries, the one who sees the invisible wins.");
  FILL("The deepest vulnerabilities hide where you least expect them.");
#undef FILL

  struct state state = {.index = 0, .messages = fortune_messages};

  printf("====================================================\n"
         "================== FORTUNE TELLER ==================\n"
         "====================================================\n"
         "  0 [max length]         Grab random fortune message\n"
         "  1                      Display the fortune message\n"
         "  2 [number]             Choose your fortune message\n"
         "  3 [length] [message]   Overwrite a fortune message\n"
         "  4                      Exit\n");

  char *line = NULL;
  size_t n = 0;
  ssize_t bytes_read = 0;

  printf("Choice: ");
  while (bytes_read = getline(&line, &n, stdin), bytes_read > 0) {
    struct message message = parse_line(line);
    if (!dispatch(&state, &message))
      return 0;
    printf("Choice: ");
  }

  return 0;
}

