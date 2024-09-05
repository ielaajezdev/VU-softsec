import os
import re

inputdir = './dictionary/'
outputfile = './unique-top250.txt'

def main():
    # Open all txt files in the ./dictionary folder and read thei content
    dictdir = inputdir
    
    files = []
    for f in os.listdir(dictdir):
        if os.path.isfile(os.path.join(dictdir, f)):
            files.append(f)    
    
    unique_words = {}
    
    for file in files:
        with open(dictdir + file, "r") as f:
            print("Reading file: " + file)
            lines = f.read().splitlines()
            for line in lines:
                # split the line by space and check if the word is already set in the unique_words dictionary (lowercase)
                words = line.split("\t")
                for word in words:
                    # strip special characters, spaces and numbers
                    #word = word.strip().replace(",", "").replace("?", "").replace("'", "").replace("\"", "").replace(".", "").replace("!", "").replace("-", "")
                    word = re.sub(r'[^a-zA-Z0-9]', '', word)
                    word = word.strip()
                    
                    if word and word != "" and word.lower() not in unique_words:
                        unique_words[word.lower()] = 1            
    
    print("Found unique words: " + str(len(unique_words)))
    # Write the unique words to a file, one word per line
    with open(outputfile, 'w') as f:
        for word in unique_words:
            f.write(word + "\n")
            # also write all caps
            f.write(word.upper() + "\n")

    
if __name__ == "__main__":
    main()