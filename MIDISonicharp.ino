/*
  Sonic Distance detection by HC-SR04 Module and Midi control sender over UDP RTP_MIDI
  Maximun refresh rate and maximal distance for both ways,
  is about for 3.4 Meter - 10ms duration 100Hz
  The minimum Pulselenght and therfor min Distance detection is
  3.4mm by about 10 us.
  wavespeed depence on AirTempeatur.bit this would to much.

Licence is under MIT Permission so free to republish
Thanks for download this cody 

///*  by jokiestyler  *\\\
*/
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN     2

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  16

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 20

#define echoPin 14 // ESP32 Wrover 16 // 14 ESP8266 D6 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 12 // ESP32 Wrover 17 // 12 ESP8266 D5 // attach pin D3 Arduino to pin Trig of HC-SR04
#define PAUSESECONDS 10
#define MEMORIES 16     // variables vor timecode and distance value
#define INTERMEASURE 10 // PulseTime for 40 kHz Pulse Transmission
#define MESUREX 5     // Floating mean distance calculation X Measurement
#define TaktPosLen 16   //  Steps of Beats also Micro Steps div by 4

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define SerialMon Serial
#include <AppleMIDI_Debug.h>
#include <WiFiClient.h>
#endif
#include <AppleMIDI.h>
#include <Adafruit_NeoPixel.h>

char ssid[] = "MidiWlan"; // this is the default WIFI network name down in te code you can put 3 more network names
char pass[] = "MidiWlanServices"; // this is the WIFI network password
unsigned long t0 = millis();
int8_t isConnected = 0;

APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();

byte target_ip[4] = {  192, 168, 0, 6 }; // this is the IP address of the target computer e.g. 192.168.1.1
int target_port = 7400; // this is the UDP port
byte midi_message[3];
byte note = 43;
byte velocity = 55;
byte channel = 1;

int current;
int previous;

int ledState = LOW;
int TaktPos = 1;      //Startpoint of midi Loop 1-16 Beats
int mTaktPos = 1;   //Startpoint of midi Loop divider 1- 16
int Bpm = 128; //Beats per minute

int ColorR[TaktPosLen + 1]; // speicher für Ledstribe
int ColorR2[TaktPosLen + 1]; // speicher für Ledstribe
int ColorG[TaktPosLen + 1];
int ColorB[TaktPosLen + 1];

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long detecStartTime = millis();
const long detecInterval = 40;
const long BpmTime = ((60000 / 16) / Bpm) ; // in ms
int pulseLength = 5; // in us Sonar Beametime
int i = 0;
int j = 0;
long sonarduration;
int expectdist = 60;
int lowdist = 20;
int highdist = 30;
int distance;
int xNote = 0; // Pointer for Arpregio Note counter

