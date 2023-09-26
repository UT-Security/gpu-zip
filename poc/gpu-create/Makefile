GPP:= g++
CFLAGS:= -std=c++17 -O3 -D_POSIX_SOURCE -D_GNU_SOURCE -m64 -falign-functions=64 -Wno-unused-result -Wall -I ./../library/glad/include -I ./../library/utils 
LIBS:= -lglfw -lGL -ldl
UTILS:= ./../library/glad/src/glad.o ./../library/utils/shader.o 

all: obj bin texture 

texture: obj/texture.o $(UTILS)
	$(GPP) -o bin/$@ $^ $(LIBS)

obj/%.o: %.cpp
	$(GPP) -c $(CFLAGS) -o $@ $<

obj:
	mkdir -p $@

bin:
	mkdir -p $@

clean:
	rm -rf bin obj 

.PHONY: all clean
