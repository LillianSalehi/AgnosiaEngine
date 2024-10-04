CC=gcc
CXX=g++
CPPFLAGS=-g

SRC=$(shell find . -name *.cpp)
OBJ=$(SRC:%.cpp=%.o)
BIN= build/placeholderengine


.PHONY: all
all: $(BIN)

.PHONY: dep
dep: 
	sudo pacman -S $(CC)
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make dep: 	Make all required dependencies"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make clean:	Clean all files"

$(BIN): $(OBJ)
	mkdir -p build
	$(CXX) $(CPPFLAGS) -o $(BIN) $(OBJ)

%.o: %.cpp
	g++ -c $< -o $@

.PHONY: clean
clean:
	rm -rf build
