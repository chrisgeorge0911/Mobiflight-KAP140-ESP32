
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Fonts/FreeSans6pt7b.h"
#include "Fonts/DSEG7Classic_Italic14pt7b.h"
#include "Fonts/DSEG14Classic_Italic14pt7b.h"
#include "Fonts/DSEG14Classic_Regular14pt7b.h"

#define I2C_MOBIFLIGHT_ADDR 0x27
#define I2C_MOBIFLIGHT_SDA 21
#define I2C_MOBIFLIGHT_SCL 22

#define I2C_DISPLAY_SDA 17
#define I2C_DISPLAY_SCL 16

TwoWire I2Ctwo = TwoWire(1);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// address of the multiplexer to change the channels
#define TCA9548A_I2C_ADDRESS  0x70
#define KAP140_DISPLAY_LEFT  0
#define KAP140_DISPLAY_CENTRE 1
#define KAP140_DISPLAY_RIGHT 2

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Ctwo, OLED_RESET);

String rightBlockValue = "-";
int RightBlockMode = 0;
int BaroMode = 0;
bool BaroDisplay = 0;

// Display flags
bool AltArm = false;
bool NavArm = false;
bool AprArm = false;
bool RevArm = false;
bool ROL_HDG = false;
bool VertHold = false;
bool AltHold = false;
bool ROL_NAV = false;
bool ROL_WingLeveler = false;
bool ROL_REV = false;
bool ROL_APR = false;

