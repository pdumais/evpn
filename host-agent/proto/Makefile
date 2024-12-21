all: build
PROTOC = protoc
PROTOS_PATH = ./

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<


build: gobgp.pb.o gobgp.grpc.pb.o attribute.pb.o attribute.grpc.pb.o capability.pb.o capability.grpc.pb.o
	ar rcs gobgp.a *.o