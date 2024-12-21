#include "GobgpClient.h"

GobgpClient::GobgpClient(std::shared_ptr<grpc::Channel> channel)
    : mStub(apipb::GobgpApi::NewStub(channel))
{
    this->mDataPlane = new DataPlane();
}

void GobgpClient::run()
{
    this->watch_routes();
    // TODO: When returning here, it means the grpc connection got interrupted. We should reattempt.
}

void GobgpClient::list_paths()
{
    grpc::ClientContext context;
    apipb::ListPathRequest listPathRequest;
    apipb::ListPathResponse listPathResponse;
    apipb::Family *family = new apipb::Family();
    family->set_afi(apipb::Family_Afi_AFI_L2VPN);
    family->set_safi(apipb::Family_Safi_SAFI_EVPN);

    listPathRequest.set_table_type(apipb::TableType::GLOBAL);
    listPathRequest.set_allocated_family(family);

    auto listPathResponseReader = this->mStub->ListPath(&context, listPathRequest);
    while (listPathResponseReader->Read(&listPathResponse))
    {
        if (listPathResponse.has_destination())
        {
            std::cout << listPathResponse.destination().prefix() << std::endl;
        }
    }
    grpc::Status status = listPathResponseReader->Finish();
    if (!status.ok())
    {
        std::cout << "WatchEvent failed: " << status.error_message() << std::endl;
    }
}

void GobgpClient::watch_routes()
{
    grpc::ClientContext context;
    apipb::WatchEventRequest request;
    apipb::WatchEventResponse response;

    auto table = request.table().New();
    request.set_allocated_table(table);
    auto filter = table->add_filters();
    filter->set_type(apipb::WatchEventRequest_Table_Filter_Type_BEST);
    filter->set_init(true);

    auto reader = this->mStub->WatchEvent(&context, request);
    std::cout << "Waiting for event\n";
    while (reader->Read(&response))
    {
        std::cout << "event\n";
        // Process the response (e.g., print the route advertisements)
        if (response.has_table())
        {
            const auto &table = response.table();
            for (const auto &path : table.paths())
            {
                apipb::EVPNIPPrefixRoute routePrefix;
                if (path.nlri().UnpackTo(&routePrefix))
                {
                    for (auto a : path.pattrs())
                    {

                        // if the AS path is not empty, then it means the route is not a local one. So we should be handling this vtep
                        apipb::AsPathAttribute aspath;
                        if (a.UnpackTo(&aspath))
                        {
                            if (aspath.segments().size() != 0)
                            {
                                std::cout << "Accepting route advertisement: " << routePrefix.ip_prefix() << std::endl;
                                this->mDataPlane->add_vtep(routePrefix.ip_prefix());
                            }
                        }
                    }
                    // TODO: Handle delete and adds
                }
            }
        }
    }

    grpc::Status status = reader->Finish();
    if (!status.ok())
    {
        std::cout << "WatchEvent failed: " << status.error_message() << std::endl;
    }
}
