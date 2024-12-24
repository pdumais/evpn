#pragma once
#include <string>

class DataPlane
{
public:
    DataPlane();
    void add_vtep(const std::string &vtep_address, int vni);
    void remove_vtep(const std::string &vtep_address, int vni);

private:
    std::string mBridge;
    std::string mVxlanDevice;
};