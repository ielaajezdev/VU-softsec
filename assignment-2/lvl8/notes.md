- probably a uaf vuln, can use memcpy and also includes a malloc and free
- goal is to malloc, then free so that we can malloc again and set the memory to what we want. Then the next time the "freed" pointer does what we want
- want to overwrite pair->data through modifying the heap
- heap metadata is stored one byte after the pointer that is returned by malloc and used by free

- `pair_add()` sets the size in the struct to 1 byte smaller than the malloced memory. When a pair is deleted, this makes that the last byte not is cleared to 0
    - but this byte is probably zero already, as this is the null terminator from a string

- blocks of 8 bytes are allocated

- `free_list_remove()` segf can be avoided by setting both prev and next to NULL, performing no actions
- then the size gets 

- 32 bytes before newkey=... starts, the pointer is loaded in memory (so this is where the struct metadata starts)
- because the `process_cmd` checks only for bytes < 0 we can insert NULL bytes!