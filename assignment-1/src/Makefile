build:
	gcc -O3 -Wall -Wextra -pthread -o guessword \
        guessword.c -lcrypt -lpthread

clean:
	rm -f guessword

start: clean build
	./guessword