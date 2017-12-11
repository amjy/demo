all:
	gcc -c async_job.c
	ar crv libajob.a async_job.o

test:
	gcc example.c -L. -lajob -o example
	./example

clean:
	rm -f *.a *.o example
