check: reducer_tree_test
	./reducer_tree_test

CXX=clang++
CXXFLAGS=-Weverything -Wno-c++98-compat -Werror -Wall -W -Wextra -Wswitch -Wimplicit-fallthrough -ggdb -O0 -std=c++20
fitness: fitness.cc

reducer_tree_test.o: reducer_tree_test.cc reducer_tree.h
reducer_tree_test: reducer_tree_test.o
	$(CXX) $< -o $@
