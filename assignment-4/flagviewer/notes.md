- tried first with several characters, ' " ` and #
    - `#` gives a warning that the web server had a request error, showing some of the response made internally to /backend.php but not of much use
    - probably not a cache poisoning or proxy parsing discrepancy vuln, as there is no cache and it is clear that there is a PHP request being made from the PHP script

- "works.php" gives away that there is a possibilty for request splitting, using CRLF which is url encoded as %0d%0a
- tried some basic payloads for "country" with burp. See if I can make a request to my requestbin. This payload gives two responses:
```
NL%0d%0aContent-Length:%200%0d%0a%0d%0aGET%20/backend.php%20HTTP/1.1%0d%0aHost:%20enipetl9joz2p.x.pipedream.net%0d%0a%0d%0a
```
    - a 404 and a 400
        - the 400 is likely due to the secend request being a GET request instead of a POST
            - confirmed, changing to POST returns the NL flag
```
VU%0d%0aContent-Length:%200%0d%0a%0d%0aPOST%20/backend.php%20HTTP/1.1%0d%0aHost:%20enipetl9joz2p.x.pipedream.net%0d%0a%0d%0a
```
- went over the list of flags, saw VU flag but needs the "Secure-Request" header to be set
- tried to inject the Secure-Request header manually
```
NL%0d%0aContent-Length:%200%0d%0a%0d%0aGET%20/backend.php%20HTTP/1.1%0d%0aSecure-Request:%20enipetl9joz2p.x.pipedream.net%0d%0a%0d%0a
```
- tried with different hosts
```
country=VU%0d%0aGET%20/backendje.php%20HTTP/1.1%0d%0aHost:%20localhost:62886
```

- this seems to work sometimes, using the above payloads gave me two responses so request splitting definitely works, but still no way to set the endpoint. When I change GET into POST, everything works normally and have nothing to explore further

- know that country=# would give an error, so try to replicate that in my normal POST request to index.php (which also runs on apache, so should give the same error for the same payload)
    - inserting a # into any header does not cause an error
    - inserting a # into the POST body does not cause an error
    - inserting a # into the path does cause an error!! (i.e. `POST /K59z1TeP/cgi-bin/index.php# HTTP/1.1`)
        - so this probably means that `country` is injected as GET parameter, like so: `POST /backend.php?country=$country HTTP/1.1`
            - then experimenting is easy, nice. Can follow closely with my first POST request to see if expected output matches
                - first tried setting country to `VU%20HTTP/1.1` which crashes as expected (becomes `POST /backend.php?country=VU HTTP/1.1 HTTP/1.1`)
                - so need to move the second "leftover" HTTP/1.1 to the next line, to make it become meaningless and not cause request errors
                    - this can be done by making it a header value. we want to set the Secure-Request anyway so this solves two problems in one
                        - tried setting country to `VU%20HTTP/1.1%0d%0aSecure-Request:%20` which would become  
                            `POST /backend.php?country=VU HTTP/1.1 
                             Secure-Request: HTTP/1.1`
                             due to the line break introduced by CRLF (%0d%0a)
                            - did not work
                                - stupid, added the %20 at the end for a space between header and value but that space was already there
                                - so setting country to `VU%20HTTP/1.1%0d%0aSecure-Request:` did work
    