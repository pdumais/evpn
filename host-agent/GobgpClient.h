#pragma once
#include <memory>
#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "gobgp.grpc.pb.h"
#include "attribute.grpc.pb.h"
#include "DataPlane.h"

class GobgpClient
{
public:
    GobgpClient(std::shared_ptr<grpc::Channel> channel);
    void run();

private:
    std::unique_ptr<apipb::GobgpApi::Stub> mStub;
    DataPlane *mDataPlane;

    void watch_routes();
    void list_paths();
};
