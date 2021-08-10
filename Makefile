tsp_pthread: tsp_pthread.o
	gcc tsp_pthread.o -lm -pthread -o tsp_pthread
tsp_pthread.o: tsp_pthread.c
	gcc -c tsp_pthread.c
clean:
	rm -rf *.o