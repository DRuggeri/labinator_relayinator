#include <Arduino.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Button2.h>

// NOTE: backwards from board which counts up from right to left where this counts up from left to right
const int pins[8] = { 13, 12, 14, 27, 26, 25, 33, 32 };
const byte numChars = 64;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define TOGGLE_TIME 250
#define POWER_TIME 5500
#define ON 1
#define OFF 0

char receivedChars[numChars];
int status[8] = {OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF};
unsigned long offstack[8][2] = { {0, OFF}, {0, OFF}, {0, OFF}, {0, OFF}, {0, OFF}, {0, OFF}, {0, OFF}, {0, OFF} };
std::string bootMessages[6] = {"", "", "", "", "", ""};
Adafruit_SSD1306 display;
Button2 button = Button2(0);
int selected = -1;

bool ready = false;
const long messageIntervalMin = 100;
const long messageIntervalMax = 1100;
const long range = messageIntervalMax - messageIntervalMin + 1;
long messageInterval = 1000;
unsigned long previousMillis = 0;
std::vector<std::string> messages = {
    "Reticulating splines",
    "PROCESSING!!!",
    "Calculating Pi",
    "Dividing by zero",
    "Generating a soul",
    "Fluxing capacitor",
    "Belonging your base",
    "Charging the lasers",
    "Entering hyperdrive",
    "Hitting the NOS",
    "Activating turbo",
    "Combing mustache",
    "Reloading",
    "Flexing",
    "Firing engines",
    "Boiling the ocean",
    "Simulating waves",
    "Compressing",
    "Amplifying EMP",
    "Imagining friends",
    "Experiencing love",
    "Fitting in",
    "Seeking meaning",
    "Stimulating nerves",
    "Laughing at you",
    "Powdering hand",
    "Spreading malware",
    "Engaging in thought",
    "Targeting",
    "Sparking the plug",
    "Clearing tubes",
    "Fanning flames",
    "Pushing buttons",
    "Pressing luck",
    "Enhancing",
    "Trying patience",
    "Infecting network",
    "Completing mission",
};

boolean newData = false;

void setMessage(const char *message, int x, int y) {
    display.setCursor(x, y);
    display.setTextSize(1);
    display.print(message);
}

void updateBoot(std::string message) {
    display.clearDisplay();
    bootMessages[0] = bootMessages[1];
    bootMessages[1] = bootMessages[2];
    bootMessages[2] = bootMessages[3];
    bootMessages[3] = bootMessages[4];
    bootMessages[4] = bootMessages[5];
    bootMessages[5] = message;
    for ( int i = 0 ; i < 6 ; i++ ) {
        display.setCursor(3, i * 10);
        display.print(bootMessages[i].c_str());
    }
    display.display();
}

const int sel_coords[8][2] = {
    {1, 17},
    {1, 29},
    {1, 41},
    {1, 53},
    {79, 17},
    {79, 29},
    {79, 41},
    {79, 53},
};
void printStatus() {
    display.clearDisplay();
    int base = 19;

    setMessage("    Power Status    ", 3, 0);
    display.drawLine(1,13 , 127,13 , WHITE);
    for (int i = 0; i < 4 ; i++) {
        std::stringstream line;
        line << "P";
        line << i+1;
        line << "  ";
        if (status[i] == OFF) line << "Off";
        else line << " On";
        line << "      P";
        line << i+5;
        line << "  ";
        if (status[i+4] == OFF) line << "Off";
        else line << " On";
        setMessage(line.str().c_str(), 4, base + i * 12);
        //Serial.printf("Setting line %d: %s\n", i, line.str().c_str());
    }

    if (selected > -1 && selected < 8) {
        display.drawRect(sel_coords[selected][0], sel_coords[selected][1], 16, 11, WHITE);
    }
    display.display();
}

void nextOption(Button2& btn) {
    ready = true;
    selected++;
    if (selected >= 8) {
        selected = 0;
    }

    printStatus();
}

void toggleOption(Button2& btn) {
    if (selected >= 0 && selected < 8) {
        ready=true;
        digitalWrite(pins[selected], ON);
        offstack[selected][0] = millis() + (status[selected] == OFF ? TOGGLE_TIME : POWER_TIME);
        offstack[selected][1] = status[selected] == OFF ? ON : OFF;

        printStatus();
    }
}

void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 8; i++ ) {
        pinMode(pins[i], OUTPUT);
    }
    Serial.println("starting");

    Wire.begin();
    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.setTextColor(WHITE, BLACK);
    display.clearDisplay();

    button.setClickHandler(nextOption);
    button.setLongClickDetectedHandler(toggleOption);
    button.setDoubleClickTime(10);
    button.setLongClickTime(500);

    digitalWrite(pins[6], ON);
    offstack[6][0] = millis() + TOGGLE_TIME;
    offstack[6][1] = ON;
    
    Serial.println("ready");
}


void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (rc != endMarker) {
            if (rc == '\r') return;
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0';
            ndx = 0;
            newData = true;
        }
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

void processData() {
    if (newData == true) {
        std::string input = receivedChars;
        std::vector<std::string> parts = split(input, ' ');
        if (parts.size() == 0) {
            newData = false;
            return;
        }

        std::string command = parts[0];
        if (parts.size() == 1 && (command == "ready" || command == "status")) {
            ready = true;
            printStatus();
            if (command == "status" )
                Serial.printf("status %d %d %d %d %d %d %d %d\n", status[0], status[1], status[2], status[3], status[4], status[5], status[6], status[7]);
        } else if (parts.size() == 1 && command == "unready") {
            ready = false;
        } else if (parts.size() == 2 && (command == "on" || command == "off")) {
            ready = true;
            
            int target = command == "off" ? OFF : ON;

            if ("all" == parts[1]) {
                for(int i = 0 ; i < 8 ; i++) {
                    if (status[i] != target) {
                        digitalWrite(pins[i], ON);
                        offstack[i][0] = millis() + (target == OFF ? POWER_TIME : TOGGLE_TIME);
                        offstack[i][1] = target;
                    }
                }
            } else {
                int relay = std::stoi(parts[1]);
                int i = relay - 1;
                if (i >= 0 && i < 8 ) {
                    if (status[i] != target) {
                        digitalWrite(pins[i], ON);
                        offstack[i][0] = millis() + (target == OFF ? POWER_TIME : TOGGLE_TIME);
                        offstack[i][1] = target;
                    }
                }
            }
        }

        newData = false;
    }
}

void loop() {
    if (!ready) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= messageInterval) {
            previousMillis = currentMillis;
            messageInterval = rand() % range + messageIntervalMin;
            updateBoot(messages[rand() % messages.size()]);
        }
    }

    button.loop();
    recvWithEndMarker();
    processData();

    bool changes = false;
    unsigned long now = millis();
    for (int i = 0 ; i < 8 ; i++) {
        if (offstack[i][0] != 0 && now > offstack[i][0]) {
            digitalWrite(pins[i], OFF);
            offstack[i][0] = 0;
            status[i] = offstack[i][1];
            changes = true;
        }
    }

    if (changes) {
        printStatus();
        //Serial.printf("status %d %d %d %d %d %d %d %d\n", status[0], status[1], status[2], status[3], status[4], status[5], status[6], status[7]);
    }
}