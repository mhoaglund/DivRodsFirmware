#include <Adafruit_SSD1306.h>

// This #include statement was automatically added by the Particle IDE.
#include <HttpClient.h>
#include "application.h"

String steps[10];
String location_input = "";
String actual_location = "";

String group = "MIA3FB";
String user = "MAX";
String server = "ec2-52-205-211-230.compute-1.amazonaws.com";
int PORT = 18003;
String MODE = "track";
String MY_NAME = "Minnesota";

unsigned int nextTime = 0;    // Next time to contact the server
HttpClient http;

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

http_request_t request;
http_response_t response;

// SWITCH
unsigned int SLEEP = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("Starting!");
    Particle.variable("loc_actual", actual_location);
    Particle.function("loc_input", updatelocinput);
}

void loop() {
    if (nextTime > millis()) {
        return;
    }

    
    request.path = "/" + MODE;
    
    if(MODE == "learn"){
        request.hostname = server;
        request.port = PORT;
        request.path = "/" + MODE;
        request.body = "{\"group\":\""+group+"\",\"username\":\""+MY_NAME+"\",\"location\":\""+location_input+"\",\"wifi-fingerprint\":[";
    }
    else{
        request.hostname = "node-express-env.afdmv4mhpg.us-east-1.elasticbeanstalk.com/locate";
        request.body = "{\"group\":\""+group+"\",\"username\":\""+MY_NAME+"\",\"location\":\"DUNNO\",\"wifi-fingerprint\":[";
    }

    
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
    Serial.println(response.body);
    
    nextTime = millis() + 2000; // sends response every 5 seconds  (2 sec delay + ~3 sec for gathering signals)
}

///A user wants to configure this device to help FIND learn a location.
///The name of the location the user wants to track will come in here.
int updatelocinput(String command) {
    if(command == "off"){
        location_input = 'none';
        MODE = 'track';
        return 1;
    }
    else{
        location_input = command;
        MODE = 'learn';
        return 1;
    }
    
}