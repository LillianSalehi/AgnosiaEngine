CPPFLAGS=-g
LDFLAGS=-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
DEBUGFLAGS=-DDEBUG
SRC=$(shell find . -name *.cpp)
OBJ=$(SRC:%.cpp=%.o)

BIN=build/placeholderengine


.PHONY: all
all: $(BIN)

.PHONY: run
run: $(BIN)
	./$(BIN)
.PHONY: debug
debug: LDFLAGS+=$(DEBUGFLAGS)
debug: $(BIN)
	./$(BIN)
.PHONY: dep
dep: 
	sudo pacman -S gcc glfw glm shaderc libxi libxxf86vm
.PHONY: info
info: 
	@echo "make:		Build executable"
	@echo "make dep: 	Make all required dependencies"
	@echo "make debug: 	Make with Debug hooked in"
	@echo "make clean:	Clean all files"
	@echo "make run: 	Run the executable after building"

$(BIN): $(OBJ)
	mkdir -p build
	g++ $(CPPFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	g++ -c $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf build
	find . -name "*.o" -type f -delete