int sonardist[5];
int TriggerLevel = 30; // Triggerpoint for note on
int NoteLevTmp; // for note on Level marker
int NoteonTmpOld; // for note on marker Debounce
int NoteLev[16]; // Output stae
int NotePitc[16]; // Note pitc
int NoteonTime[16]; // length of note play - dec in 16 steps per beat
int Arpregio[16]= {43,46,50,54,57,60,59,55,43,47,50,55,57,67,55,46} ; // dynamic arpseting - define next drigger by delay in mtaktCounter 8 by 16 mtakt
int SSIDvar = 2;

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);   //LEDSttripSetup

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
  colorWipe(strip.Color(  55,   0, 0)); // Init Mix color
  delay(20);
  colorWipe(strip.Color(  0,   55, 0)); // Init Mix color
  delay(20);
  colorWipe(strip.Color(  0,   0, 55)); // Init Mix color
  delay(20);



  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  DBG_SETUP(115200);
  DBG("Booting");
  DBG(F("SonicHarp")); // print some text in Serial Monitor


  pinMode(4, INPUT_PULLUP); // a button is connected between this pin and ground
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {

    for (int i = 0; i < LED_COUNT; i++) { //Wait for Wlan connection
      currentMillis = millis();
      if (currentMillis - previousMillis >= (BpmTime * 20)) { // is time ready to go to next Wlan
        previousMillis = currentMillis;
        // Fill along the length of the strip in various colors...
        colorSet(strip.Color(255 - ( i), TaktPos ,   (TaktPos - i) + 22), (i)); // Red
        TaktPos++;
        if (TaktPos == (TaktPosLen + 1)) {

          TaktPos = 1;  // Start at Pos1
          switch (SSIDvar) {
            case 1: {
                //do something when SSIDvar equals 1
                WiFi.disconnect();
                char ssid[] = "SSID1"; // this is the WIFI network name
                char pass[] = "PASS1"; // this is the WIFI network password
                WiFi.begin(ssid, pass);
                 delay(200);//  give The Connection some timr to be Established
               
                SSIDvar = 2 ; // next WLAN activated
                break;
              }
            case 2: {
                //do something when SSIDvar equals 2
                WiFi.disconnect();
                char ssid[] = "SSID2";
                char pass[] = "Pass2";
                WiFi.begin(ssid, pass);
                 delay(200);
                SSIDvar =   3;
                break;
              }
            case 3: {
                WiFi.disconnect();
                char ssid[] = "MidiWlan"; // this is the WIFI network name
                char pass[] = "MidiWlanServices"; // this is the WIFI network password
                WiFi.begin(ssid, pass);
                delay(200);
                SSIDvar =   1;
                break;
            
              }
          }

        }
      }
      ColorG[TaktPos] = 0; // Blue Takt Pixel

    }//Ende for LedSetup

    DBG("Establishing connection to WiFi..");
    DBG(ssid);
    DBG(SSIDvar);
    digitalWrite(LED_BUILTIN, current);
    if (current) { // test for change in button state
      current = 0;
    } else {
      current = 1;
    }
  }
  DBG("Connected to network");

  DBG(F("OK, now make sure you an rtpMIDI session that is Enabled"));
  DBG(F("Add device named Arduino with Host"), WiFi.localIP(), "Port", AppleMIDI.getPort(), "(Name", AppleMIDI.getName(), ")");
  DBG(F("Select and then press the Connect button"));
  DBG(F("Then open a MIDI listener and monitor incoming notes"));
  DBG(F("Listen to incoming MIDI commands"));
  MIDI.begin();

  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    isConnected++;
    DBG(F("Connected to session"), ssrc, name);
  });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    isConnected--;
    DBG(F("Disconnected"), ssrc);
  });

  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    DBG(F("NoteOn"), note);
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    DBG(F("NoteOff"), note);
  });

  DBG(F("Sending NoteOn at trigger level"), TriggerLevel);

  detecStartTime = currentMillis;
  for (int j = 0; j < MESUREX; j++) { // 5 Mesurements for Mean Caculation

    sonarPulse(pulseLength);// start Measurement - write in global duration
    sonardist[j] = sonarduration * 0.017;
    delay(20);

  }// for END j
  //cut until

  expectdist = (sonardist[0] + sonardist[1] + sonardist[2] + sonardist[3] + sonardist[4]) / 5;

  lowdist = 0.8 * expectdist;
  highdist = 1.25 * expectdist;
  NoteLevTmp = 0;
}// Setup ende



