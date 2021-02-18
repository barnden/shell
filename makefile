CXX_FLAGS=-g -w -fsanitize=undefined,address -std=c++20

all: Tokenizer.o Parser.o PromptString.o Shell.o Terminal.o main.o
	g++ $(CXX_FLAGS) -o shell Tokenizer.o Parser.o Terminal.o Shell.o PromptString.o main.o -lreadline

main.o: main.cpp
	g++ $(CXX_FLAGS) -c main.cpp

Shell.o: Shell.h Shell.cpp
	g++ $(CXX_FLAGS) -c Shell.cpp

Terminal.o: Terminal.h Terminal.cpp
	g++ $(CXX_FLAGS) -c Terminal.cpp

Parser.o: Parser.h Parser.cpp
	g++ $(CXX_FLAGS) -c Parser.cpp

Tokenizer.o: Tokenizer.h Tokenizer.cpp
	g++ $(CXX_FLAGS) -c Tokenizer.cpp

PromptString.o: PromptString.h PromptString.cpp
	g++ $(CXX_FLAGS) -c PromptString.cpp

clean:
	rm -rf *.o *.out shell