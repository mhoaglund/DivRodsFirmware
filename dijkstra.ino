#define MAP_SIZE 70
#define EDGES 5

struct edge{
    String Name;
    int weight; //rename to distance
};
typedef struct edge Edge;

struct distance{
    String Name;
    int distance;
};
typedef struct distance Distance

struct node{
    String Name;
    int pos[2];
    Edge edges[EDGES];
};
typedef struct node Node;

Node floorMap[]={
  {"205",{1013,1803}, {{"220",1},{"206",1}}},
  {"206",{1140,1812}, {{"205",1},{"217",1}}},
  {"214",{1284,1395}, {{"235",1},{"215",1}}},
  {"215",{1288,1530}, {{"214",1},{"216",1}}},
  {"216",{1278,1636}, {{"215",1}}},
  {"217",{1217,1710}, {{"206",1}}},
  {"219",{1009,1509}, {{"223",1}, {"222",1}, {"220",1}}},
  {"220",{1010,1677}, {{"219",1}, {"222",1}, {"221",1}}},
  {"221",{918,1683}, {{"222",1}, {"220",1}}},
  {"222",{920,1603}, {{"223",1}, {"219",1}, {"220",1}, {"221",1}}},
  {"223",{925,1503}, {{"224",1}, {"219",1}, {"222",1}}},
  {"224",{937,1413}, {{"229",1}, {"223",1}}},
  {"225",{1047,1413}, {{"226",1}, {"224",1}}},
  {"226",{1162,1413}, {{"227",1}, {"225",1}}},
  {"227",{1172,1286}, {{"237",1}, {"226",1}, {"228",1}}},
  {"229",{881,1284}, {{"239",1}, {"224",1}}},
  {"235",{1286,1283}, {{"214",1}, {"227",1}, {"236",1}}},
  {"236",{1280,1170}, {{"250",1}, {"235",1}}},
  {"237",{1121,1154}, {{"251",1}, {"250",1}, {"227",1}}},
  {"238",{1034,1154}, {{"237",1}, {"239",1}}},
  {"239",{931,1154}, {{"253",1}, {"229",9}, {"236",1}}},
  {"247",{984,762}, {{"261",1}, {"255",1}}},
  {"250",{1265,1009}, {{"254",1}, {"236",1}}},
  {"251",{1108,1054}, {{"252",1}, {"237",1}}},
  {"252",{1021,1054}, {{"251",1}, {"253",1}, {"255",1}}},
  {"253",{918,1049}, {{"252",1}, {"239",1}}},
  {"254",{1121,910}, {{"250",1}, {"255",1}}},
  {"255",{1008,904}, {{"247",1}, {"252",1}, {"256",1}, {"254",1}}},
  {"256",{919,934}, {{"255",1}}},
  {"259",{1301,643}, {{"260",1}}},
  {"260",{1152,643}, {{"259",1}, {"261",1}}},
  {"261",{1001,647}, {{"260",1}, {"262",1}, {"247",1}}},
  {"262",{854,634}, {{"261",1}, {"263",1}, {"275",1}}},
  {"263",{732,691}, {{"262",1}, {"264",1}}},
  {"264",{624,691}, {{"265",1}, {"263",1}}},
  {"265",{508,691}, {{"264",1}}},
  {"275",{792,508}, {{"280",1}, {"278",1}, {"262",1}, {"275b",1}}},
  {"276",{766,339}, {{"275b",1}, {"280",1}}},
  {"277",{518,373}, {{"280",1}, {"278",1}}},
  {"278",{434,341}, {{"277",1}}},
  {"280",{623,503}, {{"264",1}, {"275",1}, {"277",1}}},
  {"275b",{799,419}, {{ "275",1}, {"276",1}}}
};

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

void get_shortest_path(String _start, String _end){
  Node nodes_to_visit[MAP_SIZE];
  Node visited_nodes[MAP_SIZE];
  Node tentative_parents[MAP_SIZE];
  Distance distances_from_start[MAP_SIZE]; //wrong construct for this
  distances_from_start[0] = {_start, 0};
  //locate the start node:
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = nodes_to_visit[i];
    if(curr.Name == _start){
      nodes_to_visit[0] = curr;
      break;
    }
  }

  int current_step = 0;
  int visited = 0;

  while(has_remaining_nodes(nodes_to_visit)){
    Node current = find_node_by_edge(get_min_edge(floorMap[current_step]), floorMap)
    //Pick next node: shortest weight from list of edge peers
    if(current.Name == _end) break;

    pull(current, nodes_to_visit);
    visited_nodes[current_step] = current;

    //TODO: diff edges of current node with visited nodes
    for(int i=0; i<EDGES; i++){
      Edge curr = edges[i];
      if(find_node_by_edge(curr, visited_nodes) == NULL){
        //found an unvisited edge...
        Edge neighbor_edge = get_edge_by_name(curr.Name, current.edges);
        int neighbor_distance = find_distance_by_name(curr.Name, distances_from_start, 0) + neighbor_edge.weight;
        if(neighbor_distance < find_distance_by_name(curr.Name, distances_from_start, 99)){
          //distance_from_start[neighbour] = neighbour_distance
          //t0entative_parents[neighbour] = current
          //nodes_to_visit.add(neighbour)
        }
      }
    }


    current_step++;
  } 
}

boolean has_remaining_nodes(Node items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name != "0"){
      return true;
    }
  }
  return false;
}



void pull(Node item, Node items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name == item.Name){
      items[i] = {"0",{0,0},{}}; //hideous
      break;
    }
  }
}

Node find_node_by_edge(Edge edge, Node items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name == edge.Name){
      return curr;
    }
  }
  return NULL;
}

Edge get_edge_by_name(String name, Edge edges[]){
  Edge found = {"0", 0};
  for(int i=0; i<EDGES; i++){
    Edge curr = edges[i];
    if(curr.Name == name) return curr;
  }
  return found;
}

int find_distance_by_name(String name, Distance items[], int _default){
  for(int i=0; i<MAP_SIZE; i++){
    Distance curr = items[i];
    if(curr.Name == name){
      return curr.distance;
    }
  }
  return _default; //or 99?
}

Edge get_min_edge(Edge edges[]){
  int localmin = 99;
  Edge found;
  for(int i=0; i<EDGES; i++){
    Edge curr = edges[i];
    if(curr.weight == 0) continue;
    else{
      if(curr.weight < localmin){
        found = curr;
      }
    }
  }
  return found;
  //given an array of edges, find the one with least weight and return it.
}

