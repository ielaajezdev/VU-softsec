# Utility to run a program with only specific environment variables set

import os
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 run.py <program> <program_args>")
        sys.exit(1)
    
    clean_env = {
        "USERNAME": os.environ['USERNAME'],
        "HOSTNAME": os.environ['HOSTNAME'],
        "L33T": os.environ['L33T'],
    }

    # Run the program
    # os.system(" ".join(sys.argv[1:]))
    os.execve(sys.argv[1], sys.argv[1:], clean_env)
    

if __name__ == "__main__":
    main()