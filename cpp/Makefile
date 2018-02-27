CFLAGS=-O3


pagerank_test: pagerank_test.cpp table.cpp pagerank.cpp table.h
	icpc -o pagerank_test pagerank_test.cpp table.cpp
pagerank: pagerank.cpp table.cpp table.h
	icpc -Wall -o pagerank pagerank.cpp table.cpp 

all-tests: all-tests.txt pagerank_test
	./pagerank_test all-tests.txt

small-test: small pagerank_test
	./pagerank_test small

medium-test: medium pagerank_test
	./pagerank_test medium

large-test: large pagerank_test
	./pagerank_test large

enormous-test: enormous pagerank_test
	./pagerank_test enormous

ginormous-test: ginormous pagerank_test
	./pagerank_test ginormous

clean:
	rm -f pagerank pagerank_test
