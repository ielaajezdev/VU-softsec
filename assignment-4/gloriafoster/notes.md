- this is a padding oracle attack, server exposes if padding is incorrect
    - also a CBC encryption, so requirements met
    - used padBuster to use the padding oracle:

```
sudo perl padBuster.pl http://websec.vusec.net:62039/GdoioDWi/cgi-bin/index.php jiB9eXhTEEVeMlhGU+E9axj0Ii0cVyTjQkPDukq9PhAvr4kbAuSiLfc4izahT9ss 16 -usebody -cookies PHPSESSID=03sqskm1ffl0g0u83vopj48i7h -post "cipher_input=jiB9eXhTEEVeMlhGU+E9axj0Ii0cVyTjQkPDukq9PhAvr4kbAuSiLfc4izahT9ss" -l "that's the flag"
```