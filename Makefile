all: simulate

simulate: Cache.cpp Cache.hpp
	g++ Cache.cpp Cache.hpp -o cache_simulate 
	# ./cache_simulate 64 1024 2 65536 8 test.txt
	# ./cache_simulate 1 8 2 8 2 test.txt

run: simulate
	# ./cache_simulate test.txt
	./cache_simulate 64 1024 2 65536 8 test.txt

run2: simulate
	./cache_simulate 16 512 2 512 2 test.txt


clean:
	rm cache_simulate