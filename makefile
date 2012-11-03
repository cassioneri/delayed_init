all : delayed_init

delayed_init : delayed_init.cpp delayed_init.h
	$(CXX) --version
	$(CXX) $(CXXFLAGS) -std=c++11 -Wall -pedantic -O4 -o $@ $<

.PHONY : clean
clean :
	rm -f delayed_init
