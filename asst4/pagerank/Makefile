all: default grade

default: page_rank.cpp main.cpp
	g++ -I../ -std=c++17 -fopenmp -g -O3 -o pr main.cpp page_rank.cpp ../common/graph.cpp ref_pr.a
grade: page_rank.cpp grade.cpp
	g++ -I../ -std=c++17 -fopenmp -g -O3 -o pr_grader grade.cpp page_rank.cpp ../common/graph.cpp ref_pr.a
clean:
	rm -rf pr pr_grader *~ *.*~
