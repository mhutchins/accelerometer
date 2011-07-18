CFLAGS = -I/usr/lib/jvm/java-6-openjdk/include

all:    D2XX.class libD2XX.so

libD2XX.so:	D2XX.o ../ftdi/LIS302DL_D2XX.o
	gcc -shared -Wl,-soname,libD2XX.so.1 -o libD2XX.so D2XX.o ../ftdi/LIS302DL_D2XX.o ../ftdi/filter.o -lftd2xx

D2XX.class: D2XX.java
	javac D2XX.java

D2XX.h:	D2XX.class
	javah D2XX

D2XX.o:	D2XX.c D2XX.h

clean:
	rm -f D2XX.class D2XX.h D2XX.o libD2XX.so

install:
	cp libD2XX.so /lib
