#! /bin/bash

export LD_LIBRARY_PATH=../build/output:$LD_LIBRARY_PATH

rm lgraph_data -rf

./simple_paper_wal $1 $2
