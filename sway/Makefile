CXX=clang++
swaytabs: swaytabs.cc
	$(CXX) -I$(HOME)/opt/simdjson/include -L$(HOME)/opt/simdjson/lib -Wl,-rpath,$(HOME)/opt/simdjson/lib -O3 -march=native -mtune=native -lsimdjson -o swaytabs swaytabs.cc
