#include "GobgpClient.h"
#include <iostream>
#include <string>
#include <sstream>

std::string bufferToIpAddress(const char *buffer)
{
    std::ostringstream ipStream;
    ipStream << static_cast<unsigned int>(static_cast<unsigned char>(buffer[0])) << '.'
             << static_cast<unsigned int>(static_cast<unsigned char>(buffer[1])) << '.'
             << static_cast<unsigned int>(static_cast<unsigned char>(buffer[2])) << '.'
             << static_cast<unsigned int>(static_cast<unsigned char>(buffer[3]));

    return ipStream.str();
}

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
                apipb::EVPNInclusiveMulticastEthernetTagRoute route;
                apipb::EVPNMACIPAdvertisementRoute mac_route;

                if (path.nlri().UnpackTo(&route))
                {
                    apipb::PmsiTunnelAttribute pmsi;
                    apipb::AsPathAttribute aspath;
                    bool has_aspath = false;
                    bool has_pmsi = false;
                    int vni = 0;
                    std::string vtep;
                    std::cout << "EVPNInclusiveMulticastEthernetTagRoute" << std::endl;
                    for (auto a : path.pattrs())
                    {
                        if (a.UnpackTo(&pmsi))
                        {
                            vni = pmsi.label();
                            vtep = bufferToIpAddress(pmsi.id().c_str());
                            has_pmsi = true;
                        }

                        if (a.UnpackTo(&aspath))
                        {
                            if (aspath.segments().size() != 0)
                            {
                                // if the AS path is not empty, then it means the route is not a local one. So we should be handling this vtep
                                has_aspath = true;
                            }
                        }
                    }
                    if (has_aspath && has_pmsi)
                    {
                        if (path.is_withdraw())
                        {
                            this->mDataPlane->remove_vtep(vtep, vni);
                        }
                        else
                        {
                            this->mDataPlane->add_vtep(vtep, vni);
                        }
                    }
                }
                else if (path.nlri().UnpackTo(&mac_route))
                {
                    apipb::PmsiTunnelAttribute pmsi;
                    apipb::AsPathAttribute aspath;
                    bool has_aspath = false;
                    bool has_pmsi = false;
                    int vni = 0;
                    std::string vtep;
                    std::cout << "EVPNMACIPAdvertisementRoute" << std::endl;
                    std::string mac = mac_route.mac_address();
                    for (auto a : path.pattrs())
                    {
                        if (a.UnpackTo(&pmsi))
                        {
                            vni = pmsi.label();
                            vtep = bufferToIpAddress(pmsi.id().c_str());
                            has_pmsi = true;
                        }

                        if (a.UnpackTo(&aspath))
                        {
                            if (aspath.segments().size() != 0)
                            {
                                // if the AS path is not empty, then it means the route is not a local one. So we should be handling this vtep
                                has_aspath = true;
                            }
                        }
                    }
                    if (has_aspath && has_pmsi)
                    {
                        if (path.is_withdraw())
                        {
                            this->mDataPlane->remove_mac(mac, vtep, vni);
                        }
                        else
                        {
                            this->mDataPlane->add_mac(mac, vtep, vni);
                        }
                    }
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
