# Define the range of indices you want to scan
start = -4000
end = 4000
    
# Get the address of messages[0]
messages_0 = gdb.execute("p state->messages[0]", to_string=True).split("\n")[0].split(" ")[-1]
print("messages_0", messages_0) 
messages_0_ptr = int(messages_0, 16)

# Loop through the indices
for i in range(start, end + 1):
    try:
        res = gdb.execute(f"p &state->messages[{i}]->message", to_string=True)      
        message_ptr = res  
        message_ptr = message_ptr.split(" ")[4].strip().replace("\n", "")
        message_address = int(message_ptr, 16)

        # diff = abs(message_address - messages_0_ptr)
        
        
        if message_ptr == messages_0_ptr:    
            print(f"[{i}] message_ptr {hex(message_address)} points to messages[0]: {hex(messages_0_ptr)}")
            print("res > ", res)
            
        # if diff == 0:
        #     break
                 
    except gdb.error as e:
        # Catch and ignore errors, like invalid memory access or invalid pointer dereferencing
        pass
