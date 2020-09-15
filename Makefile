
cash: cash.o utilities.o functions.o
	mkdir -p obj
	g++ -g cash.o utilities.o functions.o -o cash
	mv *.o ./obj
	chmod +x cash

cash.o: ./src/cash.c
	g++ -c ./src/cash.c

utilities.o: ./src/utilities.c
	g++ -c ./src/utilities.c

functions.o: ./src/functions.c
	g++ -c ./src/functions.c

clean:
	rm -r ./obj
	rm ./cash