CC = gcc

CFLAGS = -Wall -Wextra -Iinclude

LIBS = -pthread -lrt

SRC = src/main.c \
      src/scanner.c \
      src/backup.c \
      src/monitor.c \
      src/ipc.c \
      src/stats.c \
      src/logger.c

EXEC = main

all:
	$(CC) $(CFLAGS) $(SRC) -o $(EXEC) $(LIBS)

scan:
	./$(EXEC) scan originales

monitor:
	./$(EXEC) monitor originales

daemon:
	./$(EXEC) daemon originales

stop:
	./$(EXEC) stop

clean:
	rm -f $(EXEC)