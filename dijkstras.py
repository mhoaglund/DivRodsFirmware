import json
from pprint import pprint

with open('2fnodes.json') as data_file:    
    data = json.load(data_file)

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

print(get_shortest_path(data, '222', '259'))
