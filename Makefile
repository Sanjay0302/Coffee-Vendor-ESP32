CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall -pthread
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3

SRC = coffee_server.c  # Replace with your source file name
OBJ = $(SRC:.c=.o)
EXEC = coffee_server

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)
