#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>

#define STACK_SIZE 0x100000
#define STACK_END 0x7ffffffff000
#ifndef STACK_PROT
#define STACK_PROT (PROT_READ|PROT_WRITE|PROT_EXEC)
#endif


int main_no_aslr(int argc, char **argv, char **envp);

extern char **environ;

static char *noaslr_write_string(char *p, char *str) {
	size_t len = strlen(str);
	p -= len + 1;
	strcpy(p, str);
	return p;
}

static char *noaslr_skip_strings(char *p, char **strarr, size_t *count) {
	char *str, **strcurr;

	*count = 0;
	for (strcurr = strarr; (str = *strcurr); strcurr++) {
		p -= strlen(str) + 1;
		*count += 1;
	}
	return p;
}

static void noaslr_write_strings(char *p, char **strarrnew, char **strarr) {
	char *str, **strcurr;

	for (strcurr = strarr; (str = *strcurr); strcurr++, strarrnew++) {
		*strarrnew = p;
		strcpy(p, str);
		p += strlen(str) + 1;
	}
}

#ifdef __cplusplus
#define NOASLR_MAIN "_Z12main_no_aslriPPcS0_"
#else
#define NOASLR_MAIN "main_no_aslr"
#endif

int main(int argc, char **argv, char **envp) {
	char **arr, **arrargs, **arrenv;
	size_t countargs, countenv;
	char *p, *pargs, *penv;
	long result;
	void *stack;

	if (((long) argv | 0xfff) + 1 == STACK_END) {
		return main_no_aslr(argc, argv, envp);
	}

	stack = mmap((void *) (STACK_END - STACK_SIZE), STACK_SIZE,
		STACK_PROT, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
	if (stack == MAP_FAILED) {
		perror("error: cannot allocate no-ASLR stack");
		exit(-1);
	}

	p = (char *) STACK_END - 8;
	p = noaslr_write_string(p, argv[0]);
	p = penv = noaslr_skip_strings(p, envp, &countenv);
	p = pargs = noaslr_skip_strings(p, argv, &countargs);
	p = (char *) ((long) p & ~7L);
	
	arr = (char **) p;
	arrenv = (arr -= countenv + 1);
	arrargs = (arr -= countargs + 1);
	noaslr_write_strings(penv, arrenv, envp);
	noaslr_write_strings(pargs, arrargs, argv);

	// The System V calling convention requires the stack to be aligned on
	// 16 bytes. If this is not the case, the optimizations in libc will
	// break.
	uintptr_t new_stack_ptr = (uintptr_t)(arrargs - 1);
	new_stack_ptr &= ~0xf;

	environ = arrenv;
	__asm__("push %%rbp           \n"
		"mov  %%rsp, %%rbp    \n"
		"mov  %1,    %%rsp    \n"
		"call " NOASLR_MAIN " \n"
		"mov  %%rbp, %%rsp    \n"
		"pop  %%rbp           \n"
		: "=a" (result)
		: "r" (new_stack_ptr),
		  "D" (argc),
		  "S" (arrargs),
		  "d" (arrenv));
	return result;
}

#define main main_no_aslr
