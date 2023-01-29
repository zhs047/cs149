all: default grade

default: main.cpp bfs.cpp
	g++ -I../ -std=c++17 -fopenmp -O3 -s -o bfs main.cpp bfs.cpp ../common/graph.cpp ref_bfs.o
grade: grade.cpp bfs.cpp
	g++ -I../ -std=c++17 -fopenmp -O3 -s -o bfs_grader grade.cpp bfs.cpp ../common/graph.cpp ref_bfs.o
clean:
	rm -rf bfs_grader bfs  *~ *.*~
