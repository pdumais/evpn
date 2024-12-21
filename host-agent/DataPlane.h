#pragma once
#include <string>

class DataPlane
{
public:
    DataPlane();
    void add_vtep(const std::string &vtep_address);
    void remove_vtep(const std::string &vtep_address);

private:
    std::string mBridge;
    std::string mVxlanDevice;
};