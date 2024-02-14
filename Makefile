shell: shell.c
	gcc -Wall -Werror -o shell shell.c

clean:
	rm -f shell *~
