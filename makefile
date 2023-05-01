# 046267 Computer Architecture - Winter 20/21 - HW #2

# Environment for C++
CXX = g++
CXXFLAGS = -std=c++11 -Wall

cacheSim: cacheSim.cpp
	$(CXX) -o cacheSim *.cpp
	
Cache.o: Cache.cpp
	$(CXX) -o Cache.cpp
	
.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
