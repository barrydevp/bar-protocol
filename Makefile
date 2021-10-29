var1 := hello man

SHARED_DIR := ./shared
SHARED_SRC := $(shell find $(SHARED_DIR) -maxdepth 1 -name "*.c")
SHARED_OBJ := $(SHARED_SRC:%.c=%.o)

SERVER_DIR := ./server
SERVER_SRC := $(shell find $(SERVER_DIR) -maxdepth 1 -name "*.c")
SERVER_OBJ := $(SERVER_SRC:%.c=%.o)

CLIENT_DIR := ./client
CLIENT_SRC := $(shell find $(CLIENT_DIR) -maxdepth 1 -name "*.c")
CLIENT_OBJ := $(CLIENT_SRC:%.c=%.o)

all: server.out client.out
	echo "BUILD ALL"

server.out: $(SHARED_OBJ) $(SERVER_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SHARED_OBJ) $(SERVER_OBJ) -o server.out

client.out: $(SHARED_OBJ) $(CLIENT_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SHARED_OBJ) $(CLIENT_OBJ) -o client.out

