input_file = "unique-gutenberg.txt"
output_file = "24letter-gutenberg.txt"

def main():
    combinations = {}
    
    with open(input_file, "r") as f:
            print("Reading file: " + input_file)
            words = f.read().splitlines()
            
            letterwords11 = []
            letterwords13 = []
            for word in words:
                if len(word) == 11:
                    letterwords11.append(word)
                elif len(word) == 13:
                    letterwords13.append(word)
            
            # For every word, try to find a combination of appended words that is exactly 24 characters long
            for word in letterwords11:
                for word2 in letterwords13:
                    com = word + word2
                    if com not in combinations:
                        combinations[com] = 1
                
    print("Found combinations: " + str(len(combinations)))            
    
    # write all combinations to a file
    with open(output_file, 'w') as f:
        for comb in combinations:
            f.write(comb + "\n")

if __name__ == "__main__":
    main()