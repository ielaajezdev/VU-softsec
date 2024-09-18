#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "noaslr.h"

#include "mymalloc.h"

#define PAIRCOUNT 1024

struct pair {
	size_t size;
	char *data;
};

static struct pair pairs[PAIRCOUNT];
static size_t paircount;

static struct pair *pair_add(const char *data, size_t datalen);
static void pair_delete(struct pair *pair);
static struct pair *pair_find(const char *key, size_t keylen);
static int pair_matches_key(struct pair *pair, const char *key, size_t keylen);
static void pair_set(struct pair *pair, const char *data, size_t datalen);
static void process_command(const char *line, size_t linelen);
static void process_commands(void);
static void process_command_get(const char *arg, size_t arglen);
static void process_command_set(const char *arg, size_t arglen);
static const char *strnchr(const char *s, int c, size_t n);

static struct pair *pair_add(const char *data, size_t datalen) {
	struct pair *pair;

	if (paircount >= PAIRCOUNT) {
		fprintf(stderr, "error: too many key-value pairs\n");
		exit(1);
	}

	pair = &pairs[paircount++];
	pair->size = datalen;
	pair->data = mymalloc(datalen + 1);
	pair_set(pair, data, datalen);

	return pair;
}

static void pair_delete(struct pair *pair) {
	memset(pair->data, 0, pair->size);
	myfree(pair->data);
}

static struct pair *pair_find(const char *key, size_t keylen) {
	size_t i;
	struct pair *pair;

	for (i = 0; i < paircount; i++) {
		pair = &pairs[i];
		if (pair_matches_key(pair, key, keylen)) return pair;
	}

	return NULL;
}

static int pair_matches_key(struct pair *pair, const char *key, size_t keylen) {
	size_t pairkeylen;
	char *separator;

	separator = strchr(pair->data, '=');
	if (!separator) return 0;

	pairkeylen = separator - pair->data;
	if (keylen != pairkeylen) return 0;

	return strncmp(key, pair->data, keylen) == 0;
}

static void pair_set(struct pair *pair, const char *data, size_t datalen) {
	memcpy(pair->data, data, datalen);
	pair->data[datalen] = 0;
}

static void process_command(const char *line, size_t linelen) {

	if (linelen == 1 && line[0] == 'x') exit(0);
	if (linelen < 1) return;
	if (linelen < 3 || line[1] != ' ') {
		fprintf(stderr, "invalid command syntax\n");
		return;
	}

	switch (line[0]) {
	case 'g':
		process_command_get(line + 2, linelen - 2);
		break;
	case 's':
		process_command_set(line + 2, linelen - 2);
		break;
	default:
		fprintf(stderr, "unknown command\n");
		break;
	}
}

static void process_commands(void) {
	int c;
	char line[1024];
	size_t linelen = 0;
	int warned = 0;

	for (;;) {
		c = getchar();
		if (c < 0 || c == '\n') {
			process_command(line, linelen);
			if (c < 0) break;
			linelen = 0;
			warned = 0;
			continue;
		}
		if (linelen >= sizeof(line)) {
			if (!warned) {
				fprintf(stderr, "line too long, truncating\n");
				warned = 1;
			}
			continue;
		}
		line[linelen++] = c;
	}
}

static void process_command_get(const char *arg, size_t arglen) {
	struct pair *pair;

	pair = pair_find(arg, arglen);
	if (!pair) {
		fprintf(stderr, "key not stored\n");
		return;
	}
	printf("retrieved: %s\n", pair->data);
}

static void process_command_set(const char *arg, size_t arglen) {
	struct pair *pair;
	const char *separator;

	separator = strnchr(arg, '=', arglen);
	if (!separator) {
		fprintf(stderr, "not a key=value pair\n");
		return;
	}

	pair = pair_find(arg, separator - arg);
	if (!pair) {
		pair = pair_add(arg, arglen);
		printf("added: %s\n", pair->data);
		return;
	}

	if (pair->size < arglen) {
		pair_delete(pair);
		pair = pair_add(arg, arglen);
	} else {
		pair_set(pair, arg, arglen);
	}
	printf("overwritten: %s\n", pair->data);
}

static const char *strnchr(const char *s, int c, size_t n) {
	while (n-- > 0) {
		if (*s == c) return s;
		s++;
	}
	return NULL;
}

int main(int argc, char **argv, char **envp) {
	printf("available commands:\n");
	printf("  g key       - get value of variable 'key'\n");
	printf("  s key=value - set variable 'key' to 'value'\n");
	printf("  x           - exit\n");

	process_commands();
}

