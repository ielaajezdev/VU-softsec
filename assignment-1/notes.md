## Patterns

- passwords seem to have repeating pattern of one long, multi-word string
    - long string length seems to be 24 chars long and comes from the gutenberg dictionary
    - `grep`: `^.{6}:[a-zA-Z0-9]{24}$`
    - these strings are all lowercase
    - these strings consist of a 11 and 13-letter word concatenated
- uses variations on full names (such as password `valerie` for Valerie Johnson)
- uses alternative characters for common letters:
    - `\/\/` for W
    - `^` for A
    - `-|-` for T
    - `|\/|` for M
    - `*` for O
    - `0` for O
    - `}{` for H
    - `(` for C
    - `5` for S
    - `#` for H
    - ``i'` for Y
    - `C-` for G
    - `8` for B
    - `L|` for U
    # new 
    - `|*` for P
    - `|(` for K
    - `0` for O
    - `(v)` for M
    
    - but this is for names/usernames only
    - for usernames: some are fully caps or mixed case as well
        - some contain two year numbers as suffix, or a full year
        
- words also suffixed with `zorz` (usernames as well)
    - same for `xor`

- uses seemingly random number combinations
- Also second name sometimes used
- Also multiple letters of a name lowercased/uppercased
