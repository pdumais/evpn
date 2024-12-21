#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string.h>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "GobgpClient.h"

using grpc::Channel;

int main(int argc, char **argv)
{
    auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    GobgpClient client(channel);
    client.run();
}