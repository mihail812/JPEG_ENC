PATHTOLIB=-I/home/dan/opencv_build/opencv/3rdparty/include/opencl/1.2
.PHONY: all clean

all: encode

clean:
	rm -rf *.o encode

encode: decoder.o preprocessing.o muxer.o compression.o opencl_encode.o encoder.o main.o 
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall decoder.o preprocessing.o muxer.o compression.o opencl_encode.o encoder.o main.o -o encode -lOpenCL 

main.o: src/main.cpp
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/main.cpp

encoder.o: src/encoder.cpp include/encoder.h
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/encoder.cpp

opencl_encode.o: src/opencl_encode.cpp include/opencl_encode.h
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/opencl_encode.cpp

compression.o: src/compression.cpp include/compression.h
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/compression.cpp

muxer.o: src/muxer.cpp include/muxer.h
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/muxer.cpp

preprocessing.o: src/preprocessing.cpp include/preprocessing.h
	g++ -std=c++11 $(PATHTOLIB) -O3 -Wall -c src/preprocessing.cpp

decoder.o: src/decoder.cpp include/decoder.h
	g++ -std=c++11 -O3 -Wall -c src/decoder.cpp



