#!/bin/bash

icpc -qopenmp -qopt-report=5 -qopt-report-routine=pagerank -qopt-report-phase=vec,par,loop,openmp -O3 -o pagerank_test pagerank_test.cpp table.cpp
