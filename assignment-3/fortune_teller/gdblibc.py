# Define the range of indices you want to scan
start = -4000
end = 4000

# Loop through the indices
for i in range(start, end + 1):
    try:
        # print(f"Index {i}: ----")    
        # Evaluate the pointer at state->messages[i]->message
        message_ptr = gdb.execute(f"x/1gx state->messages[{i}]->message", to_string=True)
        
        message_ptr = message_ptr.split(":")[-1].strip()
        
        # print("got msg ptr", message_ptr)

        # Dereference the pointer to get the address it points to
        #message_value = message_ptr.dereference()

        # Try to find if this pointer points to any known libc symbol
        # You can check if the address is within a known libc range
        # or if GDB can map it to a known symbol
        symbol = gdb.execute(f"info symbol {message_ptr}", to_string=True)

        if not "No symbol matches" in symbol:
            print(f"[{i}] got symbol", symbol)
     

    except gdb.error as e:
        # Catch and ignore errors, like invalid memory access or invalid pointer dereferencing
        # print(f"Index {i}: Invalid pointer or memory access. Skipping.", e)
        pass
