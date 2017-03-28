
#include <SparkJson.h>
#include "SparkJson/ArduinoJson.h"
// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_SSD1306.h>
#include <vector>
#include <cmath>

//using namespace std;
#include "application.h"
#include <HttpClient.h>


class Room{
    public:
        String name;
        int pos[2];
        std::vector<int> ports;
};

class OrientationMap{
    public:
        String tstamp;
        unsigned int devoffset;
        std::vector<Room> rooms;
        
        // Computes the bearing in degrees from the point A(a1,a2) to
        // the point B(b1,b2). http://math.stackexchange.com/questions/1596513/find-the-bearing-angle-between-two-points-in-a-2d-space
        double getHeadingBetween(int a[], int b[]){
            double a1 = (double)a[0];
            double a2 = (double)a[1];
            double b1 = (double)b[0];
            double b2 = (double)b[1];
            static const double TWOPI = 6.2831853071795865;
            static const double RAD2DEG = 57.2957795130823209;
            // if (a1 = b1 and a2 = b2) throw an error 
            double theta = atan2(b1 - a1, a2 - b2);
            if (theta < 0.0)
                theta += TWOPI;
            return RAD2DEG * theta;
        }
        
        //TODO: given a desired room to head to and a computed geometric heading,
        //check for a list of ports in the current room and try to reconcile a heading
        //which points the user to the nearest doorway that will take them to the room they
        //need to head for.
        double headingNearestPort(double input){
            return input;
        }
        
        //Find room by name in vector
        Room findRoom(String _name){
            Room resp;
            for(int i = 0; i < rooms.size(); i++){
                if(rooms[i].name == _name){
                    resp = rooms[i];
                }
            }
            return resp;
        }
};

Room CURRENT_ROOM;
OrientationMap NAV_MAP;
unsigned int nextTime = 0;    // Next time to contact the server
HttpClient http;
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); 

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

unsigned char nl = '\n';
String DIVROD_API_HOST = "";
String TRACKING_HOST = "";
String GROUP = "DIVRODTEST";
String USER = "MAX";
String MODE = "/track";
String MY_NAME = "Minnesota";
String LOGGED = "encounters\":[";

String location_input = "";
String actual_location = "";
String myID = "";

http_request_t request;
http_response_t response;

void setup() {
    Time.zone(-6); //US central
    Serial.begin(9600);
    Serial1.begin(9600);
    myID = System.deviceID();
    NAV_MAP.tstamp = "0";
    
    Serial.println("Starting!");
    Particle.variable("loc_actual", actual_location);
    Particle.function("loc_input", updatelocinput);
    Particle.function("do_onboard", onboard);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    //getConfig();
    //In future, an RFID scan from the intern arduino will enact this.
    queueSelfForOnboarding();
    delay(2000);
}

///A user wants to configure this device to help FIND learn a location.
///The name of the location the user wants to track will come in here.
int updatelocinput(String command) {
    display.clearDisplay();
    display.setCursor(0,0);
    
    display.setTextColor(WHITE);
    if(command == "off" | command == ""){
        display.println("Tracking...");
        display.display();
        location_input = "";
        MODE = "/track";
        display.setTextSize(2);
        return 2;
    }
    else{
        display.println("Learning: " + command);
        display.display();
        location_input = command;
        MODE = "/learn";
        display.setTextSize(1);
        return 1;
    }
}

//Called by the utility API with user setup information.
//TODO: parse the setup string and get variables set.
int onboard(String command){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(command);
    display.display();
    delay(2000);
    return 5;
}

//master method to update variables and request a new set of steps to destination
//http://stackoverflow.com/questions/15517991/search-a-vector-of-objects-by-object-attribute
void updateHeading(String _destination){
    // actual_location = _destination; 
    // int r = 25;
    // Room _current_room;
    // CURRENT_ROOM = NAV_MAP.findRoom(_destination);
    // Serial1.write(r);
    // Serial1.write(nl);
}

///Given a desired destination gallery, determine a traversable path from a known current location.
void getWalkingPath(String desired, String actual){
    
}

//Call our REST api to queue this device for config push.
//Occurs when the onboarding RFID is scanned. Send unique ID and name.
void queueSelfForOnboarding(){
    http_request_t onb_request;
    http_response_t onb_response;
    onb_request.hostname = DIVROD_API_HOST;
    onb_request.port = 80;
    onb_request.path = "/onboard?deviceid=";
    onb_request.path = onb_request.path + myID;
    onb_request.path = onb_request.path + "&devicename=";
    onb_request.path = onb_request.path + MY_NAME;
    http.put(onb_request, onb_response, headers);
    char *s2 = strdup(onb_response.body);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(onb_response.body);
    display.display();
}

