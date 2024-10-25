CC = gcc
CFLAGS = -I.
TARGET = machprog
OBJS = machprog.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS)
