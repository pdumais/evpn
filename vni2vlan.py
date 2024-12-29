import os
import json
import sys

j = json.loads(sys.stdin.read())

searched_vni = int(sys.argv[1])
max_vlan = 1
if len(j) > 0:
    for tunnel in j[0]["tunnels"]:
        tunid = tunnel["tunid"]
        tunend = tunid
        if "tunidEnd" in tunnel:
            tunend = tunnel["tunidEnd"]
        index = searched_vni - tunid
        max_vlan = max(max_vlan, tunnel["vlan"] + (tunend - tunid))
        if searched_vni >= tunid and searched_vni <= tunend:
            print(tunnel["vlan"] + index)
            sys.exit()

print(max_vlan + 1)
