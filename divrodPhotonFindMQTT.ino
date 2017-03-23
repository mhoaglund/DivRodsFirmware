// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

// SWITCH
unsigned int SLEEP = 0;
unsigned int GREENLIGHT = 0;
unsigned int REDLIGHT = 0;
unsigned int nextTime = 0;


String steps[10];
String location_input = "";
String actual_location = "";
String group = "DENSEDOG";
String user = "MAX";
String server = "ec2-54-209-226-130.compute-1.amazonaws.com";
String password = "qGzGmd";
MQTT client("ml.internalpositioning.com", 1883, rxcb);

void button_handler(system_event_t event, int duration, void* )
{
    if (!duration) { // just pressed
        RGB.control(true);
        if (SLEEP == 0) {
            RGB.color(0,255,0);
            GREENLIGHT = 1;
            SLEEP = 1; // sleep mode on
        } else {
            RGB.color(255,0,0);
            REDLIGHT = 1;
            SLEEP = 0; // sleep mode off
        }
    }
    else {    // just released
        RGB.control(false);
        GREENLIGHT = 0;
        REDLIGHT = 0;
    }
}

void setup() {
    
    Particle.variable("loc_actual", actual_location);
    Particle.function("loc_input", updatelocinput);
    RGB.control(false);
    client.connect(server,group,password);
    System.on(button_status, button_handler);

    if (client.isConnected()) {
        client.subscribe("DENSEDOG/location/#"); //is this the right thing or will we get a firehose of all devices?
    }
}

void loop() {

    // SWITCH
    if (REDLIGHT == 1) {
        RGB.color(255,0,0);
    }
    if (GREENLIGHT == 1) {
        RGB.color(0,255,0);
    }

    if (nextTime > millis()) {
        return;
    }

    if (client.isConnected()) {
        String body;
        body = "";
        WiFiAccessPoint aps[20];
        int found = WiFi.scan(aps, 20);
        for (int i=0; i<found; i++) {
            WiFiAccessPoint& ap = aps[i];
            char mac[17];
            sprintf(mac,"%02x%02x%02x%02x%02x%02x",
             ap.bssid[0] & 0xff, ap.bssid[1] & 0xff, ap.bssid[2] & 0xff,
             ap.bssid[3] & 0xff, ap.bssid[4] & 0xff, ap.bssid[5] & 0xff);
            body = body + mac;
            float f = -1*ap.rssi;
            if (f > 100) {
                f = 99;
            }
            String sf(f, 0);
            if (f < 10) {
                body = body + " " + sf;
            } else {
                body = body + sf;
            }
        }

        client.publish(group + "/track/" + user,body); // Use this for tracking
        // client.publish(group + "/learn/" + user + "/" + location_input,body); // Use this for learning

        nextTime = millis() + 2000; // sends response every 5 seconds  (2 sec delay + ~3 sec for gathering signals)

        // // SWITCH
        if (SLEEP == 1) {
            System.sleep(5);
        }
        client.loop();

    } else {
        client.connect(server,group,password);
    }
}

//TODO: determine what happens with flow here
///Receive MQTT response from the server and set location
void rxcb(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    String message(p);
    actual_location = message;
}

///A user wants to configure this device to help FIND learn a location.
///The name of the location the user wants to track will come in here.
int updatelocinput(String command) {
    location_input = command;
    return 1;
}

///Given a desired destination gallery, determine a traversable path from a known current location.
void getWalkingPath(String desired, String actual){
    
}
