all: my_malloc.a

my_malloc.a: my_malloc.o
	ar rcs my_malloc.a my_malloc.o

my_malloc.o: my_malloc.c
	gcc -c my_malloc.c -Wall -Wextra -Werror

clean: 
	rm -f *.o

fclean: clean
	rm -f my_malloc.a

re: fclean all
