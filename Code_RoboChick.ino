#include <ESP8266WiFi.h>          // ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <ESP8266WiFiMulti.h>     // Include the Wi-Fi-Multi library
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiClient.h>
#include "pages.h"
#include <Wire.h>
#include <list>

#define SENSOR_ADDRESS 0x40 //I2C address of the proximity sensor (GP2Y0E03)
#define DISTANCE_ADDRESS 0x5E // data address of distance value


//PleasureChirps are on trigger 0
//DistressCalls are on trigger 1
enum eventType {peck, sound, vibrate, calibrate, approach, expStart, expEnd};
struct event{ // This class defines the type of events that are logged
  eventType category;
  String timestamp;
  String message;
};

struct experiment{          // This structure describes all the elements of the experiment (Peck detection)

  //initialize variables
  bool alive;               // Is the experiment running? 0=no 1=yes
  unsigned long startTime;  // When did the experiment start (timestamp in milliseconds since power was switched on)
  unsigned long proximityCheck; // time of the last proximity sensor check (timestamp in millis)
  unsigned long soundStart; //time when sound started (millis)
  int distance;             // Distance read from the proximity sensor (cm)
  bool activeCheck=0;
  bool sound= LOW; //LOW indicates sound is off
  unsigned long flashStart;     // timestamp when the previous flash started 
  unsigned long previousFlash; // timer for flashing lights
  unsigned long lastPeck;    // time when the last peck was detected (timestamp)
  byte dist[2];            // byte array for reading distance from the proximity detector

  //experimental settings (set via the online gui)
  unsigned long duration;   // How long is the experiment meant to last (in milliseconds)
  int sensitivity;          // Sensitivity of the tactile sensor (set via the online gui)
  bool Sound1;              // Option: Play sound 1 on detection of a peck 
  bool Sound3;              // Option: Play sound 2 (indexed as 3) on detection of a peck
  bool light;               // Option: Light in response to pecking
  bool flashing;            // Option: constant flashing light
  bool approachSound;       // Option: flash light when chick detected at a specified distance (set via the online gui) 
 
};

// Functions that will be defined later
void buzzDroid(int timeMS);
void handleVibrate();
void handleStop();
void startExperiment();
int readSensor();
int proximity();     //read proximity sensor             
void playSound1();   //distress
void playSound3();   //pleasure
String millisToString(unsigned long ms);
void handlePeck();
int calibrateSensor();
void flashLight();
void chickApproaching(); // proximity sensor detecting the chick
void check(); //turns off actions which are finished
event newEvent(eventType cat, String msg);
void reconnectWifi(); //reconnects to wifi if connection was lost

experiment expConfig;
int stableMeasurement;
std::list<event> events;
String minsIn;
String hrsIn;
bool pecked;
bool vibrating;

int motor = 12;
int Sound3 = 16;
int Sound1 = 13;
int stateFlash = LOW; 



//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(8000); //Server on port 8000
ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

//===============================================================
// This routine is executed when you open the IP:port\routine in browser- in order to start the experiment, IP:port needs to be open in browser
//===============================================================
void mainPage() // What to do when main page called?
{
  String s = MAIN_pageS; //Read HTML contents of header
  if (expConfig.alive) // Print status of experiment
  {
    s = s + "Experiment is running<br>";
  }
  else
  {
    s = s+ "Experiment is not running<br><br>";
  }
  s = s + MAIN_pageE; // Add end of HTML page
  server.send(200, "text/html", s); //Send web page
}

void handleRoot()
{
  Serial.println(F("You called root page"));
  mainPage();
}

void handleViewConfig() // This happen when we choose to view current experiment status
{
  Serial.println(F("You asked to see current experiment"));
  if (expConfig.alive)
  {
    String page;
    page = CONFIG_pageS;
    page = page + "Duration - " + hrsIn + ":" + minsIn + "<br>";
    page = page + "Threshold - " + expConfig.sensitivity + "<br>";
    if (expConfig.vibration)
    {
      page = page + "Vibration pulse of length " + expConfig.pulseLength + "ms" "<br>";
    }
    if (expConfig.Sound1)
    {
      page = page + "Sound 1<br>";
    }
    else if (expConfig.Sound3)
    {
      page = page + "Sound 2<br>";
    }
    page = page + RESULTS_pageE;
    server.send(200, "text/html", page);
  }
  else
  {
    server.send(412, "text/html", "Experiment not running - go back and start an experiment");
  }
}