// State flags
bool ap_powered_on = false;
bool ap_preflight_PFT=false;
bool ap_preflight_showall=false;
bool ap_preflight_complete=false;
bool ap_baro_initialised=false;
bool ap_engaged = false;
bool ap_disengaging = false;
bool ap_display_state = false;
bool ap_alert_solid = false;
bool ap_alert_flash = false;
bool rightblock_display_state = false;
bool alert_display_state = false;
bool changedArmState = false;
bool NavArmState = false;
bool AprArmState = false;
bool RevArmState = false;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long flashperiod = 370;
int ap_flash_max = 7;
int flashcount = 0;
int PFT_step = 0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  Wire.onReceive(onReceive);

  Wire.begin((uint8_t)I2C_MOBIFLIGHT_ADDR,I2C_MOBIFLIGHT_SDA,I2C_MOBIFLIGHT_SCL,400000);
  I2Ctwo.begin(I2C_DISPLAY_SDA,I2C_DISPLAY_SCL,400000); // SDA pin 16, SCL pin 17, 400kHz frequency

  setTCAChannel(KAP140_DISPLAY_LEFT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  updateDisplayLeft();

  setTCAChannel(KAP140_DISPLAY_RIGHT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  updateDisplayRight();

  setTCAChannel(KAP140_DISPLAY_CENTRE);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  updateDisplayCentre();

}

void loop() {  
  if (!ap_powered_on) {
    power_off_display();
  }
  else if (ap_powered_on && !ap_preflight_complete) {     // Autopilot preflight sequence
    startupSequence();
  } 
  else {                            // Resume normal operation    
    updateDisplayRight();
    updateDisplayCentre();
    updateDisplayLeft();
  }
  delay(50);
}

void onReceive(int len){
  //Serial.println(Wire.available());
  char msgArray[9]="";
  // // if smaller than 32 ignore
  if(Wire.available()>=32)
  {
    for (int i=0; i <= 8; i++){
      uint8_t hibits = (uint8_t)Wire.read();      
      Wire.read(); // ignore
      uint8_t lowbits = (uint8_t)Wire.read(); 
      Wire.read(); // ignore

      msgArray[i] = msgArray[i] | (hibits & 0xf0);
      msgArray[i] = msgArray[i] | ((lowbits & 0xf0)>>4);
      if(msgArray[i] == 0x20){
        msgArray[i]='\0';
      }
    }
    if(msgArray[7] != '\0'){
        msgArray[8]='\0';
    }

    handleCommand(String(msgArray));

  }else{
    while(Wire.available()){
      Wire.read();
    }
  }
}

void handleCommand(String command){
  //Serial.println(command);
  if(command.startsWith("#0")){    
      if(command.substring(2)=="0"){           // ALT
      RightBlockMode=0;        
      BaroDisplay=false;
    } else if (command.substring(2)=="1") {   // VS
      RightBlockMode=1;        
      BaroDisplay=false;
    } else if (command.substring(2)=="2") {   // Baro
      RightBlockMode=2;
      BaroDisplay=true;      
    }
  }
  else if(command.startsWith("#1")){  // how long the value should be
    if (!ap_preflight_showall) {
      if (command=="#1310100") {
        PFT_step=1;
      } 
      else if(command=="#1310200") {
        PFT_step=2;
      }
    }
    int rightBlockLength = command.substring(2,3).toInt();
    rightBlockValue=command.substring(8-rightBlockLength);    
  }
  else if(command.startsWith("#2")){
    if(command.substring(2)=="0"){
      BaroMode = 0;
    }else{
      BaroMode = 1; 
      if (RightBlockMode == 2)
      {
        if (rightBlockValue.length() < 5)
        {
          rightBlockValue += " ";
        }
      }     
    }
  }
  else if(command.startsWith("#3")){
    // if(command.substring(2)=="0"){
    //   BaroDisplay=false;
    // }else{
    //   BaroDisplay=true;
    //   if (BaroMode == 1) {
    //     rightBlockValue += " ";  // if HG, pad the value with one space to display XX,XX
    //   }
    // }
  }
  else if(command.startsWith("#4")){
    ROL_HDG=false;
    ROL_NAV=false;
    ROL_WingLeveler=false;
    ROL_REV=false;
    ROL_APR=false;

    if(command.charAt(2)=='1'){
      ROL_HDG=true;
    }

    if(command.charAt(3)=='1'){        
      ROL_NAV=true;
    }

    if(command.charAt(4)=='1'){
      ROL_WingLeveler=true;
    }

    if(command.charAt(5)=='1'){
      ROL_REV=true;
    }

    if(command.charAt(6)=='1'){
      ROL_APR=true;
    }
  }
  else if(command.startsWith("#5")){
    VertHold=false;
    AltHold=false;

    if(command.charAt(2)=='1'){
      VertHold=true;
    }
    if(command.charAt(3)=='1'){
      AltHold=true;
    }
  }
  else if(command.startsWith("#6")){

   // Serial.println(command.substring(2));
    ap_preflight_PFT=false;
    ap_preflight_showall=false;
    ap_preflight_complete=false;
    if(command.charAt(2)=='0'){  // KAP State (on/off)
      ap_powered_on=false;
      reset_AP();
    }
    if(command.charAt(2)=='2'){  // KAP State (on/off)
      ap_powered_on=true;
      ap_preflight_PFT=true;
    }
    if(command.charAt(3)=='1'){  // MS autopilot show all
      ap_preflight_showall=true;
    }
    if(command.charAt(4)=='1'){  // MS autopilot preflight complete
      ap_preflight_complete=true;
    }
  }
  else if(command.startsWith("#7")){
    AltArm=false;
    NavArm=false;
    AprArm=false;
    RevArm=false;

    if(command.charAt(2)=='1'){
      AltArm=true;
    }
    if(command.charAt(3)=='1'){
      NavArm=true;
    }
    if(command.charAt(4)=='1'){
      AprArm=true;
    }
    if(command.charAt(5)=='1'){
      RevArm=true;
    }
  }
  else if(command.startsWith("#8")){
     if(command.charAt(2)=='0'){  // KAP State (on/off)
      ap_baro_initialised=false;
     }
     else if (command.charAt(2)=='1'){
      ap_baro_initialised=true;
     }

     if(command.charAt(3)=='0'){
      ap_alert_flash = false;
      ap_alert_solid = false;
     } else if (command.charAt(3)=='1'){
      ap_alert_solid = true;
      ap_alert_flash = false;     
     } else if (command.charAt(3)=='2'){
      ap_alert_solid = false;
      ap_alert_flash = true;
     }
  }
}

/*
  switch multiplexer channel
*/
void setTCAChannel(byte i){
  I2Ctwo.beginTransmission(TCA9548A_I2C_ADDRESS);
  I2Ctwo.write(1 << i);
  I2Ctwo.endTransmission();  
  //  delay(5); // Pause
}

void reset_AP(void){
  ap_powered_on = false;
  ap_preflight_PFT=false;
  ap_preflight_showall=false;
  ap_preflight_complete=false;
  ap_baro_initialised=false;  // not used yet
  ap_engaged = false;
  ap_disengaging = false;
  ap_display_state = false;
  changedArmState = false;
  NavArmState = false;
  AprArmState = false;
  RevArmState = false;
}

//****************** Display methods

void power_off_display(void) {
  setTCAChannel(KAP140_DISPLAY_CENTRE);
  display.clearDisplay();
  display.display();

  setTCAChannel(KAP140_DISPLAY_RIGHT);
  display.clearDisplay();
  display.display();

  setTCAChannel(KAP140_DISPLAY_LEFT);
  display.clearDisplay();
  display.display();
}

void startupSequence(void) {
  // Serial.println("Startup");
  if (ap_preflight_PFT && !ap_preflight_showall) {
 //   Serial.println("PFT");
      display_preFlightTest();
    }

    if (ap_preflight_PFT && ap_preflight_showall) {
 //     Serial.println("Display");
      PFT_step=0; // reset PFT step count
      display_DisplayTest();
    }
}

void display_preFlightTest()
{
  setTCAChannel(KAP140_DISPLAY_CENTRE);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);   

  _display_upperValue("PFT");
  display.display();

  setTCAChannel(KAP140_DISPLAY_RIGHT);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);   
  display.setFont(&DSEG7Classic_Italic14pt7b);
  display.setCursor(4,30);

  display.println(PFT_step);

  display.display();

  setTCAChannel(KAP140_DISPLAY_LEFT);
  display.clearDisplay();
  display.display();
}

