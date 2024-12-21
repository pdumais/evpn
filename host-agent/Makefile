SOURCES=main.cpp GobgpClient.cpp DataPlane.cpp
all: build


install-deps:
	# Ubuntu
	sudo apt install -y libgrpc++-dev libgrpc-dev libprotobuf-dev protobuf-compiler grpc-proto protobuf-compiler-grpc

build:
	cd proto && make
	g++ -std=c++11 -I/usr/local/include -I./proto/ -I. -pthread -L/usr/local/lib $(SOURCES) proto/gobgp.a -o host_agent -lgrpc++ -lgrpc -lprotobuf -lgrpc++_reflection -ldl 