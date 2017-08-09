all: sample2D

sample2D: backup0.cpp glad.c
	g++ -std=c++11 -o sample2D backup0.cpp glad.c -lpthread -lao -lmpg123 -lGL -lglfw -ldl

clean:
	rm sample2D
