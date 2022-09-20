#! /bin/bash

export LD_LIBRARY_PATH=../build/output:$LD_LIBRARY_PATH

#rm lgraph_data -rf
#./simple_paper_wal $*


:<<!
rm lgraph_data -rf
./simple_paper_wal --durable=1 --mode=batch --vertex_num=1000000 --batch_num=10

rm lgraph_data -rf
./simple_paper_wal --durable=0 --mode=batch --vertex_num=1000000 --batch_num=10
!

:<<!
rm lgraph_data -rf
./simple_paper_wal --durable=1 --vertex_num=1000000

rm lgraph_data -rf
./simple_paper_wal --durable=0 --vertex_num=1000000
!

:<<!
rm lgraph_data -rf
./simple_paper_wal --durable=1 --mode=multi_thread --vertex_num=1000000 --thread_num=10 --batch_num=1

rm lgraph_data -rf
./simple_paper_wal --durable=0 --mode=multi_thread --vertex_num=1000000 --thread_num=10 --batch_num=1
!

rm lgraph_data -rf
./simple_paper_wal --durable=1 --mode=multi_thread --vertex_num=1000000 --thread_num=10 --batch_num=10

rm lgraph_data -rf
./simple_paper_wal --durable=0 --mode=multi_thread --vertex_num=1000000 --thread_num=10 --batch_num=10