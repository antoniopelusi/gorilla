all: compile run

compile:
	gcc src/main.c -o Gorilla -std=c99 -I./include/ -L./lib/ -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

run:
	./Gorilla