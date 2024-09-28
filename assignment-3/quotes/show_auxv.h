#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>

/*
 * LD_SHOW_AUXV doesn't seem to work with SGID binaries. Re-implement it here
 * so we can use it.
 */
__attribute__((constructor)) void show_auxv() {
  if (getenv("_REAL_LD_SHOW_AUXV")) {
    printf("%-22s %lx\n", "AT_SYDINFO_EHDR", getauxval(AT_SYSINFO_EHDR));
    printf("%-22s %lx\n", "AT_MINSIGSTKSZ", getauxval(AT_MINSIGSTKSZ));
    printf("%-22s %lx\n", "AT_HWCAP", getauxval(AT_HWCAP));
    printf("%-22s %lx\n", "AT_PAGESZ", getauxval(AT_PAGESZ));
    printf("%-22s %lx\n", "AT_CLKTCK", getauxval(AT_CLKTCK));
    printf("%-22s %lx\n", "AT_PHDR", getauxval(AT_PHDR));
    printf("%-22s %lx\n", "AT_PHENT", getauxval(AT_PHENT));
    printf("%-22s %lx\n", "AT_PHNUM", getauxval(AT_PHNUM));
    printf("%-22s %lx\n", "AT_BASE", getauxval(AT_BASE));
    printf("%-22s %lx\n", "AT_FLAGS", getauxval(AT_FLAGS));
    printf("%-22s %lx\n", "AT_ENTRY", getauxval(AT_ENTRY));
    printf("%-22s %lu\n", "AT_UID", getauxval(AT_UID));
    printf("%-22s %lu\n", "AT_EUID", getauxval(AT_EUID));
    printf("%-22s %lu\n", "AT_GID", getauxval(AT_GID));
    printf("%-22s %lu\n", "AT_EGID", getauxval(AT_EGID));
    printf("%-22s %lx\n", "AT_SECURE", getauxval(AT_SECURE));
    printf("%-22s %lx\n", "AT_RANDOM", *((uint64_t *)getauxval(AT_RANDOM)));
    printf("%-22s %lx\n", "AT_HWCAP", getauxval(AT_HWCAP2));
  }
}