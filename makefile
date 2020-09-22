all: sim_disk.cpp
	g++ sim_disk.cpp main.cpp -o main
all-GDB: sim_disk.cpp
	g++ -g sim_disk.cpp -o main