check: reducer_tree_test
	./reducer_tree_test

CXXFLAGS=-Werror -Wall -W -Wextra -ggdb -O3 -std=c++20
fitness: fitness.cc

reducer_tree_test.o: reducer_tree_test.cc reducer_tree.h
reducer_tree_test: reducer_tree_test.o
	$(CXX) $< -o $@
