#define MAP_SIZE 70
#define EDGES 5

struct edge{
    String Name;
    int weight; //rename to distance
};
typedef struct edge Edge;

struct lineage{
    String NodeName;
    String ParentName;
};
typedef struct lineage Lineage;

struct distance{
    String Name;
    int distance;
};
typedef struct distance Distance;

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

// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
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

//Embarassments:
void pull(Node item, Node items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name == item.Name){
      items[i] = {"0",{0,0},{}}; //hideous
      break;
    }
  }
}

bool is_lineage_in_array(String _name, Lineage items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Lineage curr = items[i];
    if(curr.NodeName == _name){
      return true;
      break;
    }
  }
  return false;
}

String find_lineage_from_node_name(String _name,  Lineage items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Lineage curr = items[i];
    if(curr.NodeName == _name){
      return curr.ParentName;
      break;
    }
  }
  return "0";
}

void set_distance_from_start(String _name, int _distance, Distance items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Distance curr = items[i];
    if(curr.Name == _name){
      items[i].distance = _distance;
      break;
    }
  }
}

void set_parent(String _name, String _parent, Lineage items[]){
  for(int i=0; i<MAP_SIZE; i++){
    Lineage curr = items[i];
    if(curr.NodeName == _name){
      items[i].ParentName = _parent;
      break;
    }
  }
}

Node find_node_by_edge(Edge edge, Node items[]){
  Node empty = {"0", {0,0}, {}};
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name == edge.Name){
      return curr;
    }
  }
  return empty;
}

Node find_node_by_edge(String edge, Node items[]){
  Node empty = {"0", {0,0}, {}};
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = items[i];
    if(curr.Name == edge){
      return curr;
    }
  }
  return empty;
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
    if(curr.Name == name && curr.distance > -1){
      return curr.distance;
    }
  }
  return _default;
}

//END Embarassments

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

void distill_path(Lineage tentative_parents[], String _end){
  //if end isn't in tentative parents, path is impossible.
  if(!is_lineage_in_array(_end, tentative_parents)){
    return;
  }
  String cursor = _end;
  String path_string = "";
  int pathnodes = 0;
  while(cursor != "0"){
    path_string += cursor;
    path_string += ", ";
    pathnodes++;
    //python's dictionary get() returns a value when supplied with a key.
    cursor = find_lineage_from_node_name(cursor, tentative_parents);
  }
  //C funk below: reversing the path string
  String path[pathnodes];
  for(int i=0; i<pathnodes; i++){
    int substring_index = pathnodes - i;
    path[i] = getValue(path_string, ',', substring_index);
  }
}

void get_shortest_path(String _start, String _end){
  Node nodes_to_visit[MAP_SIZE];
  Node visited_nodes[MAP_SIZE];

  Distance distances_from_start[MAP_SIZE];
  Lineage tentative_parents[MAP_SIZE];

  int current_step = 0;
  int visited = 0;
  int current_neighbor_index = 0;

  //locate the start node and set its distance to 0, and prep distance and lineage arrays
  for(int i=0; i<MAP_SIZE; i++){
    Node curr = nodes_to_visit[i];
    Distance dcurr = {curr.Name, -1};
    if(curr.Name == _start){
      nodes_to_visit[0] = curr;
      current_neighbor_index++;
      dcurr.distance = 0;
    }
    distances_from_start[i] = dcurr;
    tentative_parents[i] = {curr.Name, "0"};
  }

  while(has_remaining_nodes(nodes_to_visit)){
    Node current = find_node_by_edge(get_min_edge(floorMap[current_step].edges), floorMap);
    //Pick next node: shortest weight from list of edge peers
    if(current.Name == _end) break;

    pull(current, nodes_to_visit);
    visited_nodes[current_step] = current;

    for(int i=0; i<EDGES; i++){
      Edge neighbor = current.edges[i];
      if(neighbor.Name != "0" && find_node_by_edge(neighbor, visited_nodes).Name == "0"){
        //found an unvisited edge...
        Edge neighbor_edge = get_edge_by_name(neighbor.Name, current.edges);
        int neighbor_distance = find_distance_by_name(neighbor.Name, distances_from_start, 0) + neighbor_edge.weight;
        if(neighbor_distance < find_distance_by_name(neighbor.Name, distances_from_start, 99))
          set_distance_from_start(neighbor.Name, neighbor_distance, distances_from_start);
          set_parent(neighbor.Name, current.Name, tentative_parents);
          nodes_to_visit[current_neighbor_index] = find_node_by_edge(neighbor.Name, floorMap);
          current_neighbor_index++;
        }
      }
      current_step++;
    }
    distill_path(tentative_parents, _end);
  } 