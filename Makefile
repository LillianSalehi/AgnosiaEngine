CPPFLAGS=-std=c++23 -g
CFLAGS = -g
LDFLAGS=-lglfw -Ilib -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -ltinyobjloader -lglslang -lglslang-default-resource-limits -Ilib/imgui -DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
MAKEFLAGS += -j16
SRC = $(shell find . -name "*.cpp")
CSRC = $(shell find . -name "*.c")
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
	
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make gdb:	Make with GDB hooked in"
	@echo "make clean:	Clean all files"
	@echo "make run: 	Run the executable after building"

$(BIN): $(OBJ) $(COBJ)
	mkdir -p build
	g++ $(CPPFLAGS) -o $(BIN) $(OBJ) $(COBJ) $(LDFLAGS)

%.o: %.cpp
	g++ -c $(CPPFLAGS) $< -o $@ $(LDFLAGS)
%.o : %.c
	gcc -c $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf build
	find . -name "*.o" -type f -delete

