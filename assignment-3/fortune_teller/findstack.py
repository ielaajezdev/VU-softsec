# Define the range of indices you want to scan
start = -4000
end = 4000

# Get the memory mappings to identify the stack region
mappings = gdb.execute("info proc mappings", to_string=True)

# Parse the memory mappings to find the stack region
stack_start, stack_end = None, None
for line in mappings.splitlines():
    if '[stack]' in line:
        parts = line.split()
        stack_start = int(parts[0], 16)
        stack_end = int(parts[1], 16)
        break

if stack_start is not None and stack_end is not None:
    print(f"Stack region: {hex(stack_start)} - {hex(stack_end)}")
    
# Get the address of messages[0]
messages_0 = gdb.execute("p state->messages", to_string=True).split("\n")[0].split(" ")[-1]
print("messages_0", messages_0) 
messages_0_ptr = int(messages_0, 16)

# Loop through the indices
for i in range(start, end + 1):
    try:
        res = gdb.execute(f"x/1gx &state->messages[{i}]->message", to_string=True)      
        message_ptr = res  
        message_ptr = message_ptr.split(":")[-1].strip()
        message_address = int(message_ptr, 16)

        # Check if the message pointer is within the stack region
        if stack_start <= message_address <= stack_end:
            print(f"[{i}] message_ptr {hex(message_address)} is in the stack region, difference from messages[0]: {hex(abs(message_address - messages_0_ptr))}")
            print("res > ", res)
                 
    except gdb.error as e:
        # Catch and ignore errors, like invalid memory access or invalid pointer dereferencing
        pass
