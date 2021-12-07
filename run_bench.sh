CC=bin/clang CXX=bin/clang++ cmake .
make

CILK_NWORKERS=4 ./bench
