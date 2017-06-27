// This #include statement was automatically added by the Particle IDE.
#include <SparkJson.h>

// This #include statement was automatically added by the Particle IDE.
#define _USE_MATH_DEFINES
#include <Adafruit_SSD1306.h>
#include <cmath>

#include "application.h"
#undef swap
#include <vector>
#include <HttpClient.h>

class Room{
    public:
        String name = "0";
        int pos[2] = {0,0};
};

std::vector<Room> navSteps;
int _steps = 0;

unsigned int nextTime = 0;
HttpClient http;
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); 

ApplicationWatchdog wd(20000, wd_exit);

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { NULL, NULL }
};

unsigned char nl = '\n';
String UTILITY_HOST = "node-express-env.afdmv4mhpg.us-east-1.elasticbeanstalk.com";
String TRACKING_HOST = "ec2-54-209-226-130.compute-1.amazonaws.com";
String GROUP = "mia2f";
String USER = System.deviceID();
String MODE = "/track";

String location_input = "";
String actual_location = "";
String _goal = "";

String myID = "";

http_request_t request;
http_response_t response;

const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
const char errorflag = 'e';
const char print_flag = 'p';
const char colorflag = 'c';

String goal1 = "259";
String goal2 = "276";
String current_goal = "276";
int int_goal = 276;

void setup() {
    Time.zone(-6); //US central
    Serial.begin(9600);
    Serial1.begin(9600);
    myID = System.deviceID();
    
    Serial.println("Starting!");
    Particle.variable("loc_actual", actual_location);
    Particle.function("loc_input", updatelocinput);
    Particle.function("do_onboard", onboard);
    Particle.function("show_onboard", onboard_response);
    current_goal = "276";
    //refreshPathJson(goal1,current_goal);
    //refreshPathJson();
    instructCoController(print_flag, "Hello from Photon!");
}

String parseLocation(String _host, int _port, String _path, String _body){
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = _host;
    this_request.port = _port;
    this_request.path = _path;
    this_request.body = _body;
    //get deviceid in here
    http.post(this_request, this_response, headers);

    if(this_response.body.length() > 0){
        //In a dense area, the response from FIND is big. Can't parse it with json.
        int _spot = this_response.body.indexOf("location:");
        if(_spot != -1){
            String _locationstring = this_response.body.substring(_spot+10, _spot+14);
            if(String(_locationstring.charAt(0)) == "g"){
                return _locationstring;
            }
            else return "";
        } else return "";
    }
}

void instructCoController(char command, String payload){
    Serial1.print('<');
    Serial1.print(command);
    Serial1.print(payload);
    Serial1.print('>');
}

void instructCoController(char command, int payload){
    Serial1.print('<');
    Serial1.print(command);
    Serial1.print(payload);
    Serial1.print('>');
}

//int calculateHeading(int from[2], int to[2]){
int calculateHeading(int from[2], int to[2]){
    int diff_y = from[1] - to[1];
    int diff_x = from[0] - to[0];
    float rad_angle = atan2(diff_y, diff_x);
    int deg_angle = rad_angle * 180 / M_PI;
    int bearing = (90 - deg_angle) % 360;
    return bearing;
}

void updateHeading(Room current, Room next){
    int heading = calculateHeading(current.pos, next.pos);
    String headinginfo = "H " + String(heading) + " from " + current.name + " to " + next.name;
    instructCoController(print_flag, headinginfo);
    instructCoController(headingflag, heading);
    delay(1500);
}
/*
    A user wants to configure this device to help FIND learn a location.
    The name of the location the user wants to track will come in here.
*/
int updatelocinput(String command) {

    if(command == "off" | command == ""){
        location_input = "";
        MODE = "/track";
        return 2;
    }
    else{
        location_input = command;
        MODE = "/learn";
        return 1;
    }
}

//Called by the utility API with user setup information.
//TODO: parse the setup string and get variables set.
int onboard(String command){
    delay(2000);
    return 5;
}

