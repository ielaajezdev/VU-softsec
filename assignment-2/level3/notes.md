- The filename can't be fooled, uses access and must be in the level3 directory
- The argument is copied AFTER the check, and placed after the filename + there are no length checks on the input so you can overflow the argument into the filename array

- Then the executed filename is different from the checked filename

```bash
./3 sort aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaahalloaaaaaaaaaaaaaaabbbb/usr/local/bin/l33t
```