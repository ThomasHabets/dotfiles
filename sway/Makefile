CXX=clang++
CXXFLAGS=-g -Os -march=native -mtune=native
TARGETS=focus_tab_parent
all: $(TARGETS)
%: %.cc
	$(CXX) -I$(HOME)/opt/simdjson/include -L$(HOME)/opt/simdjson/lib -Wl,-rpath,$(HOME)/opt/simdjson/lib -o $@ $< -lsimdjson
clean:
	rm -f $(TARGETS)
