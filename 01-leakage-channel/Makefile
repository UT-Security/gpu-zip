CC:= gcc
override CFLAGS+= -O3 -D_POSIX_SOURCE -D_GNU_SOURCE -m64 -falign-functions=64 -Wno-unused-result -Wunused-variable -Wunused-but-set-variable -Wall
LIBS:= -lpthread -lrt -lm
UTILS:= ../util/util.o   ../util/imc-utils.o ../util/gpu-utils.o ../util/msr-utils.o ../util/amd-df-utils.o ../util/amd-gpu-utils.o

all: obj bin data driver

driver: obj/driver.o $(UTILS)
	$(CC) -o bin/$@ $^ $(LIBS)

obj/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

obj:
	mkdir -p $@

bin:
	mkdir -p $@

data:
	mkdir -p $@

clean:
	rm -rf bin obj
	rm -rf ../util/*.o

.PHONY: all clean
