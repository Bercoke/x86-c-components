main:thrd_pool.o main.o
	gcc thrd_pool.o main.o -o main -lpthread
thrd_pool.o:thrd_pool.c
	gcc -c thrd_pool.c -o thrd_pool.o 
main.o:main.c
	gcc -c main.c -o main.o
.PHONY:clean
clean:
	rm *.o main