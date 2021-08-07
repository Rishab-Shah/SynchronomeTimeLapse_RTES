CC = gcc
LDFLAGS = -O0 -pg -g -Wcpp -Wall -lpthread -lrt

run	: main.o camera_driver.o time_spec.o services.o
	$(CC) $(LDFLAGS) -o run main.o camera_driver.o time_spec.o services.o

main.o : main.c main.h camera_driver.h time_spec.h services.h
	$(CC) $(LDFLAGS) -c main.c

camera_driver.o	: camera_driver.c camera_driver.h
	 $(CC) $(LDFLAGS) -c camera_driver.c

time_spec.o	: time_spec.c time_spec.h
	 $(CC) $(LDFLAGS) -c time_spec.c
   
services.o	: services.c services.h
	 $(CC) $(LDFLAGS) -c services.c

clean :
	rm -f *.o *.d frames/* run gmon.out
