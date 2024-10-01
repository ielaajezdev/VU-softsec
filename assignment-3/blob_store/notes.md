- when freeing a key, it is not removed from entries (`entry->blob`) so we have a dangling pointer
    - can call free multiple times
        - but the key is cleared on the first free and this key is not stored on the heap
            - not a problem because you can free a blob with an empty key (`strncmp` will return true)

- can use `malloc` to get access to memory that is still pointed to by the dangling pointer: can set free metadata this way

- if we "delete" a blob (frees it) and then request to read that key again with an empty key field (`\n`), it will read and show the data at that blob's memory address 
    - if we allocate a large buffer, this will be in the unsorted bin (see: https://blog.quarkslab.com/heap-exploitation-glibc-internals-and-nifty-tricks.html) and we can leak the addr of `main_arena` which is in libc to find the libc base
        - in this case `main_arena+96`
    
- used the slides to do the tcache poison