- need to first modify the uid, so that an arbitrary function pointer is executed
- need to set the function pointer to some shellcode

- need to set %r13d to 53, and need to set %r14 to the callback location
    - this is done due to optimizations, that is why r14 is not on the stack

- so rules:

- subtract enough from $0x8000000abcde to become the return address
- let the last number of this subtraction be ASCII 5