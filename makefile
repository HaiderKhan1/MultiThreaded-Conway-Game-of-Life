#the compiler
CC = gcc
#compiler flags:
CFLAGS = -lpthread -Wall
all: gol_data gol_task

gol_data: gol_data.c
	$(CC) -o gol_data gol_data.c $(CFLAGS)

gol_task: gol_task.c
	$(CC) -o gol_task gol_task.c $(CFLAGS)

clean:
	rm gol_data gol_task