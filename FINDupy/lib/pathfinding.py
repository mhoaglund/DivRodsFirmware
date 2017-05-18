import math

data = {
    "205":{"loc": (1013,1803), "edges":{"220":1, "206":1}},
    "206":{"loc": (1140,1812), "edges":{"205":1, "217":1}},
    "214":{"loc": (1284,1395), "edges":{"235":1, "215":1}},
    "215":{"loc": (1288,1530), "edges":{"214":1, "216":1}},
    "216":{"loc": (1278,1636), "edges":{"215":1}},
    "217":{"loc": (1217,1710), "edges":{"206":1}},
    "219":{"loc": (1009,1509), "edges":{"223":1, "222":1, "220":1}},
    "220":{"loc": (1010,1677), "edges":{"219":1, "222":1, "221":1}},
    "221":{"loc": (918,1683), "edges":{"222":1, "220":1}},
    "222":{"loc": (920,1603), "edges":{"223":1, "219":1, "220":1, "221":1}},
    "223":{"loc": (925,1503), "edges":{"224":1, "219":1, "222":1}},
    "224":{"loc": (937,1413), "edges":{"229":1, "223":1}},
    "225":{"loc": (1047,1413), "edges":{"226":1, "224":1}},
    "226":{"loc": (1162,1413), "edges":{"227":1, "225":1}},
    "227":{"loc": (1172,1286), "edges":{"237":1, "226":1, "228":1}},
    "229":{"loc": (881,1284), "edges":{"239":1, "224":1}},
    "235":{"loc": (1286,1283), "edges":{"214":1, "227":1, "236":1}},
    "236":{"loc": (1280,1170), "edges":{"250":1, "235":1}},
    "237":{"loc": (1121,1154), "edges":{"251":1, "250":1, "227":1}},
    "238":{"loc": (1034,1154), "edges":{"237":1, "239":1}},
    "239":{"loc": (931,1154), "edges":{"253":1, "229":9, "236":1}},
    "247":{"loc": (984,762), "edges":{"261":1, "255":1}},
    "250":{"loc": (1265,1009), "edges":{"254":1, "236":1}},
    "251":{"loc": (1108,1054), "edges":{"252":1, "237":1}},
    "252":{"loc": (1021,1054), "edges":{"251":1, "253":1, "255":1}},
    "253":{"loc": (918,1049), "edges":{"252":1, "239":1}},
    "254":{"loc": (1121,910), "edges":{"250":1, "255":1}},
    "255":{"loc": (1008,904), "edges":{"247":1, "252":1, "256":1, "254":1}},
    "256":{"loc": (919,934), "edges":{"255":1}},
    "259":{"loc": (1301,643), "edges":{"260":1}},
    "260":{"loc": (1152,643), "edges":{"259":1, "261":1}},
    "261":{"loc": (1001,647), "edges":{"260":1, "262":1, "247":1}},
    "262":{"loc": (854,634), "edges":{"261":1, "263":1, "275":1}},
    "263":{"loc": (732,691), "edges":{"262":1, "264":1}},
    "264":{"loc": (624,691), "edges":{"265":1, "263":1}},
    "265":{"loc": (508,691), "edges":{"264":1}},
    "275":{"loc": (792,508), "edges":{"280":1, "278":1, "262":1, "275b":1}},
    "275b":{"loc": (799,419), "edges":{ "275":1, "276":1}},
    "276":{"loc": (766,339), "edges":{"275b":1, "280":1}},
    "277":{"loc": (518,373), "edges":{"280":1, "278":1}},
    "278":{"loc": (434,341), "edges":{"277":1}},
    "280":{"loc": (623,503), "edges":{"264":1, "275":1, "277":1}}
}

def _get_heading(destination, location):
    angle = math.degrees(math.atan2(destination[1] - location[1], destination[0] - location[0]))
    bearing1 = (angle + 360) % 360
    bearing2 = (90 - angle) % 360
    print(bearing1)

def get_shortest_path(weighted_graph, start, end):
    """
    Calculate the shortest path for a directed weighted graph.

    Node can be virtually any hashable datatype.

    :param start: starting node
    :param end: ending node
    :param weighted_graph: {"node1": {"node2": "weight", ...}, ...}
    :return: ["START", ... nodes between ..., "END"] or None, if there is no
             path
    """

    # We always need to visit the start
    nodes_to_visit = {start}
    visited_nodes = set()
    # Distance from start to start is 0
    distance_from_start = {start: 0}
    tentative_parents = {}

    while nodes_to_visit:
        # The next node should be the one with the smallest weight
        current = min(
            [(distance_from_start[node], node) for node in nodes_to_visit]
        )[1]

        # The end was reached
        if current == end:
            break

        nodes_to_visit.discard(current)
        visited_nodes.add(current)

        edges = weighted_graph[current]["edges"]
        unvisited_neighbours = set(edges).difference(visited_nodes)
        for neighbour in unvisited_neighbours:
            neighbour_distance = distance_from_start[current] + \
                                 edges[neighbour]
            if neighbour_distance < distance_from_start.get(neighbour,
                                                            float('inf')):
                distance_from_start[neighbour] = neighbour_distance
                tentative_parents[neighbour] = current
                nodes_to_visit.add(neighbour)

    return _deconstruct_path(tentative_parents, end)


def _deconstruct_path(tentative_parents, end):
    if end not in tentative_parents:
        return None
    cursor = end
    path = []
    while cursor:
        path.append(cursor)
        cursor = tentative_parents.get(cursor)
    return list(reversed(path))

#print(get_shortest_path(data, '222', '259'))

print("---")

_get_heading(data["262"]["loc"],data["261"]["loc"])
_get_heading(data["275"]["loc"],data["262"]["loc"])
_get_heading(data["262"]["loc"],data["275"]["loc"])
