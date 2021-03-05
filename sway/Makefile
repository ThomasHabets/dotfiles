CXX=clang++
all: swaytabs # next_tab
swaytabs: swaytabs.cc
	$(CXX) -g -I$(HOME)/opt/simdjson/include -L$(HOME)/opt/simdjson/lib -Wl,-rpath,$(HOME)/opt/simdjson/lib -O3 -march=native -mtune=native  -o swaytabs swaytabs.cc -lsimdjson
#next_tab: next_tab.cc
#	$(CXX) -I$(HOME)/opt/simdjson/include -L$(HOME)/opt/simdjson/lib -Wl,-rpath,$(HOME)/opt/simdjson/lib -O3 -march=native -mtune=native -o $@ $< -lsimdjson
