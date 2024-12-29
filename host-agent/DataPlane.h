#pragma once
#include <string>

class DataPlane
{
public:
    DataPlane();
    void add_vtep(const std::string &vtep_address, int vni);
    void remove_vtep(const std::string &vtep_address, int vni);
    void add_mac(const std::string &mac, const std::string &vtep_address, int vni);
    void remove_mac(const std::string &mac, const std::string &vtep_address, int vni);

private:
    std::string mBridge;
    std::string mVxlanDevice;

    bool mac_string_to_bytes(const std::string &mac, uint8_t *buf);
};