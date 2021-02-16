all: Parser.o PromptString.o Shell.o Terminal.o main.o
	g++ -g -w -fsanitize=undefined,address -o shell -std=c++20 Parser.o Terminal.o Shell.o PromptString.o main.o -lreadline

main.o: main.cpp
	g++ -g -w -fsanitize=undefined,address -std=c++20 -c main.cpp -lreadline

Shell.o: Shell.h Shell.cpp
	g++ -g -w -fsanitize=undefined,address -std=c++20 -c Shell.cpp

Terminal.o: Terminal.h Terminal.cpp
	g++ -g -w -fsanitize=undefined,address -std=c++20 -c Terminal.cpp

Parser.o: Parser.h Parser.cpp
	g++ -g -w -fsanitize=undefined,address -std=c++20 -c Parser.cpp

PromptString.o: PromptString.h PromptString.cpp
	g++ -g -w -fsanitize=undefined,address -std=c++20 -c PromptString.cpp

clean:
	rm -rf *.o *.out shell