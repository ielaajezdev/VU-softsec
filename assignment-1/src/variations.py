input_file = "./unique-top250.txt"
output_file = "./variations-top250.txt"

found = {}

# recursive function that returns all found variations, based on a set of rules
def variations(word, varied):

    # Replace every individual occurence with a variation
    for i in range(len(word)):
        if word[i] == "a":
            vary = word[:i] + "^" + word[i+1:]
            vary2 = word[:i] + "4" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                variations(vary, True)
            if vary2 != word:
                if vary2 not in found:
                    found[vary2] = 1
                # variations(vary2, True)    
        if word[i] == "w":
            vary = word[:i] + "\\/\\/" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                variations(vary, True)    
        if word[i] == "h":
            vary = word[:i] + "}{" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                variations(vary, True)    
        if word[i] == "t":
            vary = word[:i] + "-|-" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "m":
            vary = word[:i] + "|\\/|" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                variations(vary, True)    
        if word[i] == "o" or word[i] == "0":
            vary = word[:i] + "*" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "c":
            vary = word[:i] + "(" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                variations(vary, True)    
        if word[i] == "s":
            vary = word[:i] + "5" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "u":
            vary = word[:i] + "L|" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "g":
            vary = word[:i] + "C-" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "b":
            vary = word[:i] + "8" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)    
        if word[i] == "y":
            vary = word[:i] + "`i'" + word[i+1:]
            if vary != word:
                if vary not in found:
                    found[vary] = 1
                # variations(vary, True)
                
        # try upercasing the letter
        vary = word[:i] + word[i].upper() + word[i+1:]
        if vary != word:
            if vary not in found:
                found[vary] = 1
            # variations(vary, True)
        
    if varied and word not in found and word.lower() not in found:
        found[word] = 1

def main():
    with open(input_file, "r") as f:
            print("Reading file: " + input_file)
            words = f.read().splitlines()
            
            for word in words:
                variations(word, False)        
                found[word + 'xor'] = 1
                found[word + 'zorz'] = 1
                
    print("Found Variations: " + str(len(found)))            
    
    # write all variations to a file
    with open(output_file, 'w') as f:
        for comb in found:
            f.write(comb + "\n")

if __name__ == "__main__":
    main()