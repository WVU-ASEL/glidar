CXX=g++
CXXFLAGS=-Wall -g -O0
LDFLAGS=-lglfw3 -lglew -lassimp
FRAMEWORKS=-framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo
MAGICK_CXXFLAGS=`Magick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`Magick++-config --ldflags --libs`
MAGICK=`Magick++-config --cppflags --cxxflags --ldflags --libs`

all: lidargl

lidargl: main.o mesh.o
	$(CXX) main.o mesh.o -o lidargl $(LDFLAGS) $(FRAMEWORKS) $(MAGICK_LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp $(MAGICK_CXXFLAGS)

mesh.o: mesh.cpp
	$(CXX) $(CXXFLAGS) -c mesh.cpp $(MAGICK_CXXFLAGS)

clean:
	rm -rf lidargl *.o
