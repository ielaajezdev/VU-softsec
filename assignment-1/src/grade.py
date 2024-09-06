import sys
import subprocess
import time

def main(script, answers_path):
    print("Grading...")
    print("Run with script: '" + script + "' and answers file: '" + answers_path + "'")
    print("")
    
    # Open the answers file and read the content
    answers = []
    with open(answers_path, "r") as f:
        answers = f.read().splitlines()
        
    start = time.time()
        
    # Execute the bash script and read from stdout, separate the output by newline
    process = subprocess.Popen(script, stdout=subprocess.PIPE, shell=True)
    output, error = process.communicate()
    end = time.time()
    output = output.decode("utf-8").splitlines()
    
    # For every output line, check if it matches one of the answers exactly
    correct = 0
    incorrect = 0
    for line in output:
        if line in answers:
            correct += 1
        else:
            incorrect += 1
            print(" INCORRECT: " + line)
            
            
    print("--- Results ---")
    print("Total answers: " + str(len(answers)))
    print("- correct by you: " + str(correct))
    print("- incorrect by you: " + str(incorrect))
    print("- unanswered: " + str(len(answers) - correct - incorrect))
    print("")
    pct = (correct / len(answers)) * 100
    print("You discovered " + str(pct) + "% of the answers")
    print("Execution time: " + str(end - start) + " seconds (" + str((end - start) / len(answers)) + " seconds per answer) (" + str((end - start) / 60) + " minutes)")

if __name__ == "__main__":
    # Parse args, the first argument is the shell script to execute, the second argument is the path to the answers file
    main(sys.argv[1], sys.argv[2])