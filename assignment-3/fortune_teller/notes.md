- shadow stack information finding from the sldies, mmap uses `MAP_ANONYMOUS` which means that memory is not backed by physical page, can allocate terabytes without actually using it

- messages[17] points to `random64()`+82 
    - seems to disappear when program executes a dispatch
- can set negative indices!
    - using `source gdblibc.py`, found that messages[-118] until [-122] are often pointers to `main_arena`, used to leak libc base
