CFLAGS = -Wall -Wpedantic -Wextra -std=c++17
LDFLAGS = -lD3D11 -lD3DCompiler

default: build run

build:
	g++ -g $(CFLAGS) main.cpp $(LDFLAGS) -o main.exe

debug:
	g++ -ggdb $(OPTIONS) main.cpp $(LDFLAGS) -o main.exe

run:
	./main.exe

clean:
	rm main.exe
