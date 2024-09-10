input_file = "./training-plain.txt"
output_file = "./unique-plain2.txt"

found = {}

def main():
    with open(input_file, "r") as f:
            print("Reading file: " + input_file)
            words = f.read().splitlines()
            
            for word in words:
                # split by : to get the username and password
                sp = word.split(":")
                pw = sp[1]
        
                
                
                if pw not in found:
                    found[pw] = 1
                
    print("Found Plain: " + str(len(found)))            
    
    # write all variations to a file
    with open(output_file, 'w') as f:
        for comb in found:
            f.write(comb + "\n")

if __name__ == "__main__":
    main() 