int onboard_response(String command){
    instructCoController(waitflag, 0);
    delay(2000);
    return 5;
}

void wd_exit(){
    instructCoController(errorflag, 0);
    instructCoController(print_flag, "Photon Error...");
    System.reset();
}

//master method to update variables and request a new set of steps to destination
//TODO: rework to use standard strings. Something's up with the String object, and its
//not getting passed to our fucking functions.
void updateNavigation(String _location){
    String the_goal(int_goal);
    if(_location.length() != 3){
        return;
    }
    if(navSteps.empty()){
        instructCoController(waitflag, '0');
        refreshPathJson(_location, the_goal);
        return;
    }
    int _stepindex = 99;
    bool _onTrack = false;
    bool _atEnd = false;
    bool _atStart = false;
    //Path ONE: check if the gallery is in the current nav steps. if so:
    for(int i=0; i<navSteps.size(); i++){
        if(navSteps[i].name == _location){
            _stepindex = i;
            _onTrack = true;
            if(_stepindex == 0){
                _atStart = true;
            }
            if(_stepindex == (navSteps.size()-1)){
                _atEnd = true;
            }
            break;
        }
    }
    if(_onTrack){
        if(_atEnd){
            //Reached the goal!
            instructCoController(successflag, '0');
            if(current_goal == goal1) current_goal = goal2;
            else if(current_goal == goal2) current_goal = goal1;
            refreshPathJson(_location, the_goal);
            //Play success routine, then put arduino in color signal mode
            //and wait for serial response from arduino.
            //Upon arrival of RFID and pref data:
                //Hit the pref engine to get a new goal
                //refreshGoal(artID, pref);
                //refreshPathJson(_location, current_goal);
        }
        else{
            //Moving along the path, get the next step.
            int targetstep = _stepindex + 1;
            updateHeading(navSteps[_stepindex], navSteps[_stepindex + 1]);
        }
    }
    else{ //get a new path based on where the user has ended up
        instructCoController(waitflag, '0');
        refreshPathJson(_location, the_goal);
    }
    actual_location = _location;
}
/*
    Retrieve a new path from the server based on current location and goal.
    Parses JSON response into Vector of Rooms.
*/
void refreshPathJson(String _location, String _destination){
    String _locs = "Path points: " + _location + ", " + _destination;
    instructCoController(print_flag, _locs);
    delay(800);
    String qs = "&start=" + _location + "&end=" + _destination;
    StaticJsonBuffer<1500> jsonBuffer;
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = UTILITY_HOST;
    this_request.port = 80;
    this_request.path = "/path?deviceid=" + myID + qs;
    
    http.get(this_request, this_response, headers);
    
    if(this_response.body.length() > 0){
        char json[this_response.body.length()+1];
        strcpy(json, this_response.body.c_str());
        JsonObject& newpath = jsonBuffer.parseObject((char*)json);
        if (newpath.success()) {
            if (!newpath.containsKey("steps"))
            {
                //No steps enclosed? API must have returned nothing.
                instructCoController(print_flag, "Pathfinding Call Returned Empty.");
                return;
            }else{
                instructCoController(print_flag, "Got path.");
                delay(500);
            }
            navSteps.clear();
            String _roomnames = "";
            int number_of_steps = newpath["steps"];
            navSteps.reserve(number_of_steps);
            JsonArray& _journeysteps = newpath["journey"].asArray();
            for(int i = 0; i< number_of_steps; i++){
                JsonObject& step = _journeysteps[i];
                Room _room_for_step;
                _room_for_step.name = step["room"].asString();
                _roomnames = _roomnames + step["room"].asString() + ", ";
                _room_for_step.pos[0] = step["coords"][0];
                _room_for_step.pos[1] = step["coords"][1];
                navSteps.push_back(_room_for_step);
            }
            instructCoController(print_flag, _roomnames);
            delay(800);
        }else{
            instructCoController(print_flag, "Path JSON Parse Failed.");
            delay(300);
        }
    }
}

