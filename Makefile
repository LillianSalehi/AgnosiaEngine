CPPFLAGS=-std=c++23 -g
CFLAGS = -g
LDFLAGS=-lglfw -Ilib -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -ltinyobjloader -Ilib/imgui -DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
MAKEFLAGS += -j16
SRC = $(shell find . -name "*.cpp")
CSRC = $(shell find . -name "*.c")
SHDRSRC = $(shell find . -name "*.frag" -o -name "*vert")
SPV = $(SHDRSRC:%.vert=%.spv) $(SHDRSRC:%.frag=%.spv)
OBJ = $(SRC:%.cpp=%.o)
COBJ=$(CSRC:%.c=%.o)
BIN=build/agnosiaengine

.PHONY: all
all: $(BIN)

.PHONY: run
run: $(BIN)
	./$(BIN)

.PHONY: gdb
gdb: $(BIN)
	gdb -q $(BIN)
.PHONY: debug
debug: $(BIN)
	./$(BIN)

.PHONY: dep
dep: 
	sudo pacman -S gcc glfw glm shaderc libxi libxxf86vm gdb shaderc stb
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make dep: 	Make all required dependencies"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make gdb:	Make with GDB hooked in"
	@echo "make clean:	Clean all files"
	@echo "make run: 	Run the executable after building"

$(BIN): $(OBJ) $(COBJ) $(SPV)
	mkdir -p build
	g++ $(CPPFLAGS) -o $(BIN) $(OBJ) $(COBJ) $(LDFLAGS)

%.o: %.cpp
	g++ -c $(CPPFLAGS) $< -o $@ $(LDFLAGS)
%.o : %.c
	gcc -c $(CFLAGS) $< -o $@ $(LDFLAGS)
%.spv: %.frag
	glslc $< -o $@
%.spv: %.vert
	glslc $< -o $@

.PHONY: clean
clean:
	rm -rf build
	find . -name "*.o" -type f -delete
	find . -name "*.spv" -type f -delete
