import copy

input_file = "./unique-gutenberg.txt"
output_file = "./variations-test.txt"

# All found variations
found = {}

# All replacemenets
replacements = {
    'a': ['^', '4'],
    'm': ['(v)', '|\\/|'],
    'w': ['\\/\\/'],
    't': ['-|-'],
    'o': ['*', '0'],
    'h': ['}{', '#'],
    'c': ['('],
    's': ['5'],
    'y': ['`i`'],
    'g': ['C-'],
    'b': ['8'],
    'u': ['L|'],
    'p': ['|*'],
    'k': ['|('],
}

# We need to run each variation twice, starting from the input word to make sure that we get all possible variations
def vary_replacements(base_word, key, value, at):
    if at >= len(base_word):
        return
    
    while at < len(base_word):
        if base_word[at] == key:
            # Replace with the value
            new_word = base_word[:at] + value + base_word[at+1:]
            if new_word not in found:
                found[new_word] = 1
                # Continue the replacements with the new word and with the base word
                vary_replacements(new_word, key, value, 0)
                vary_replacements(base_word, key, value, at + 1)
        at += 1
        
# Try with different casing
def vary_case(base_word):
    # Start with lowercase
    new_word = base_word.lower()
    if new_word not in found:
        found[new_word] = 1

    # Replace every individual occurence with an uppercase letter
    for i in range(len(base_word)):
        variation = new_word[:i] + new_word[i].upper() + new_word[i+1:]
        if variation not in found:
            found[variation] = 1
    
    # Try with all uppercase
    new_word = base_word.upper()
    if new_word not in found:
        found[new_word] = 1
        
    # Replace every individual occurence with a lowercase letter
    for i in range(len(base_word)):
        variation = new_word[:i] + new_word[i].lower() + new_word[i+1:]
        if variation not in found:
            found[variation] = 1
    
    

# recursive function that returns all found variations, based on a set of rules
# def variations(word, varied):
#     # Replace every individual occurence with a variation
#     for i in range(len(word)):
#         if word[i] == "a":
#             vary = word[:i] + "^" + word[i+1:]
#             vary2 = word[:i] + "4" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 variations(vary, True)
#             if vary2 != word:
#                 if vary2 not in found:
#                     found[vary2] = 1
#                 # variations(vary2, True)    
#         if word[i] == "w":
#             vary = word[:i] + "\\/\\/" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 variations(vary, True)    
#         if word[i] == "h":
#             vary = word[:i] + "}{" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 variations(vary, True)    
#         if word[i] == "t":
#             vary = word[:i] + "-|-" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "m":
#             vary = word[:i] + "|\\/|" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 variations(vary, True)    
#         if word[i] == "o" or word[i] == "0":
#             vary = word[:i] + "*" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "c":
#             vary = word[:i] + "(" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 variations(vary, True)    
#         if word[i] == "s":
#             vary = word[:i] + "5" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "u":
#             vary = word[:i] + "L|" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "g":
#             vary = word[:i] + "C-" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "b":
#             vary = word[:i] + "8" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)    
#         if word[i] == "y":
#             vary = word[:i] + "`i'" + word[i+1:]
#             if vary != word:
#                 if vary not in found:
#                     found[vary] = 1
#                 # variations(vary, True)
                
#         # try upercasing the letter
#         vary = word[:i] + word[i].upper() + word[i+1:]
#         if vary != word:
#             if vary not in found:
#                 found[vary] = 1
#             # variations(vary, True)
        
#     if varied and word not in found and word.lower() not in found:
#         found[word] = 1

def main():
    # do this for the input file once
    words = []
    with open(input_file, "r") as f:
            print("Reading file: " + input_file)
            words = f.read().splitlines()
            
            print("Running first round")
            
            for word in words:
                for key in replacements:
                    for value in replacements[key]:
                        vary_replacements(word, key, value, 0)
                        
     
    
    # now do this for all generated words one more time
    # old = copy.deepcopy(found)
    # print("Running second round on " + str(len(old)) + " words")
    # for word in old:
    #     for key in replacements:
    #         for value in replacements[key]:
    #             vary_replacements(word, key, value, 0)
                
    old = copy.deepcopy(found)
    print("Running case round on " + str(len(old)) + " words")
    for word in old:
        vary_case(word)         
    
    # Add these last
    for word in words:
        found[word + 'xor'] = 1       
        found[word + 'zorz'] = 1
    
                
    # i = 3            
    
    # while len(old) != len(found):
    #     print("Running round " + str(i) + " on " + str(len(old)) + " words")
        
    #     old = copy.deepcopy(found)
    #     for word in old:
    #         for key in replacements:
    #             for value in replacements[key]:
    #                 vary_replacements(word, key, value, 0)
        
    #     i += 1
        
                
                # found[word + 'xor'] = 1
                # found[word + 'zorz'] = 1
                
    print("Found Variations: " + str(len(found)))            
    
    # write all variations to a file
    with open(output_file, 'w') as f:
        for comb in found:
            f.write(comb + "\n")

if __name__ == "__main__":
    main()