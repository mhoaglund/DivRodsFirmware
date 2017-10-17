// This #include statement was automatically added by the Particle IDE.
#include <SparkJson.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

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
        String color = "blue";
};

std::vector<Room> navSteps;
std::vector<String> recentLocations;

Goal navGoal;
int _steps = 0;
int _navsteptime = 1500;
int _fallbackTime = 160000;
int _recoveryTime = 20000;
int _navbounces = 0;
Timer fallbacktimer(_fallbackTime, toggleFreeMode);
Timer recoverytimer(_recoveryTime, toggleFreeMode);

unsigned int nextTime = 0;
HttpClient http;
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); 

ApplicationWatchdog wd(30000, wd_exit);

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
const char rgbflag = 'c';
const char sleepflag = 'z';
const char rainbowflag = 'r';

//rbg values to send over serial
const String orange = ".255.125.5"; //more like orange
const String purple = ".25.2.255";
const String red = ".255.5.50";
const String blue = ".2.50.255";
const String green = ".50.250.75";

//inbound
const char rfidflag = 'f';
const char statusflag = 'x';

boolean newData = false;
const byte numChars = 32;
char receivedChars[numChars];

int CHGPIN = D3;
bool isAsleep = false;
bool isInFreeMode = false;
bool hasSession = false;

void setup() {
    Time.zone(-6); //US central
    pinMode(CHGPIN, INPUT);  
    Serial.begin(9600);
    Serial1.begin(9600);
    myID = System.deviceID();
    Particle.variable("loc_actual", actual_location);
    cycleSession();
    hasSession = true;
    bool start_scan = sendScannedTag("/artwork", "artid=0&pref=n");
    fallbacktimer.start();
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
    if(command == waitflag){
        recoverytimer.start();
    }
    else{
        recoverytimer.stop();
    }
}

//TODO: reverse one of the diff calculations because the BNO has been flipped.
int calculateHeading(int from[2], int to[2]){
    int diff_y = from[1] - to[1];
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
    instructCoController(headingflag, heading);
}

void wd_exit(){
    instructCoController(errorflag, 0);
    System.reset();
}

//master method to update variables and request a new set of steps to destination
void updateNavigation(String _location){
    String the_goal = navGoal.room;
    if(_location.length() != 3){
        return;
    }
    if(navSteps.size() == 0){
        refreshPathJson(_location, the_goal);
    }
    int _stepindex = 99;
    bool _onTrack = false;
    bool _atEnd = false;
    if(isInFreeMode){
        //momentarily deviating from pathfinding to give the user a simpler task.
        instructCoController(rainbowflag, 0);
        _navsteptime = 30000;
    } else{
        //check if the gallery is in the current nav steps. if so:
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
                fallbacktimer.stop();
                fallbacktimer.reset();
                String _sendcolor = blue;
                if(navGoal.color == "orange") _sendcolor = orange;
                if(navGoal.color == "purple") _sendcolor = purple;
                if(navGoal.color == "red") _sendcolor = red;
                if(navGoal.color == "blue") _sendcolor = blue;
                if(navGoal.color == "green") _sendcolor = green;
                instructCoController(rgbflag, _sendcolor);
                _navsteptime = 30000; //slow down nav loop since we're at the destination.
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
            refreshPathJson(_location, the_goal);
        }
    }
    if(actual_location != _location){
        actual_location = _location;
        debounceLocation(_location);
    }
}

/*
    Common failure mode of nav is to keep giving the user opposite, contradictory directions.
    We try to catch that and go into wildcard mode here.
*/
void debounceLocation(String _location){
    //If we made it here, we have a location the user newly arrived in.
    if (std::find(recentLocations.begin(), recentLocations.end(), _location) != recentLocations.end())
    {
      //If they newly arrived, but it's one of the last two galleries they've been in, they are confused. Confusion++.
      _navbounces++;
      if(_navbounces > 3){
          if(!isInFreeMode){
            _navbounces = 0;
            toggleFreeMode();
          }
      }
    } else{
        _navbounces = 0;
        recentLocations.push_back(_location);
        //Keep length to 2
        if(recentLocations.size() > 2){
            recentLocations.erase(recentLocations.begin());
        }
    }
}

/*
    Retrieve a new path from the server based on current location and goal.
    Parses JSON response into Vector of Rooms.
*/
void refreshPathJson(String _location, String _destination){
    String qs = "&start=" + _location + "&end=" + _destination;
    StaticJsonBuffer<1800> jsonBuffer;
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
                return;
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
        }
    }
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

bool cycleSession(){
    String resp = getStringFromUAPI(UTILITY_HOST, 80, "/devices/cycle", "");
    return true;
}

void loop() {

    int val = digitalRead(CHGPIN);
    if(val == HIGH && WiFi.ready()){
        //cycleSession();
        fallbacktimer.stop();
        instructCoController(sleepflag, 0);
        System.sleep(CHGPIN, FALLING, 600);
    }
    if(!WiFi.ready()){
        return;
    }

    recvWithStartEndMarkers();
    if (newData == true) {
        newData = false;
        String temp(receivedChars);
        applySerialReport(temp);
    }
    if (nextTime > millis() | !WiFi.ready()) {
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
          if(scanned_target){ //they scanned what we thought they might. clear their path and reset nav loop time.
            instructCoController(successflag, 0);
            navSteps.clear();
          }
          _navsteptime = 1500;
          nextTime = millis() + _navsteptime; //kick the loop so scanning loop doesn't run too long
          if(isInFreeMode) isInFreeMode = false;
      }
}

bool sendScannedTag(String _path, String _query){
    String oride = (isInFreeMode) ? "&oride=1" : "&oride=0";
    StaticJsonBuffer<1500> jsonBuffer;
    http_request_t this_request;
    http_response_t this_response;
    this_request.hostname = UTILITY_HOST;
    this_request.port = 80;
    this_request.path = _path + "?deviceid=" + myID + "&" + _query + oride;
    http.get(this_request, this_response, headers);
    fallbacktimer.start();
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

//If a user has spent a good deal of time without scanning our intended tag, we need to go into a fallback state.
//Play a rainbow fade, scan anything.
void toggleFreeMode(){
    if(isInFreeMode) isInFreeMode = false;
    else isInFreeMode = true;
}
