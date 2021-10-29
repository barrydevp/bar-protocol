SHARED_DIR := ../shared
SHARED_SRC := $(shell find $(SHARED_DIR) -maxdepth 1 -name "*.c")
SHARED_OBJ := $(SHARED_SRC:%.c=%.o)

all: server.out
	echo "BUILD SERVER"

server.out: $(SHARED_OBJ) server.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SHARED_OBJ) server.o -o server.out

