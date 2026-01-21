CC = gcc
CFLAGS =  -pthread -Wall
TARGET = mts
SRC = mts.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
clean:
	rm -f $(TARGET) output.txt