void display_DisplayTest()
{
  displayTestRight();
  displayTestMiddle();
  displayTestLeft();
}

void displayTestRight(void)
{
  setTCAChannel(KAP140_DISPLAY_RIGHT);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);        // Draw white text
 
  display_alert();
  display_ft();
  display_hpa();
  display_inhg();
  display_fpm();

  //7 seg, all-on is 8
  display_rightblock("88888");

  display.display();
}

void displayTestMiddle(void)
{ 
  setTCAChannel(KAP140_DISPLAY_CENTRE);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);        // Draw white text

    //display in upper ALT/VS position
  display.setFont(&DSEG14Classic_Italic14pt7b);
  display.setCursor(20,25);             
  display.println("888");
  display.setCursor(20,25);             
  display.println("8ST");

  // display in ALT position
  display.setFont(&DSEG14Classic_Italic14pt7b);
  display.setCursor(20,61);             
  display.println("888");
  display.setCursor(20,61);
  display.println("T8T");
  
  display_ARM();
  display_PitchTrimUP();
  display_PitchTrimText();
  display_PitchTrimDOWN();

  display.display();
}

void displayTestLeft(void)
{ 
  setTCAChannel(KAP140_DISPLAY_LEFT);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);        // Draw white text

  display.setFont(&DSEG14Classic_Italic14pt7b);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20,25);             
  display.println("888");
  display.setCursor(20,25); 
  display.println("S)S");

  display.setFont(&DSEG14Classic_Regular14pt7b);
  display.setCursor(20,61);             
  display.println("888");
  display.setCursor(20,61);        
  display.println("S8S");

  display_ARM();
  display_AP_Symbol();
  // display YD symbol
  
  display.display();
}

void updateDisplayRight(void)
{
  setTCAChannel(KAP140_DISPLAY_RIGHT);

  // Clear the buffer
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);        // Draw white text
  
  if(!ap_baro_initialised)
  {
    currentMillis=millis();
    if(currentMillis - startMillis >= flashperiod)
    {      
      rightblock_display_state = !rightblock_display_state;      
      startMillis = currentMillis;
    }
  }
  else {
    rightblock_display_state = true;
  }

  if(ap_alert_solid)
  {
    alert_display_state = true;
  }
  else if (ap_alert_flash) {
    currentMillis=millis();
    if(currentMillis - startMillis >= flashperiod)
    {      
      alert_display_state = !alert_display_state;      
      startMillis = currentMillis;
    }
  } else {
    alert_display_state = false;
  }

  if (rightblock_display_state){
    if(RightBlockMode == 0) // showing target alt
    {
      display_ft();
    }
    else if(RightBlockMode == 1)   // showing vs fpm
    {
      display_fpm();
    }
    else if(RightBlockMode == 2)   // showing baro
    {
      if(BaroDisplay && BaroMode == 1) // Baro is in HG
      {
        display_inhg();        
      }

      if(BaroDisplay && BaroMode == 0) // Baro is in HPA
      {
        display_hpa();
      }
    }

    if (alert_display_state)
    {
      display_alert();
    }
    
    display_rightblock(rightBlockValue);
  }

    // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();  
}

