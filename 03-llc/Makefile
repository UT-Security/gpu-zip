GPP:= g++
CFLAGS:= -O1 -std=c++17 -D_POSIX_SOURCE -D_GNU_SOURCE -m64 -falign-functions=64 -Wno-unused-result -Wall -I ./../poc/library/glad/include -I ./../poc/library/utils
LIBS:= -lglfw -lGL -ldl
UTILS:= ./../poc/library/glad/src/glad.o ./../poc/library/utils/shader.o

all: obj bin texture 

texture: obj/texture.o $(UTILS)
	$(GPP) -o bin/$@ $^ $(LIBS)

obj/texture.o: texture.cpp pputil.c
	$(GPP) -c texture.cpp $(CFLAGS) -o obj/texture.o

obj:
	mkdir -p $@

bin:
	mkdir -p $@

clean:
	rm -rf bin obj 

.PHONY: all clean
