#!/bin/bash

for x in 200000 300000 400000 500000 600000 700000 800000 900000 1000000 2000000 3000000 4000000 5000000 6000000 ; do
	 python ../python/pagerank_test.py barabasi-$x.txt > barabasi-$x-pr-p.txt
done