void updateDisplayCentre(void)
{
  setTCAChannel(KAP140_DISPLAY_CENTRE);
  
 // Clear the buffer
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);        // Draw white text

  if (VertHold) {
    display_VS();
  }

  if (AltHold) {
    display_ALT();
  }

  if (AltArm) {
    display_ALTARM();
  }
 
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
}

void updateDisplayLeft(void)
{
  setTCAChannel(KAP140_DISPLAY_LEFT);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE); 
  
  if (!ap_disengaging){
    if (ap_engaged) {
      if(!(changedArmState && ROL_WingLeveler)){
        if(ROL_HDG){
          display_ROLLMODE_HDG();
        }
        
        if(ROL_WingLeveler){
          display_ROLLMODE_ROL();
        }

        if(ROL_NAV){
          display_ROLLMODE_NAV();
        }

        if(ROL_APR){
          display_ROLLMODE_APR();
        }

        if(ROL_REV){
          display_ROLLMODE_REV();
        }
      }

      if(NavArm) {
        display_NAVARM(); 
        if(!NavArmState){
          changedArmState = true;   
          NavArmState = true;
          AprArmState = false;
          RevArmState = false;       
        }
      }

      if(AprArm) {
        display_APRARM();
        if(!AprArmState){
          changedArmState = true;
          NavArmState = false;
          AprArmState = true;
          RevArmState = false;
        }
      }

      if(RevArm) {
        display_REVARM();
        if(!RevArmState){
          changedArmState = true;
          NavArmState = false;
          AprArmState = false;
          RevArmState = true;
        }
      }

      if (!(NavArm || AprArm || RevArm))
      {
        changedArmState = false;
        NavArmState = false;
        AprArmState = false;
        RevArmState = false;
      }

      // Flash HDG if in ROL mode and the ARM state has changed.
      if (ROL_WingLeveler && changedArmState){
        currentMillis=millis();
        if(currentMillis - startMillis >= flashperiod)
        {      
          ap_display_state = !ap_display_state;          
          startMillis = currentMillis;
          flashcount = flashcount+1;
        }
        
        if(ap_display_state){
          display_ROLLMODE_HDG();
        }
        if (flashcount >= ap_flash_max*2) {
          changedArmState = false;
          flashcount=0;
        }
      }
    }
  }

  if(ROL_HDG || ROL_WingLeveler || ROL_NAV || ROL_APR || ROL_REV) {
    display_AP_Symbol();
    ap_engaged=true;
  } else {  // AP no longer engaged
    if(ap_engaged) // if AP was engaged, then start disengaging routine
    {
      flashcount=0;
      startMillis=millis();
      ap_display_state = true;
      ap_disengaging=true;
    }
    ap_engaged=false;
  }

  // Flash AP and the AP symbol to indication AP disengage
  if (ap_disengaging){  
    currentMillis=millis();
    if(currentMillis - startMillis >= flashperiod)
    {      
      ap_display_state = !ap_display_state;      
      startMillis = currentMillis;
      flashcount = flashcount+1;
    }

    if (ap_display_state){
      display_AP();
      display_AP_Symbol();
    }

    if (flashcount >= ap_flash_max*2) {
      ap_disengaging = false;
      flashcount=0;
    }
  }
 
  display.display();
}

// Right hand section
void display_inhg(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(80,61);             
  display.println("IN  HG");
}

void display_hpa(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(25,61);             
  display.println("HPA");
}

void display_ft(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(102,47);             
  display.println("FT");
}

void display_fpm(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(66,47);             
  display.println("FPM");
}

void display_alert(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(10,47);             
  display.println("ALERT");
}

