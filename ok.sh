#!/bin/bash

make
./linear_hashing 70 < set1.txt > linear_random_70_b
./linear_hashing 70 < set2.txt > linear_high_70_b
./linear_hashing 10 < set1.txt > linear_random_10_b
./linear_hashing 10 < set2.txt > linear_high_10_b
./extendible_hashing 70 < set1.txt > ext_random_70_b
./extendible_hashing 70 < set2.txt > ext_high_70_b
./extendible_hashing 10 < set1.txt > ext_random_10_b
./extendible_hashing 10 < set2.txt > ext_high_10_b


