CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -pedantic

TARGET = grid_pathfinding
SOURCES = grid_pathfinding.cpp
DEPENDENCIES = \
	grid_pathfinding.cpp \
	algorithms/path_result.h \
	algorithms/brute_force.cpp \
	algorithms/dynamic_programming.cpp \
	algorithms/greedy.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(DEPENDENCIES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@


run: $(TARGET)
	./$(TARGET)
	
clean:
	rm -f $(TARGET)