void display_rightblock(String _rightBlockValue)
{
  int stringLength = _rightBlockValue.length();
  String rightString = "";
  String leftString = "";

  if(stringLength <=3 ) {
    rightString = _rightBlockValue;
  } else if (stringLength > 3) {
    rightString = _rightBlockValue.substring(stringLength-3);
    leftString = _rightBlockValue.substring(0,stringLength-3);
  }

  // display right part of the value
  display.setFont(&DSEG7Classic_Italic14pt7b);
  switch (stringLength)
  {
    case 1:
      display.setCursor(97,26);    
      break;
    case 2:
      display.setCursor(76,26);    
      break;
    default:
      display.setCursor(55,26);    
      break;
  }
  display.println(rightString);
    
  // display left part of the value (to the left of the comma)
  if (leftString.length() > 0) {
    display.setFont(&DSEG7Classic_Italic14pt7b);
    if (leftString.length()==2){  
      display.setCursor(4,26); 
    } else if (leftString.length()==1) {
      display.setCursor(25,26); 
    } 
    display.println(leftString);

    if(RightBlockMode != 1) // don't show the comma in VS mode
    {
      // if we're displaying the left string, display the comma too
      display.setFont(&DSEG14Classic_Italic14pt7b);
      display.setCursor(43,34);             
      display.println(",");
    }
  }
}

// Middle section
void display_ALT(void)
{
  _display_upperValue("ALT");
}

void display_VS(void)
{
  _display_upperValue("VS");
}

void display_ALTARM(void)
{  
  _display_lowerValue("ALT");
  display_ARM();
}

void display_PitchTrimUP(void)
{
  display.fillTriangle(115,15,123,15,119,4,SH110X_WHITE);
}

void display_PitchTrimText(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setCursor(116,30);
  display.println("P");
  display.setCursor(116,42);
  display.println("T");
}

void display_PitchTrimDOWN(void)
{
  display.fillTriangle(115,47,123,47,119,58,SH110X_WHITE);
}

// Left section

void display_ROLLMODE_ROL(void)
{
  _display_upperValue("ROL");
}

void display_ROLLMODE_NAV(void)
{
  _display_upperValue("NAV");;
}

void display_ROLLMODE_HDG(void)
{
  _display_upperValue("HDG");
}

void display_ROLLMODE_REV(void)
{
  _display_upperValue("REV");
}

void display_ROLLMODE_APR(void)
{
  _display_upperValue("APR");
}

void display_AP(void)
{
  _display_upperValue("AP");
}

void display_AP_Symbol(void)
{
  display.setFont(&FreeSans6pt7b);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(107,14);             
  display.println("AP");
  display.drawFastHLine(105, 2, 19, SH110X_WHITE);
  display.drawFastVLine(103, 4, 13, SH110X_WHITE);
  display.drawFastHLine(105, 18, 19, SH110X_WHITE);
  display.drawFastVLine(125, 4, 13, SH110X_WHITE);
}

void display_NAVARM(void)
{
  _display_lowerValue("NAV");
  display_ARM();
}

void display_REVARM(void)
{
  _display_lowerValue("REV");
  display_ARM();
}

void display_APRARM(void)
{
  _display_lowerValue("APR");
  display_ARM();
}

void display_ARM(void)
{  
  _display_ARM(90,36);
}

void display_GS(void)
{
  _display_lowerValue("GS");
}

// internal helper functions

void _display_upperValue(String textToShow)
{
  display.setFont(&DSEG14Classic_Italic14pt7b);

  switch (textToShow.length())
  {
    case 1:
      display.setCursor(70,26);
      break;
    case 2:
      display.setCursor(45,26);  
      break;
    case 3:
      display.setCursor(20,26);  
      break;
    default:
      break;
  }
  
  display.println(textToShow);
}

void _display_lowerValue(String textToShow) {
  display.setFont(&DSEG14Classic_Regular14pt7b);

  switch (textToShow.length())
  {
    case 1:
      display.setCursor(70,61);
      break;
    case 2:
      display.setCursor(45,61);  
      break;
    case 3:
      display.setCursor(20,61);  
      break;
    default:
      break;
  }
  display.println(textToShow);
}

void _display_ARM(int x, int y) {
  display.setFont(&FreeSans6pt7b);
  display.setCursor(x,y);
  display.println("A");
  display.setCursor(x,y+12);
  display.println("R");
  display.setCursor(x,y+24);
  display.println("M");
}