void refreshPathJson(){
    StaticJsonBuffer<1500> jsonBuffer;
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = UTILITY_HOST;
    this_request.port = 80;
    this_request.path = "/path?deviceid=" + myID;
    
    http.get(this_request, this_response, headers);
    
    if(this_response.body.length() > 0){
        char json[this_response.body.length()+1];
        strcpy(json, this_response.body.c_str());
        JsonObject& newpath = jsonBuffer.parseObject((char*)json);
        if (newpath.success()) {
            if (!newpath.containsKey("steps"))
            {
                //No steps enclosed? API must have returned nothing.
                instructCoController(print_flag, "Pathfinding Call Returned Empty.");
                return;
            }else{
                instructCoController(print_flag, "Got path.");
                delay(500);
            }
            navSteps.clear();
            int number_of_steps = newpath["steps"];
            navSteps.reserve(number_of_steps);
            JsonArray& _journeysteps = newpath["journey"].asArray();
            for(int i = 0; i< number_of_steps; i++){
                JsonObject& step = _journeysteps[i];
                Room _room_for_step;
                _room_for_step.name = step["room"].asString();
                _room_for_step.pos[0] = step["coords"][0];
                _room_for_step.pos[1] = step["coords"][1];
                navSteps.push_back(_room_for_step);
            }
            String strnumb(number_of_steps);
        }else{
            instructCoController(print_flag, "Path JSON Parse Failed.");
            delay(300);
        }
    }
}
    
void refreshGoal(String _location){
    //interaction with session to get the location of the next artwork to go to.
    return;
}

//Call our REST api to queue this device for config push.
//Occurs when the onboarding RFID is scanned. Send unique ID and name.
void queueSelfForOnboarding(){
    http_request_t onb_request;
    http_response_t onb_response;
    onb_request.hostname = UTILITY_HOST;
    onb_request.port = 80;
    onb_request.path = "/onboard?deviceid=";
    onb_request.path = onb_request.path + myID;
    onb_request.path = onb_request.path + "&devicename=";
    http.put(onb_request, onb_response, headers);
}

//Just scanned a piece!
void encounteredArtwork(char report[]){
    return;
}

String gatherAPs(){
    String _out = "";
    WiFiAccessPoint aps[45];
    int found = WiFi.scan(aps, 45);
    for (int i=0; i<found; i++) {
        WiFiAccessPoint& ap = aps[i];
        char mac[17];
        sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
         ap.bssid[0] & 0xff, ap.bssid[1] & 0xff, ap.bssid[2] & 0xff,
         ap.bssid[3] & 0xff, ap.bssid[4] & 0xff, ap.bssid[5] & 0xff);
        _out = _out + "{\"mac\":\"" + mac + "\",";
        float f = ap.rssi;
        String sf(f, 0);
        _out = _out + "\"rssi\":" + sf + "}";
        
        if (i < found -1 ) {
            _out = _out + ",";
        }
    }
    _out = _out + "]}";
    return _out;
}

void loop() {
    if (nextTime > millis()) {
        return;
    }
    String output = "";
    String body_start = "{\"group\":\""+GROUP+"\",\"username\":\""+USER+"\",\"location\":\"somewhere\",\"time\":12309123,\"wifi-fingerprint\":[";
    body_start = body_start + gatherAPs();

    String _location = parseLocation(TRACKING_HOST, 18003, "/track", body_start);
    
    if(_location != ""){
        String clean(_location);
        if(String(clean.charAt(0)) == "g"){
            clean.remove(0,1);
            updateNavigation(clean);
        }
        
        output = clean;
    }
        
    if(output == ""){
        output = "No output.";
    }
    
    instructCoController(print_flag, output);
    wd.checkin();
    delay(250);
    nextTime = millis() + 1500;
}