void refreshResults()
{
  String s = RESULTS_pageS; //Read HTML contents
  String results;
  for (event e:events) // It loads all events and prints them
  {
    results = results + String(e.timestamp);
    results = results + String(e.message);
    results = results + "<br>";
  }
  s = s + results + RESULTS_pageE;
  server.send(200, "text/html", s); //Send web page
}

void handleResults() // This happens when results are requested
{
  Serial.println(F("Results page"));
  String s = RESULTS_pageS; //Read HTML contents
  Serial.println(F("results head loaded"));
  String results;
  for (event e:events) // It loads all events and prints them
  {
    results = results + String(e.timestamp);
    results = results + String(e.message);
    results = results + "<br>";
  }
  Serial.println(F("results loaded"));
  s = s + results + RESULTS_pageE;
  Serial.println(F("page build complete"));
  server.send(200, "text/html", s); //Send web page
  Serial.println(F("page sent"));
}

void handleSound3()
{ // When this Sound is requested
  Serial.println(F("Sound 3 (4?) manually played"));
  mainPage();
  //playSound4();
}

void handleSound2()
{
  Serial.println(F("Sound 2 manually played"));
  mainPage();
  playSound3();
}

void handleSound1()
{
  Serial.println(F("Sound 1 manually played"));
  mainPage();
  playSound1();
}

void handleVibrate()
{
  Serial.println(F("Vibration activated manually"));
  mainPage();
  buzzDroid(expConfig.pulseLength);
}

void handleStop() // When stop request of experiment
{
  Serial.println(F("experiment ended"));
  expConfig.alive = false;
  if(expConfig.sensitivity == 0)
  {
    analogWrite(motor, 0);
    vibrating = false;
  }
  mainPage();
}

void handleCalibrate() // Manual calibration request
{
  Serial.println(F("Recalibration requested"));
  stableMeasurement = calibrateSensor();
  mainPage();
  Serial.print(F("Sensor recalibrated to "));
  Serial.println(stableMeasurement);
}

