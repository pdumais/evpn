SOURCES=main.cpp GobgpClient.cpp DataPlane.cpp
all: build


install-deps:
	# Ubuntu
	sudo apt install -y libgrpc++-dev libgrpc-dev libprotobuf-dev protobuf-compiler grpc-proto protobuf-compiler-grpc libnl-3-dev libnl-route-3-dev

build:
	cd proto && make
	g++ -std=c++11 -I/usr/include/libnl3/ -I/usr/local/include -I./proto/ -I. -pthread -L/usr/local/lib $(SOURCES) proto/gobgp.a -o host_agent -lnl-3 -lnl-genl-3 -lnl-route-3 -lgrpc++ -lgrpc -lprotobuf -lgrpc++_reflection -ldl 