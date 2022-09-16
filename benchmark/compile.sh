g++ -fopenmp -std=c++11 -I../deps/fma-common -I../include -O3 -g -o $1 $1.cpp ../build/output/liblgraph.so
