import os
import json
import sys

j = json.loads(sys.stdin.read())

searched_vni = int(sys.argv[1])
max_vlan=1
if (len(j) > 0):
    for tunnel in j[0]["tunnels"]:
        tunid = tunnel['tunid']
        max_vlan = max(max_vlan, tunnel["vlan"])
        if tunid == searched_vni:
            print(tunnel["vlan"])
            sys.exit()

print(max_vlan+1)