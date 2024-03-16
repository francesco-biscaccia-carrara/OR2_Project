output : main.o tsp.o utils.o load.o display.o algorithm.o
	gcc main.o tsp.o utils.o load.o display.o algorithm.o -g -lm -o main && rm *.o

test : test.o tsp.o utils.o load.o display.o algorithm.o
	gcc test.o tsp.o utils.o load.o display.o algorithm.o -g -lm -o test && rm *.o

test.o: test.c
	gcc -c test.c

main.o: main.c
	gcc -c main.c

algorithm.o: src/algorithm.c include/algorithm.h
	gcc -c src/algorithm.c

display.o: src/display.c include/display.h
	gcc -c src/display.c

load.o: src/load.c include/load.h
	gcc -c src/load.c

tsp.o: src/tsp.c include/tsp.h
	gcc -c src/tsp.c

utils.o: src/utils.c include/utils.h
	gcc -c src/utils.c

clean:
	rm *.o main test