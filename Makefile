CC = cc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
LDFLAGS = 

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = test

MAIN_SRC = $(SRC_DIR)/main.c
EDITOR_SRC = $(SRC_DIR)/editor.c
NETWORK_SRC = $(SRC_DIR)/network.c

TARGET = $(BUILD_DIR)/koala
TEST_OT_TARGET = $(BUILD_DIR)/test_ot

all:
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(MAIN_SRC) $(EDITOR_SRC) -o $(TARGET) $(LDFLAGS)

run:
	./$(BUILD_DIR)/koala

help:
	@echo "KOALA Editor - Makefile targets:"
	@echo "  make          - Build the editor"
	@echo "  make all      - Build the editor (same as 'make')"
	@echo "  make test_ot  - Build and run OT tests"
	@echo "  make standalone - Build editor without networking"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run in standalone mode"
	@echo "  make host     - Build and run as host (port 8888)"
	@echo "  make client   - Build and run as client (localhost:8888)"
	@echo "  make format   - Format source code"
	@echo "  make help     - Show this help message"
