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

class Goal{
    public:
        String room = "0";
        String color = "cyan";
};

std::vector<Room> navSteps;
Goal navGoal;
int _steps = 0;
int _navsteptime = 1500;

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
//String UTILITY_HOST = "faf5d997.ngrok.io";
String GROUP = "mia3f";
String USER = System.deviceID();
String MODE = "/track";

String location_input = "";
String actual_location = "";

String myID = "";

http_request_t request;
http_response_t response;

//outbound
const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
const char errorflag = 'e';
const char print_flag = 'p';
const char rgbflag = 'c';

//rbg values to send over serial
const String yellow = ".180.250.80";
const String purple = ".25.2.255";
const String red = ".255.5.50";
const String cyan = ".2.50.255";
const String green = ".50.250.75";

//inbound
const char rfidflag = 'f';
const char statusflag = 'x';

boolean newData = false;
const byte numChars = 32;
char receivedChars[numChars];

int int_goal = 276;

void setup() {
    Time.zone(-6); //US central
    Serial.begin(9600);
    Serial1.begin(9600);
    myID = System.deviceID();
    Particle.variable("loc_actual", actual_location);
    //refreshGoalJson("/artwork/default", "artid=0&pref=0");
    bool start_scan = sendScannedTag("/artwork", "artid=0&pref=n");
    int_goal = navGoal.room.toInt();
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

String getStringFromUAPI(String _host, int _port, String _path, String _query){
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = _host;
    this_request.port = _port;
    this_request.path = _path + "?deviceid=" + myID + "&location=" + actual_location + "&" + _query;
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

void wd_exit(){
    instructCoController(errorflag, 0);
    instructCoController(print_flag, "Photon Error...");
    System.reset();
}

//master method to update variables and request a new set of steps to destination
//TODO: rework to use standard strings. Something's up with the String object, and its
//not getting passed to our fucking functions.
void updateNavigation(String _location){
    String the_goal = navGoal.room;
    if(_location.length() != 3){
        return;
    }
    if(navSteps.empty()){
        instructCoController(waitflag, '0');
        refreshPathJson(_location, the_goal);
        //return;
    }
    int _stepindex = 99;
    bool _onTrack = false;
    bool _atEnd = false;
    //Path ONE: check if the gallery is in the current nav steps. if so:
    for(int i=0; i<navSteps.size(); i++){
        if(navSteps[i].name == _location){
            _stepindex = i;
            _onTrack = true;
            if(_stepindex == (navSteps.size()-1)) _atEnd = true;
            break;
        }
    }
    if(_onTrack){
        if(_atEnd){
            //Reached the destination gallery, display tag color...
            String _sendcolor = cyan;
            if(navGoal.color == "yellow") _sendcolor = yellow;
            if(navGoal.color == "purple") _sendcolor = purple;
            if(navGoal.color == "red") _sendcolor = red;
            if(navGoal.color == "cyan") _sendcolor = cyan;
            if(navGoal.color == "green") _sendcolor = green;
            instructCoController(rgbflag, _sendcolor);
            _navsteptime = 15000; //slow down nav loop since we're at the destination.
        }
        else{
            //Moving along the path, get the next step.
            _navsteptime = 1500;
            int targetstep = _stepindex + 1;
            updateHeading(navSteps[_stepindex], navSteps[_stepindex + 1]);
        }
    }
    else{ //get a new path based on where the user has ended up
        _navsteptime = 1500;
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

void refreshGoalJson(String _path, String _query){
    StaticJsonBuffer<1000> jsonBuffer;
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = UTILITY_HOST;
    this_request.port = 80;
    this_request.path = _path + "?deviceid=" + myID + "&" + _query;
    http.get(this_request, this_response, headers);
    if(this_response.body.length() > 0){
        char json[this_response.body.length()+1];
        strcpy(json, this_response.body.c_str());
        JsonObject& newgoal = jsonBuffer.parseObject((char*)json); 
        if (newgoal.success()) {
            if (!newgoal.containsKey("room"))
            {
                return;
            }
            Goal new_goal;
            new_goal.room = newgoal["room"];
            new_goal.color = newgoal["color"];
            navGoal = new_goal;
        }
    }
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
        applySerialReport(temp);
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
    }
    wd.checkin();
    nextTime = millis() + _navsteptime;
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial1.available() > 0 && newData == false) {
        rc = Serial1.read();

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

void applySerialReport(String serialcommand){
      if(serialcommand[0] == rfidflag){
          serialcommand.remove(0,1);
          char _pref = serialcommand.charAt(0);
          serialcommand.remove(0,1);
          bool scanned_target = sendScannedTag("/artwork", "artid=" + serialcommand + "&pref=" + _pref);
          if(scanned_target){ //they scanned what we thought they might. clear their path.
            instructCoController(successflag, 0);
            navSteps.clear();
            delay(500);
          }
      }
}

bool sendScannedTag(String _path, String _query){
    StaticJsonBuffer<1500> jsonBuffer;
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = UTILITY_HOST;
    this_request.port = 80;
    this_request.path = _path + "?deviceid=" + myID + "&" + _query;
    http.get(this_request, this_response, headers);
    if(this_response.body.length() > 0){
        char json[this_response.body.length()+1];
        strcpy(json, this_response.body.c_str());
        JsonObject& newgoal = jsonBuffer.parseObject((char*)json); 
        if (newgoal.success()) {
            if (!newgoal.containsKey("room"))
            { //must have scanned a non-targeted piece
                return false;
            }
            Goal new_goal;
            new_goal.room = newgoal["room"];
            new_goal.color = newgoal["color"];
            navGoal = new_goal;
            return true;
        }
    }
    return false;
}
