// This #include statement was automatically added by the Particle IDE.
#include <SparkJson.h>

// This #include statement was automatically added by the Particle IDE.
#define _USE_MATH_DEFINES
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

ApplicationWatchdog wd(15000, wd_exit);

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { NULL, NULL }
};

unsigned char nl = '\n';
String UTILITY_HOST = "node-express-env.afdmv4mhpg.us-east-1.elasticbeanstalk.com";
//String UTILITY_HOST = "c3fc3d5f.ngrok.io";
String GROUP = "mia2f";
String USER = System.deviceID();
String MODE = "/track";

String location_input = "";
String actual_location = "";

String myID = "";

http_request_t request;
http_response_t response;

const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
const char errorflag = 'e';
const char print_flag = 'p';
const char rgbflag = 'c';

const char rfidflag = 'f';

boolean newData = false;
const byte numChars = 32;
char receivedChars[numChars];

String goal1 = "259";
String goal2 = "276";
int int_goal = 276;

void setup() {
    Time.zone(-6); //US central
    Serial.begin(9600);
    Serial1.begin(9600);
    myID = System.deviceID();
    
    Serial.println("Starting!");
    Particle.variable("loc_actual", actual_location);
    Particle.function("do_onboard", onboard);
    Particle.function("show_onboard", onboard_response);
    //refreshPathJson(goal1,current_goal);
    //refreshPathJson();
    refreshGoal();
    instructCoController(print_flag, "Hello from Photon!");
}

String postLocationFromUAPI(String _host, int _port, String _path, String _body){
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = _host;
    this_request.port = _port;
    this_request.path = _path + "?deviceid=" + myID;
    this_request.body = _body;
    http.post(this_request, this_response, headers);

    if(this_response.body.length() > 0){
        return this_response.body;
        } else return "";
}

String getStringFromUAPI(String _host, int _port, String _path){
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = _host;
    this_request.port = _port;
    this_request.path = _path + "?deviceid=" + myID + "&location=" + actual_location;
    //get deviceid in here
    http.get(this_request, this_response, headers);

    if(this_response.body.length() > 0){
        return this_response.body;
        } else return "";
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

int calculateHeading(int from[2], int to[2]){
    int diff_y = to[1] - from[1];
    int diff_x = from[0] - to[0];
    float rad_angle = atan2(diff_y, diff_x);
    int deg_angle = rad_angle * (180 / M_PI);
    int bearing = (90 - deg_angle) % 360;
    if(bearing < 0){
        bearing = 360 + bearing;
    }
    return bearing;
}

void updateHeading(Room current, Room next){
    int heading = calculateHeading(current.pos, next.pos);
    String headinginfo = "H " + String(heading) + " from " + current.name + " to " + next.name;
    instructCoController(print_flag, headinginfo);
    instructCoController(headingflag, heading);
    delay(1500);
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
            //Reached the destination gallery, display tag color...
            //instructCoController(rgbflag, '125.125.125');
            refreshGoal();
            String new_goal(int_goal);
            refreshPathJson(_location, new_goal);
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
    
void refreshGoal(){
    int_goal = getStringFromUAPI(UTILITY_HOST, 80, "/path/next").toInt();
    if(int_goal == 0){
        instructCoController(print_flag, "Empty goal gallery...");
        delay(1000);
    }
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
    recvWithStartEndMarkers();
    if (newData == true) {
        newData = false;
        String temp(receivedChars);
        applySerialCommand(temp);
    }
    if (nextTime > millis()) {
        return;
    }
    String output = "";
    String body_start = "{\"group\":\""+GROUP+"\",\"username\":\""+USER+"\",\"location\":\"somewhere\",\"time\":12309123,\"wifi-fingerprint\":[";
    body_start = body_start + gatherAPs();
    String _location = postLocationFromUAPI(UTILITY_HOST, 80, "/locate", body_start);
    
    if(_location != ""){
        String clean(_location);
        if(String(clean.charAt(0)) == "g"){
            clean.remove(0,1);
            updateNavigation(clean);
        }
        output = clean;
    }
    else{
        output = "No output.";
        instructCoController(waitflag, 0);
    }

    instructCoController(print_flag, output);
    wd.checkin();
    nextTime = millis() + 1500;
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void applySerialCommand(String serialcommand){
      if(serialcommand[0] == rfidflag){

      }
}
