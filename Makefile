SOURCES_DIR = src
DWARF_SRC_DIR = $(SOURCES_DIR)/dwarf
MUSEUM_SRC_DIR = $(SOURCES_DIR)/museum

SOURCES = $(wildcard $(SOURCES_DIR)/*.c)
SOURCES_DWARF = $(SOURCES) $(wildcard $(DWARF_SRC_DIR)/*.c)
SOURCES_MUSEUM = $(SOURCES) $(wildcard $(MUSEUM_SRC_DIR)/*.c)

# HEADERS = $(SOURCES:.c=.h)
HEADERS_DWARF = $(SOURCES_DWARF:.c=.h)
HEADERS_MUSEUM = $(SOURCES_MUSEUM:.c=.h)

BIN_DIR=bin

FLAGS=-DDEBUG -g
# FLAGS=-g

all: main tags

main: $(SOURCES_DWARF) $(SOURCES_MUSEUM) $(HEADERS_DWARF) $(HEADERS_MUSEUM) Makefile
	mpicc $(SOURCES_DWARF) $(FLAGS) -DTAG=\"DWARF\" -o $(BIN_DIR)/dwarf -Wall
	mpicc $(SOURCES_MUSEUM) $(FLAGS) -DTAG=\"MUSEUM\" -o $(BIN_DIR)/museum -Wall
	

clear: clean

clean:
	rm $(BIN_DIR)/*

tags: $(SOURCES_DWARF) $(SOURCES_MUSEUM) $(HEADERS_DWARF) $(HEADERS_MUSEUM)
	ctags -R .

run: main Makefile tags
	mpirun -oversubscribe -np 3 ./$(BIN_DIR)/museum : -np 3 ./$(BIN_DIR)/dwarf
