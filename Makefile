# ---------- OPTIONS ----------
# Complier
CC = cc

# Directories
BLD_DIR = build
OBJ_DIR = $(BLD_DIR)/obj

# ---------- FILES ----------
TARGET  = $(BLD_DIR)/xtray

OBJECTS = $(OBJ_DIR)/xtray.o

HEADERS = config.h

# ---------- FLAGS/INCLUDES ----------
LDFLAGS = -lX11
CFLAGS  = -std=c99 -pedantic -Wall -I/usr/include/freetype2

# ---------- RULES ----------
all: $(TARGET)

run:
	./$(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(BLD_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)


clean:
	rm -rf build

.PHONY: all clean run
