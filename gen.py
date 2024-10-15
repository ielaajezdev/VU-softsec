# Write JPEG header bytes to file fake.jpg

# create the file
f = open("fake.jpg", "wb")
# write the header
f.write(b"\xFF\xD8\xFF\xE0\x00\x10\x4A\x46\x49\x46\x00\x01\x01\x01\x00\x60\x00\x60\x00\x00")
# write the rest of the file, a PHP script that echoes hello world
f.write(b"<?php echo 'hello world'; ?>")

# close the file
f.close()