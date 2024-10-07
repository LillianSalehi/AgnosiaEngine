CPPFLAGS=-g
LDFLAGS=-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
DEBUGFLAGS=-DDEBUG -fsanitize=address
GDBFLAGS=
SRC = $(shell find . -name *.cpp)
OBJ = $(SRC:%.cpp=%.o)
VERTEX = $(src/shaders/%.vert=%.spv)
FRAGMENT = $(src/shaders/%.frag=%.spv)

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
	sudo pacman -S gcc glfw glm shaderc libxi libxxf86vm gdb glslc
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make dep: 	Make all required dependencies"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make clean:	Clean all files"
	@echo "make run: 	Run the executable after building"

$(BIN): $(OBJ) $(VERTEX) $(FRAGMENT)
	mkdir -p build
	g++ $(CPPFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	g++ -c -g $< -o $@ $(LDFLAGS)
%.spv: %.vert %.frag
	glslc $< -o $@

.PHONY: clean
clean:
	rm -rf build
	find . -name "*.o" -type f -delete
