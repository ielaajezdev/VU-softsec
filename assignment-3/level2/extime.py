#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/time/time
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/time/time')

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
# RELRO:      Partial RELRO
# Stack:      Canary found
# NX:         NX enabled
# PIE:        No PIE (0x400000)
# FORTIFY:    Enabled
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

io = start()
io.sendline("3843")
io.interactive()

