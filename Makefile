CPPFLAGS=-g
LDFLAGS=-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
DEBUGFLAGS=-DDEBUG -fsanitize=address
GDBFLAGS=
SRC = $(shell find . -name "*.cpp")
SHDRSRC = $(shell find . -name "*.frag" -o -name "*vert")
SPV = $(SHDRSRC:%.vert=%.spv) $(SHDRSRC:%.frag=%.spv)
OBJ = $(SRC:%.cpp=%.o)
MAKEFLAGS += -j16
BIN=build/agnosiaengine

.PHONY: all
all: $(BIN)

.PHONY: run
run: $(BIN)
	./$(BIN)

.PHONY: gdb
gdb: LDFLAGS+=$(GDBFLAGS)
gdb: $(BIN)
	gdb -q $(BIN)
.PHONY: debug
debug: LDFLAGS+=$(DEBUGFLAGS)
debug: $(BIN)
	./$(BIN)

.PHONY: dep
dep: 
	sudo pacman -S gcc glfw glm shaderc libxi libxxf86vm gdb shaderc
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make dep: 	Make all required dependencies"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make gdb:	Make with GDB hooked in"
	@echo "make clean:	Clean all files"
	@echo "make run: 	Run the executable after building"

$(BIN): $(OBJ) $(SPV)
	mkdir -p build
	g++ $(CPPFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	g++ -c -g $< -o $@ $(LDFLAGS)

%.spv: %.frag
	glslc $< -o $@
%.spv: %.vert
	glslc $< -o $@

.PHONY: clean
clean:
	rm -rf build
	find . -name "*.o" -type f -delete
	find . -name "*.spv" -type f -delete
