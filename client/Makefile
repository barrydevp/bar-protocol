SHARED_DIR := ../shared
SHARED_SRC := $(shell find $(SHARED_DIR) -maxdepth 1 -name "*.c")
SHARED_OBJ := $(SHARED_SRC:%.c=%.o)

all: client.out
	echo "BUILD CLIENT"

client.out: $(SHARED_OBJ) client.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SHARED_OBJ) client.o -o client.out

