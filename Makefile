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

NO_OF_DWARVES = 2
NO_OF_MUSEUMS = 2
NO_OF_PORTALS = 2

FLAGS=-DDEBUG -g
# FLAGS=-g

all: main tags

main: $(SOURCES_DWARF) $(SOURCES_MUSEUM) $(HEADERS_DWARF) $(HEADERS_MUSEUM) Makefile
	mpicc $(SOURCES_DWARF) $(FLAGS) -DTAG=\"DWARF\" -DPORTALS=$(NO_OF_PORTALS) -DDWARVES=$(NO_OF_DWARVES) -DMUSEUMS=$(NO_OF_MUSEUMS) -o $(BIN_DIR)/dwarf -Wall
	mpicc $(SOURCES_MUSEUM) $(FLAGS) -DTAG=\"MUSEUM\" -DPORTALS=$(NO_OF_PORTALS) -DDWARVES=$(NO_OF_DWARVES) -DMUSEUMS=$(NO_OF_MUSEUMS) -o $(BIN_DIR)/museum -Wall
	

clear: clean

clean:
	rm $(BIN_DIR)/*

tags: $(SOURCES_DWARF) $(SOURCES_MUSEUM) $(HEADERS_DWARF) $(HEADERS_MUSEUM)
	ctags -R .

run: main Makefile tags
	mpirun -oversubscribe -np $(NO_OF_MUSEUMS) ./$(BIN_DIR)/museum : -np $(NO_OF_DWARVES) ./$(BIN_DIR)/dwarf
#	mpirun -oversubscribe -np 1 ./$(BIN_DIR)/dwarf