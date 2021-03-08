CXX=clang++
CXXFLAGS=
TARGETS=focus_tab_parent next_tab
CXXFLAGS=-I$(HOME)/opt/simdjson/include -g -Os -march=native -mtune=native -Wall -Wpedantic
LDFLAGS=-L$(HOME)/opt/simdjson/lib -Wl,-rpath,$(HOME)/opt/simdjson/lib
all: swaylib.o $(TARGETS)
%.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<
%: %.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< swaylib.o -lsimdjson
clean:
	rm -f $(TARGETS)
