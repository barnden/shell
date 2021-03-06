# Run make with -j flag to parallelize compilation
DEBUG=0
CXX_FLAGS=-g -w -fsanitize=undefined,address -std=c++20 -pipe
DBG_FLAGS=-D DEBUG_AST -D DEBUG_TOKEN

all: Commands.o Interpreter.o Parser.o PromptString.o Shell.o System.o Terminal.o Tokenizer.o
ifeq ($(DEBUG), 1)
	g++ $(CXX_FLAGS) $(DBG_FLAGS) -o shell *.o
else
	g++ $(CXX_FLAGS) -o shell *.o
endif

Commands.o: Commands.h Commands.cpp
	g++ $(CXX_FLAGS) -c Commands.cpp

Interpreter.o: Interpreter.h Interpreter.cpp
	g++ $(CXX_FLAGS) -c Interpreter.cpp

Parser.o: Parser.h Parser.cpp
	g++ $(CXX_FLAGS) -c Parser.cpp

PromptString.o: PromptString.h PromptString.cpp
	g++ $(CXX_FLAGS) -c PromptString.cpp

Shell.o: Shell.cpp
	g++ $(CXX_FLAGS) -c Shell.cpp

System.o: System.h System.cpp
	g++ $(CXX_FLAGS) -c System.cpp

Terminal.o: Terminal.h Terminal.cpp
	g++ $(CXX_FLAGS) -c Terminal.cpp

Tokenizer.o: Tokenizer.h Tokenizer.cpp
	g++ $(CXX_FLAGS) -c Tokenizer.cpp

clean:
	rm -rf *.o *.out shell