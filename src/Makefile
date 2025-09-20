CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ_DIR = obj

SERVER_TARGET = server_executable
CLIENT_TARGET = client_executable

SERVER_SRCS = $(wildcard server/*.c) $(wildcard common/*.c)
CLIENT_SRCS = $(wildcard client/*.c) $(wildcard common/*.c)

SERVER_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SERVER_SRCS))
CLIENT_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(CLIENT_SRCS))

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) -pthread -o $@ $^

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)
