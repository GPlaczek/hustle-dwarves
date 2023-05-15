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
	mpicc $(SOURCES_DWARF) $(FLAGS) -o $(BIN_DIR)/dwarf
	mpicc $(SOURCES_MUSEUM) $(FLAGS) -o $(BIN_DIR)/museum
	

clear: clean

clean:
	rm $(BIN_DIR)/*

tags: $(SOURCES_DWARF) $(SOURCES_MUSEUM) $(HEADERS_DWARF) $(HEADERS_MUSEUM)
	ctags -R .

run_museum: main Makefile tags
	mpirun -oversubscribe -np 1 ./$(BIN_DIR)/museum

run_dwarf: main Makefile tags
	mpirun -oversubscribe -np 1 ./$(BIN_DIR)/dwarf
