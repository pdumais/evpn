import sys
import json
from jinja2 import Template
import random


colours = [
    "deeppink4",
    "indigo",
    "darkgreen",
    "crimson",
    "orangered",
    "cyan4",
    "Aquamarine4",
    "deepskyblue2",
    "chartreuse",
    "darkorchid",
    "mediumpurple4",
    "darkolivegreen4",
    "Bisque4",
    "firebrick3",
    "coral4",
    "x11green",
    "seagreen",
    "Blue",
    "deeppink",
    "darkred",
    "greenyellow",
    "goldenrod4",
    "darkmagenta",
    "BlueViolet",
    "gold1",
]

col_index = 0
data = {"hosts": {}, "colours": {}}


# with open(sys, "r") as template_file:
#    template_content = template_file.read()
template_content = sys.stdin.read()

with open("graph.json", "r") as data_file:
    raw_data = json.load(data_file)
    for k, v in raw_data.items():
        path = raw_data[k][0]
        neigh = path["neighbor-ip"]
        if neigh not in data["hosts"]:
            data["hosts"][neigh] = {"vteps": [], "nodes": []}

        if path["nlri"]["type"] == 3:
            vlan = path["nlri"]["value"]["etag"]
            for attr in path["attrs"]:
                if attr["type"] == 22:
                    vnid = str(attr["label"])
                    vtep = attr["tunnel-id"]
                    vtep_data = {"vtep": vtep, "vni": vnid, "vlan": vlan}
                    data["hosts"][neigh]["vteps"].append(vtep_data)
                    if vnid not in data["colours"]:
                        data["colours"][vnid] = colours[col_index]
                        col_index = col_index + 1

        if path["nlri"]["type"] == 2:
            mac_address = path["nlri"]["value"]["mac"]
            # vlan = path["nlri"]["value"]["etag"]
            for attr in path["attrs"]:
                if attr["type"] == 22:
                    vnid = str(attr["label"])
                    vtep = attr["tunnel-id"]
                    mac_data = {"mac": mac_address, "vtep": vtep, "vni": vnid}
                    data["hosts"][neigh]["nodes"].append(mac_data)


template = Template(template_content)
print(template.render(data=data))
