/* sequential */
gcc tsp_seq.c -lm -o tsp_seq
time ./tsp_seq testcase/pr1002.txt > output/seq_1002.txt

/* pthread */
gcc tsp_pthread.c -lpthread -lm -o tsp_pthread
time ./tsp_pthread testcase/pr1002.txt > output/pthread_1002.txt 2

/* fork */
g++ tsp_fork.cc -lm -o tsp_fork
time ./tsp_fork testcase/pr1002.txt 2 > output/fork_1002.txt 