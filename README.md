# Introduction
## Problem statement
I want to create an environment where multiple hypervisors are running VMS that need to reside on the same L2 network. These hypervisors are connected 
together on a L3 network. This is similar to running VMs in a cloud environment such as GCP and wanting to create a L2 overlay between them. In this example, I have 3 hosts running 3 VMs in each.
![](vms.png "Simulation")

VM1,VM2 and VM3 are able to reach each other automatically after they are created since they are connected on the same bridge. Same thing for VM3-6 and VM7-9. But VM1-3 can't reach any of VM4-9. To solve this problem, we can create an overlay using vxlan.
![](vmsvxlan.png "Simulation")

This diagram shows that by adding a vxlan interface in the bridge, we are able to create an overlay network that makes br0 appear as one big distributed switch.

To do this, we need to create a vxlan interface. Only one of those is needed
```
ip link add vxlan1 type vxlan id 0 dev eth0 dstport 4789
ip link set vxlan1 master br0
ip link set vxlan1 up
```

Then, we need to add entries in the FDB to forward BUM traffic to the other vteps.
```
bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1
bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1
```
Doing this on each hypervisors will create a full-mesh of vtep connections. It can become difficult to manage as we add new hypervisors. For example, if we add a 4th hypervisor in this environment, then we need to go back on the 3 other machines and add an FDB entry. This can be solved with BGP-EVPN.

Note that BGP-EVPN is a bit overkill in this situation because the network relatively easy to manage since we only have one overlay. In a real world scenario, we would probably have multiple overlays to split traffic in different domains. For example, VM1,5,9 might need to be connected one network while the other VMS are connected on different networks. This is a scenario that I will cover another day.

## Solution
We can run gobgp, a BGP server that exposes a GRPC interface for management and monitoring, on each hypervisor to exchange EVPN routes. We need to build a client application that will communicate with gobgp to monitor the route advertisements and make the necessary changes in the FDB.

![](gobgp.png "Simulation")
In this diagram, the orange links represent a BGP channel and the green links are GRPC channels. The BGP topology here uses one master server and all instances that run on hypervisors will exchange routes with this master. We could also have done a full-mesh or ring topology but this is irrelevant for us at this moment. The important thing to note is that we're using ebgp, so each gobgp instances have their own AS.

When we create a new hypervisor, we have a script that creates the bridge and the vxlan interface, spins up the host-agent and gobgp. The script will then invoke the gobgp client tool to advertise the new vtep that was created. For example, if we add a fourth server:
```
gobgp global rib -a evpn add prefix 10.0.0.104/32 etag 1 rd 65004:100
```
This prefix will get added in the local RIB and advertised to the controller and then back down to all other hypervisors.

## Lab environment
To make this test easier to run, I am created a virtual environment using network namespaces in linux instead of using hypervisors and VMs. That simulation is a good proof of concept to show how it can work with virtual machines, but also shows that it can be used for container environments.

TODO: diagram of network namespaces


# Running the test
## Building the agent
These steps have been done on a ubuntu VM. The Makefile will use `apt install` to install the required dependencies. The host agent is a c++ application
that communicates with gobgp thru a grpc channel. It monitors changes in the RIB and creates FDB entries for vteps.
```
cd host-agent
make install-deps
make build
```

## Creating the virtual lab
make install-gobgp

TODO: show "make logs" to monitor



TODO: added bonus of evpn is Type2 routes to remove the need for ARP

TODO: Test after VMs are up
    local rib contains vtep
    other servers are now aware of the new prefix
    FDB on other servers and this one is all in sync
    can ping between VMs: only goes thru the bridge
    can ping across: We see vxlan traffic. And it is not broadcast.
    kill one server: RIB and FDB updated everywhere.


















gobgp global rib -a evpn add macadv 00:00:00:00:00:00 10.0.100.101 [esi <esi>] etag <etag> label <label> rd <rd> [rt <rt>...] [encap <encap type>] [default-gateway]

# Show routes
$ gobgp global rib -a evpn [macadv]

# Delete route
$ gobgp global rib -a evpn del macadv <mac address> <ip address> [esi <esi>] etag <etag> label <label> rd <rd>


sudo ip netns exec evpn-controller ./gobgp global rib -a evpn add prefix 10.0.0.101/32 etag 1 rd 65001:100
sudo ip netns exec evpn-controller ./gobgp global rib -a evpn add prefix 10.0.0.102/32 etag 1 rd 65002:100
sudo ip netns exec evpn-controller ./gobgp global rib -a evpn add prefix 10.0.0.103/32 etag 1 rd 65003:100


sudo ip netns exec evpn-controller ./gobgp global rib -a evpn
sudo ip netns exec evpn-1 ./gobgp global rib -a evpn
sudo ip netns exec evpn-2 ./gobgp global rib -a evpn
sudo ip netns exec evpn-3 ./gobgp global rib -a evpn

