CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = my_ls
SRC = my_ls.c

all: $(TARGET)

$(TARGET): $(SRC)
  $(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
  rm -f $(TARGET)
