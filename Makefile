all: simulate

simulate: Cache.cpp Cache.hpp
	g++ Cache.cpp Cache.hpp -o cache_simulate 

run_trace: simulate
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace1.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace2.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace3.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace4.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace5.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace6.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace7.txt
	./cache_simulate 128 1024 2 65536 8 memory_trace_files/trace8.txt

clean:
	rm cache_simulate