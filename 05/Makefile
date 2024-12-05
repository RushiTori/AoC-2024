NAME=PrintQueue

build:
	gcc main.c -o $(NAME).out -Wall -Wextra -Werror -g

start: build
	./$(NAME).out

debug: build
	gdb $(NAME).out

clean:
	rm $(NAME).out

.PHONY: build start debug clean
