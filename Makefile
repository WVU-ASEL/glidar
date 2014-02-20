CC=g++
CFLAGS=-Wall -g
LIBS=-lglfw3 -lglew -lassimp -g
FRAMEWORKS=-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
MAGICK_CFLAGS=`Magick++-config --cppflags --cxxflags`
MAGICK_LINKERFLAGS=`Magick++-config --ldflags --libs`
MAGICK=`Magick++-config --cppflags --cxxflags --ldflags --libs`

all: lidargl

lidargl: texture.o shader.o main.o
	$(CC) main.o shader.o texture.o -o lidargl $(LIBS) $(FRAMEWORKS) $(MAGICK_LINKERFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp $(MAGICK_CFLAGS)

texture.o: texture.cpp texture.h
	$(CC) $(CFLAGS) -c texture.cpp $(MAGICK_CFLAGS)

clean:
	rm -rf lidargl *.o