//wakeup method to pull configuration from utility API.
//puts our cartesian map of the museum into memory along with
//an offset to adapt the heading reading to it.
void getConfig(){
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    
    StaticJsonBuffer<2000> jsonBuffer;
    http_request_t conf_request;
    http_response_t conf_response;
    conf_request.hostname = DIVROD_API_HOST;
    conf_request.port = 80;
    conf_request.path = "/setup";
    http.get(conf_request, conf_response, headers);
    //Parse JSON object
    char *s2 = strdup(conf_response.body);
    JsonObject& configroot = jsonBuffer.parseObject(s2);
    if (!configroot.success()) {
        display.println("parseObject() failed");
        display.println(conf_response.body);
        display.display();
        return;
    }
    else{
        
        if (configroot.containsKey("timestamp"))
        {
            const char* _tstamp = configroot["timestamp"];
            String _stamp_actual(_tstamp);
            //NAV_MAP.tstamp = _stamp_actual; NEED A CAST
            display.println(_stamp_actual);
        }
        if (configroot.containsKey("deviceoffset"))
        {
            unsigned int _devoff = configroot["deviceoffset"];
            //NAV_MAP.devoffset = _devoff; NEED A CAST
            display.println(_devoff);
        }

        if (configroot.containsKey("rooms"))
        {
            JsonArray& roomArray = configroot["rooms"];
            for( int i =0; i < roomArray.size(); i++ ){
                JsonObject& _curr = roomArray[i];
                // Room _curr_room;
                // _curr_room.name = _curr["name"];
                // _curr_room.pos[0] = _curr["pos"][0];
                // _curr_room.pos[1] = _curr["pos"][1];
                _curr.printTo(display);

                // JsonArray& portArray = _curr["ports"];
                // for(int j = 0; j < portArray.size(); j++){
                //     _curr_room.ports.push_back(portArray[i]);
                // }
                //NAV_MAP.rooms.push_back(_curr_room);
            }
        }
    }
    display.display();
}

//Just scanned a piece!
void encounteredArtwork(char report[]){
    String when = (String)Time.now();
    LOGGED = LOGGED + "{\"acc\":"+report+"\",\"time\":"+when+"\"}";
}

//upon  returning to cradle, PUT the logged artwork and some other info in our repository
void putJourney(){
    LOGGED = LOGGED + "]";
    http_request_t conf_request;
    request.hostname = DIVROD_API_HOST;
    request.port = 80;
    request.path = "/putjourney";
    request.path = "/getconfig";
    request.body = "{\"devicename\":\""+MY_NAME+"\",\"loggedartworks\":"+LOGGED+"\"}";
    http.post(request, response, headers);
}

void loop() {
    static char inData[51]; // Allocate some space for the string
    static char inChar=-1; // Where to store the character read
    static int pos= 0; // Index into array; where to store the character
    //Listen for report from the arduino. Probably an RFID tag.
    while (Serial1.available()){
        display.setCursor(0,0);
        display.setTextColor(WHITE);
        display.println("Receiving serial report...");
        display.display();
        if (pos < 45){
            inChar = Serial1.read();
            inData[pos] = inChar;
            pos++;
            inData[pos] = 0;
            if (inChar == 0x0A || inChar == 0x0D){
                // If the command is 8, we scanned a piece.
                if (inData[0] == 0x08){
                    encounteredArtwork(inData);
                }
                pos = 0;
                inData[pos] = 0;
            }
        } else {
            pos = 0;
            inData[pos] = 0;
        }
    }

    if (nextTime > millis()) {
        return;
    }
    request.hostname = TRACKING_HOST;
    request.port = 18003;
    request.path = MODE;
    
    request.body = "{\"group\":\""+GROUP+"\",\"username\":\""+USER+"\",\"location\":\""+location_input+"\",\"wifi-fingerprint\":[";
    WiFiAccessPoint aps[20];
    int found = WiFi.scan(aps, 20);
    for (int i=0; i<found; i++) {
        WiFiAccessPoint& ap = aps[i];
        char mac[17];
        sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
         ap.bssid[0] & 0xff, ap.bssid[1] & 0xff, ap.bssid[2] & 0xff,
         ap.bssid[3] & 0xff, ap.bssid[4] & 0xff, ap.bssid[5] & 0xff);
        request.body = request.body + "{\"mac\":\"" + mac + "\",";
        float f = ap.rssi;
        String sf(f, 0);
        request.body = request.body + "\"rssi\":" + sf + "}";
        
        if (i < found -1 ) {
            request.body = request.body + ",";
        }
    }
    request.body = request.body + "]}";

    http.post(request, response, headers);

    StaticJsonBuffer<2000> jsonBuffer;
    char json[response.body.length()+1];
    String output = "";
    if(MODE == "/track" & response.body.length() > 1){
        strcpy(json, response.body.c_str());
        JsonObject& root = jsonBuffer.parseObject((char*)json);
        if (!root.success()) {
            output = "JSON parse failed.";
        }else{
            const char* msg = root["location"];
            output = msg;
            String msg_actual(msg);
            if(actual_location != msg_actual){
                updateHeading(msg_actual);
            }
            
        }
    }
    else{
        output = response.body;        
    }
    
    //Serial.println(response.body);
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextColor(WHITE);
    display.println(output);
    display.display();
    
    //update the heading, just random for now.

    nextTime = millis() + 2000; // sends response every 5 seconds  (2 sec delay + ~3 sec for gathering signals)
}