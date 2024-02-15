all: goenums.c lib.c
	gcc -Wall -Wextra -Werror -pedantic -g  -o goenums goenums.c lib.c
	./goenums types.sql types/types.go enums

clean:
	rm -f goenums