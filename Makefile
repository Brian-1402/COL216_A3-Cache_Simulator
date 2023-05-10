all: simulate

simulate: Cache.cpp Cache.hpp
	g++ Cache.cpp Cache.hpp -o cache_simulate

run: 
	./cache_simulate test.txt

clean:
	rm cache_simulate