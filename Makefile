test : main.o a.o b.o
	arm-buildroot-linux-gnueabihf-gcc -o test main.o a.o b.o

main.o : main.c
	arm-buildroot-linux-gnueabihf-gcc -c -o main.o main.c

a.o : a.c
	arm-buildroot-linux-gnueabihf-gcc -c -o a.o a.c

b.o : b.c
	arm-buildroot-linux-gnueabihf-gcc -c -o b.o b.c

clean:
	rm *.o test