void startExperiment() // Start experiment request
{
  String s = EXPERIMENT_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

void handleForm() // This function receives experiment form and configures experiment
{
  if(server.arg("lengthHR") == NULL){
    minsIn = server.arg("lengthMIN");
    expConfig.duration = (minsIn.toInt())*60000;
  }
  else if(server.arg("lengthMIN") == NULL){
    hrsIn = server.arg("lengthHR");
    expConfig.duration = (hrsIn.toInt())*3600000;
  }
  else {
    minsIn = server.arg("lengthMIN");
    hrsIn = server.arg("lengthHR");
    unsigned long mins = minsIn.toInt();
    unsigned long hrs = hrsIn.toInt();
    expConfig.duration = hrs*3600000 + mins*60000;
  }
  
  expConfig.sensitivity = server.arg("sensitivity").toInt();
  expConfig.pulseLength = server.arg("vibrateLength").toInt();

  expConfig.pauseLength = 0;
  
  // enable/disable 
  if (server.arg("responseVibrate")=="V"){
    expConfig.vibration = true;
    expConfig.intensity = 100 + (server.arg("vibrateIntensity").toInt() * 1.5);
    if(expConfig.sensitivity == 0){
      expConfig.pauseLength = server.arg("pauseLen").toInt();
    }
  }
  else{
    expConfig.vibration = false;
  }
  if (server.arg("responseS1")=="S1"){
    expConfig.Sound1 = true;
    expConfig.Sound3 = false;
  }
  else{
    expConfig.Sound1 = false;
  }
  if (server.arg("responseS2")=="S2"){
    expConfig.Sound3 = true;
    expConfig.Sound1 = false;
  }
  else{
    expConfig.Sound3 = false;
  }
  if (server.arg("light")=="L"){
    expConfig.light = true;
  }
  else{
    expConfig.light = false;
  }

  if (server.arg("flashing")=="F"){
    expConfig.flashing = true;
    expConfig.light = false;
    expConfig.flashStart=0;
  }
  else{
    expConfig.flashing = false;
  }
  
  if (server.arg("approachSound")=="A1"){
    expConfig.approachSound= true;
  }
  else {
    expConfig.approachSound= false;
  }
 

  expConfig.alive = true;               //enable experiment behaviour
  pecked = false;                       //Not being pecked at experiment start
  
  expConfig.startTime = millis();                 //log experiment start time
  
  events.push_back(newEvent(expStart, " - experiment start"));
  Serial.println(F("experiment start logged"));

  String chick= server.arg("chickID");
  String date= server.arg("date");
  String arena= server.arg("arena"); 
  
  Serial.println(date + "_arena_" + arena + "_chick_" + chick + '.');

  String s = SUCCESS_page;
  server.send(200, "text/html", s); //Send web page
  
}

//===============================================================
// This routine is executed when you turn on System
//===============================================================
void setup() {
  pinMode(0, OUTPUT); 
  pinMode(motor,OUTPUT);
  pinMode(Sound1, OUTPUT);
  pinMode(Sound3,OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(Sound3,LOW);
  digitalWrite(Sound1,LOW);
  digitalWrite(motor,LOW);
  digitalWrite(0,HIGH);
 

  expConfig.alive = false;

  
  //initialize I2C
  Wire.begin();
  delay(500); //start after 500ms
  
  Serial.begin(115200);
  delay(500);
  Serial.println("Serial connections online");  // Confirm serial online
 
  // We start by connecting to a WiFi network- FILL IN WITH THE DETAILS NEEDED FOR CONNECTION

 wifiMulti.addAP("NETWORK_NAME1", "PASSWORD1");
 wifiMulti.addAP("NETWORK_NAME2", "PASSWORD2");
 
 Serial.print("Connecting to wifi");

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
 
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  
  Serial.println('\n');
  Serial.print(F("Connected to "));
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print(F("IP address: "));
  Serial.print(WiFi.localIP());     // Send the IP address of the ESP8266 to the computer
  
  server.on("/", handleRoot);       //Which routine to handle at root location. This is display page
  server.on("/experiment", startExperiment);
  server.on("/viewConfig", handleViewConfig);
  server.on("/sound1", handleSound1);
  server.on("/sound2", handleSound2);
  server.on("/sound3", handleSound3);
  server.on("/vibrate", handleVibrate);
  server.on("/expStop", handleStop);
  server.on("/calibrate",handleCalibrate);
  server.on("/results", handleResults);
  server.on("/config", HTTP_POST, handleForm);

  server.begin();                  //Start server
  Serial.println(F(" HTTP server started"));

  stableMeasurement = calibrateSensor();
  Serial.print(F("stable acceleration calibrated to "));
  Serial.println(stableMeasurement);

  vibrating = false;

  int counter=0;

}

//===============================================================
// This routine is executed constantly. Contains experiment (if enabled)
//===============================================================
void loop()
{
  ESP.wdtDisable();
  ESP.wdtFeed();

  int inst;
  int d;
  byte dist[2];
  
  //Serial.println("loop");
  server.handleClient();
  if ((expConfig.alive) && (millis()%10==0)) // If experiment active
  {
    d= proximity(); 
    inst = readSensor();
    int peckInterval=1000;
    if ((expConfig.sensitivity != 0) && (!pecked) && (abs(inst - stableMeasurement) > expConfig.sensitivity) && (millis()-expConfig.lastPeck >= peckInterval)) // Experiment when sensitivity is not 0
    {
      Serial.print(F("new peck detected: "));
      Serial.print(inst);
      Serial.print(F(" at "));
      Serial.println(millisToString(millis()));
      expConfig.lastPeck= millis();
      handlePeck();
    }
    else
    {
      pecked = false;
    }

    if (d==0) {
      d= Wire.requestFrom(SENSOR_ADDRESS, 2);
      dist[0]=Wire.read(); //
      dist[1]= Wire.read();
      expConfig.distance= ((dist[0] * 16 + dist[1]) / 16) / 4; // distance
      if (expConfig.distance < 30) {
        chickApproaching();
      }
      
    }
    
    if(expConfig.sensitivity == 0) // If sensitivity is 0 there is some sort of alternating behavior
    {
      unsigned long cycleLength = expConfig.pulseLength + expConfig.pauseLength;
      if((((millis() - expConfig.startTime)%cycleLength) < expConfig.pulseLength) && (!vibrating))
      {
        analogWrite(motor, expConfig.intensity);
        vibrating = true;
      }
      if((((millis() - expConfig.startTime)%cycleLength) > expConfig.pulseLength) && (vibrating))
      {
        analogWrite(motor, 0);
        vibrating = false;
      }
    }
    
    if ((millis() - expConfig.startTime) > expConfig.duration) // If timelimit reached, terminate the experiment
    {
      analogWrite(motor, 0);
      vibrating = false;
      expConfig.alive = false;
      events.push_back(newEvent(expEnd, " - experiment ended"));
      Serial.println("experiment ended");
    }
      if ((expConfig.flashing) && (expConfig.alive))
    {
      flashLight();
    }
  }

  if (expConfig.activeCheck==1){
    check();
  }
  yield();

}



void buzzDroid(int timeMS) // Handles motor vibration
{
  analogWrite(motor, expConfig.intensity); // Turns on vibration and also output is intensity
  delay(timeMS);
  analogWrite(motor, 0);
  if (expConfig.alive && expConfig.vibration){ // Push event
    events.push_back(newEvent(vibrate, " - vibration pulse of length " + String(timeMS) + "ms"));
  }
}




void playSound3(){
  if (expConfig.sound==LOW){
    expConfig.soundStart=millis();
    expConfig.sound=HIGH;
    digitalWrite(Sound3, HIGH);
    events.push_back(newEvent(sound, " - played pleasure sound"));
    expConfig.activeCheck= 1;
  }
}

void check(){
  if ((millis() - expConfig.soundStart) > 500UL){
    digitalWrite(Sound3, LOW);
    digitalWrite(Sound1, LOW);
    expConfig.sound=LOW;
    expConfig.activeCheck= 0;
  }
  
}

void playSound1(){
  if (expConfig.sound==LOW){
    expConfig.soundStart=millis();
    digitalWrite(Sound1, HIGH);
    expConfig.activeCheck= 1;
    expConfig.sound=HIGH;
  }
}

int readSensor(){ // Read value of sensor
  return analogRead(A0);
}

int proximity(){ //read proximity sensor
    Wire.beginTransmission (SENSOR_ADDRESS); //start communication
    Wire.write (DISTANCE_ADDRESS); // specify the addtess of the table storing the distance
    expConfig.distance= Wire.endTransmission();//close data
  }


String millisToString(unsigned long ms){ // Transform milisecond into something readable
  int hours = ((ms/1000) / (60 * 60)) % 24;
  int minutes = ((ms/1000) / 60) % 60;
  int seconds = (ms/1000)%60;
  return String(hours) + ":" + String(minutes) + ":" + String (seconds);
}



void flashLight(){ //light continuously flashing throughout the duration of the experiment. Change frequency by changing "interval" in expConfig
  int interval= 500;
  if (millis()-expConfig.flashStart >=interval) {
    expConfig.flashStart= millis();

    if (stateFlash == LOW) {
      stateFlash = HIGH;
    } else {
      stateFlash = LOW;
    }
    digitalWrite(0, stateFlash);
  }
}



void chickApproaching(){
  unsigned long proxInterval= 1500;
    if (millis()-expConfig.proximityCheck >= proxInterval) {
     expConfig.proximityCheck= millis();
      //events.push_back(newEvent(approach, " - chick approaching at " + (String(expConfig.distance)) + "cm"));
      //refreshResults();
      if ((expConfig.approachSound==true) && (expConfig.distance<=15)){
        playSound1();
              }
    }
}



void handlePeck(){ // What happens after peck detection
  events.push_back(newEvent(peck, " - peck detected"));

  if(expConfig.light){ // Would turn on light if needed
    digitalWrite(0, LOW);
  }
  if(expConfig.Sound1){
    playSound1();
  }
  else if(expConfig.Sound3){
    playSound3();
  }
   if(expConfig.vibration){
    buzzDroid(expConfig.pulseLength);
   }
  delay(100); // Wait a bit
  digitalWrite(0, HIGH); //Turn off light
  refreshResults();
}



int calibrateSensor(){
  int acc = 0;
  int inst;
  for(int i=0;i<100;i++){
    inst = readSensor();
    acc = acc + inst;
  }
   if (expConfig.alive){
    events.push_back(newEvent(calibrate, (" - recalibrated - " + (String(acc/100))))); // If recalibrated in the middle of experiment
  }
  return acc/100;
}

event newEvent(eventType cat, String msg){
  event newEvt;
  newEvt.category = cat;
  newEvt.timestamp = millisToString(millis() - expConfig.startTime);
  newEvt.message = msg;
  return newEvt;
}



void reconnectWifi() {
    Serial.print("Reconnecting to wifi");

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  Serial.print(F("Reconnecting to "));
  
  Serial.println('\n');
  Serial.print(F("Reconnected to "));
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print(F("IP address: "));
  Serial.print(WiFi.localIP());     // Send the IP address of the ESP8266 to the computer
  
}