void loop() {
  // Listen to incoming notes
  MIDI.read();

  // Set timestamp for Bpm Looper 8 Times div 16 Microsteps
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= BpmTime) {
    previousMillis = currentMillis;
    if (mTaktPos >= 5) {
      mTaktPos = 1;  // Note that this switches the LED *off*

      // Led output clear Blue TaktPos
      if (NoteLevTmp >= 1) {
        ColorR[TaktPos] = 0;
        ColorG[TaktPos] = 0; // Blue Takt Pixel
      }
      ColorB[TaktPos] = 0;
      colorSet(strip.Color(ColorR[TaktPos], ColorG[TaktPos], ColorB[TaktPos]), (TaktPos - 1)); // Clear Last pixel
      TaktPos++;
      if (TaktPos == (TaktPosLen + 1)) {
        TaktPos = 1;  // Reset Takt Position to 1
      }

      ColorB[TaktPos] = 50; // Blue Takt Pixel
      ColorG[TaktPos] = 0; // Blue Takt Pixel

      colorSet(strip.Color(ColorR[TaktPos], ColorG[TaktPos], ColorB[TaktPos]), (TaktPos - 1));
      if (NoteonTime[xNote] >= 1 ) {
        velocity = NoteLevTmp;
        
        NoteLev[xNote+1] = NoteLevTmp ;
        NotePitc[xNote]=Arpregio[xNote];
        MIDI.sendNoteOn( NotePitc[xNote], velocity, channel);

       NoteonTime[xNote+1]=NoteonTime[xNote]-1;
         NoteonTime[xNote]=0;
        xNote++;
      
      
        
      } else if (NoteonTime[xNote] == 0) {
        for(int i=0; i < xNote; ++i ){
        NoteLev[xNote-i] = -1;
        velocity = 0;
        MIDI.sendNoteOff(NotePitc[xNote-i], velocity , channel);
        }
        xNote=0;
        // send note off
        // MIDI.sendNoteOff(note, velocity, channel);
      }

    } else { 
      mTaktPos++;
    }
    //mTaktpos End
  }//BpmLoopIntervall End
  //StartSonicScan

  //Wait for next detection
  currentMillis = millis();
  if (currentMillis - detecStartTime >= detecInterval) {
    detecStartTime = currentMillis;
    sonarPulse(pulseLength);// start Measurement variable Pulslength - write in global duration
    sonardist[j] = (sonarduration ) * 0.034 / 2;

    expectdist = (sonardist[0] + sonardist[1] + sonardist[2] + sonardist[3] + sonardist[4]) / 5;

    lowdist = 0.8 * expectdist;
    highdist = 1.25 * expectdist;

    // Tiggerauswertung
    if ((sonardist[j] >= 5) && (sonardist[j] < 132)) {
      //xNote++; // note handler for Polyphonic may no note set to -1
      NoteLevTmp = (sonardist[j] - 5) + 1; //Level note on shift to midi scale 7 bit

      if (NoteLevTmp != NoteonTmpOld) {
        NoteonTmpOld = NoteLevTmp;
        MIDI.sendControlChange(45, NoteLevTmp, 1);
        ColorG[NoteLevTmp >> 3] = 15;
        colorSet(strip.Color(ColorR[NoteLevTmp >> 3], ColorG[NoteLevTmp >> 3], ColorB[NoteLevTmp >> 3]), (NoteLevTmp >> 3)); // Clear Last pixel

      }
      if ((NoteLevTmp <= TriggerLevel)) {
        NoteonTime[0] = ((TriggerLevel - NoteLevTmp) );
        DBG(F("    TRIGGERING   "));
        DBG(F("distance="), NoteLevTmp);
        DBG(F(" xNote"),xNote );
        DBG(F("   Mean="), (expectdist));
        DBG(F("   NoteonTime[xNote]"), NoteonTime[0]);

        // Ausgabe vor der neuberechnung?
        for (int i = 0; i < NoteonTime[0]; i++) {
          // Led stripe control
          if (( i + TaktPos) < LED_COUNT) {
            ColorR[TaktPos + i] = 50 - (i * 2);
            colorSet(strip.Color(ColorR[TaktPos + i], ColorG[TaktPos + i], ColorB[TaktPos + i]), (TaktPos + i - 1)); // Clear Last pixel
          } else if ((i + TaktPos - LED_COUNT) <= LED_COUNT) {
            ColorR[TaktPos + i - LED_COUNT] = 50 - ((i - LED_COUNT) * 2);
            colorSet(strip.Color(ColorR[TaktPos + i - LED_COUNT], ColorG[TaktPos + i - LED_COUNT], ColorB[TaktPos + i - LED_COUNT]), (TaktPos + i - 1 - LED_COUNT)); // Clear Last pixel
          }
        }

      }//end trigger routine

      //LED Toggle
      if (current) { // test for Maschine Sequenzer run state
        current = 0;
      } else {
        current = 1;
      }
      if (current != previous) { // test for change in button state
        previous = current;
        digitalWrite(LED_BUILTIN, current); // turn LED on or off depending on button state
      }
      // OverFlow check for dataPointer Mean sonic distance
      if (j < 4) {
        j++; //next Datapoint
      } else {
        j = 0;
      }

    }
  }// End Sonic Mesurement


  ESP.wdtFeed();

}// END LOOP

//Sonar read Function Result in cm duration
void sonarPulse(int PulseDurMic) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(PulseDurMic);
  digitalWrite(trigPin, LOW);
  sonarduration = pulseIn(echoPin, HIGH);//global variable for speed up
}
// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorSet(uint32_t color, int Pixel) {
  strip.setPixelColor(Pixel, color);         //  Set pixel's color (in RAM)
  strip.show();                          //  Update strip to match
}
// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color) {
  for (int iPixel = 0; iPixel < strip.numPixels(); iPixel++) { // For each pixel in strip...
    strip.setPixelColor(iPixel, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(5);                           //  Pause for a moment
  }
}
