# Utility to run a program with only specific environment variables set

import os
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 run.py <program> <program_args>")
        sys.exit(1)
        
    # Read shellcode from file
    with open("shellcode.bin", "rb") as f:
        shellcode = f.read()
    
        clean_env = {
            "USERNAME": os.environ['USERNAME'],
            "HOSTNAME": os.environ['HOSTNAME'],
            "L33T": shellcode,
        }

        # Run the program
        os.execve(sys.argv[1], sys.argv[1:], clean_env)
    

if __name__ == "__main__":
    main()