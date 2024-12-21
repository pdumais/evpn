all: build rebuild-lab

SHELL := sudo /bin/bash

#
# Create
#
#

%.create-hosts:
	./create-host.sh $*

create-host-infra:
	ip link add name br-evpn type bridge
	ip link set br-evpn up

create-controller: 
	ip netns add evpn-controller
	ip link add veth-ctrl1 type veth peer name veth-ctrl2
	ip link set veth-ctrl1 master br-evpn
	ip link set veth-ctrl1 up
	ip link set veth-ctrl2 netns evpn-controller
	ip netns exec evpn-controller ip link set veth-ctrl2 name eth0
	ip netns exec evpn-controller ip a a 10.0.0.10/24 dev eth0
	ip netns exec evpn-controller ip link set eth0 up
	ip netns exec evpn-controller ip link set lo up
	mkdir -p logs
	nohup ip netns exec evpn-controller ./gobgpd -f gobgp-controller.conf > ./logs/gobgpd.log 2>&1 &
	

#
# Destroy
#
#
%.destroy-hosts:
	./destroy-host.sh $*

destroy-controller:
	- ip link del veth-ctrl1
	- ip netns del evpn-controller

kill-bgp:
	- killall gobgpd	
	- killall host_agent

#
# Main commands
#
#
build:
	cd host-agent && make

create-lab: create-host-infra create-controller 1.create-hosts 2.create-hosts 3.create-hosts
destroy-lab: kill-bgp destroy-controller 1.destroy-hosts 2.destroy-hosts 3.destroy-hosts
	- ip link del br-evpn

rebuild-lab: destroy-lab create-lab


# This is just to test. But we want this to be done by the BGP server
manual-fdb:
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1
	ip netns exec evpn-1 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1
	ip netns exec evpn-2 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.103 dev vxlan1
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.101 dev vxlan1
	ip netns exec evpn-3 bridge fdb append to 00:00:00:00:00:00 dst 10.0.0.102 dev vxlan1

test:
	./test-internal.sh 1 3 
	./test-internal.sh 2 3 
	./test-internal.sh 3 3 
	
install-gobgp:
	curl -L https://github.com/osrg/gobgp/releases/download/v3.32.0/gobgp_3.32.0_linux_amd64.tar.gz -O
	mkdir -p tmp
	mv ./gobgp*.tar.gz tmp/gobgp.tar.gz
	cd tmp; tar -zxf gobgp.tar.gz
	cp tmp/gobgp ./
	cp tmp/gobgpd ./


tail-logs:
	- tmux kill-session -t logsession
	tmux new-session -d -s logsession "tail -F ./logs/gobgpd.log"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-controller ./gobgp neighbor"
	tmux split-window -t logsession -h  "watch -t ip netns exec evpn-1 ./gobgp global rib -a evpn"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-2 ./gobgp global rib -a evpn"
	tmux split-window -t logsession -v "watch -t ip netns exec evpn-3 ./gobgp global rib -a evpn"
	tmux split-window -t logsession:0.1 -v "watch -t ip netns exec evpn-controller ./gobgp global rib -a evpn"
	
	#tmux resize-pane -t logsession:0.1 -y 70
	tmux attach-session -t logsession