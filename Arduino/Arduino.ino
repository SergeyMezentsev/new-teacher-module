#include "GBUSmini.h"

// define pins
#define TX_PIN 10                     // pin connected to tranciever module
#define RX_PIN  5                     // pin connected to receiver module
//#define LED_PRESENCE_PIN A0           //This led allows to know if all modules are present      //Uncomment if u need dynamic indication of modules presence
#define GET_NUMBER_OF_MODULES_PIN 4   //This pin allows to get number of modules
#define PRESENCE_STATE_PIN  7         //This allows to show presence to the esp8266

byte numberOfModules = 0;             //Maximum number of modules that will be checked

//settings for the radioprotocol
#define RF_ERR 500              //The shortest duration of the pulse
#define NUM_OF_START_PULSES 3   //Number of the shortest durations (RF_ERR) in every pulse in the teacher's message
#define RF_START_TEACHER  5     // Number of start pulses in the teacher's message
#define RF_START_CHILD  3       //Number of start pulses in preambule of every children's message

//-----------------------------------------------------------------------------------------------------
//function for teacher's message
void teacherMSGSend(void)
{
  for( uint8_t i = RF_START_TEACHER; i; i--)
  {
  digitalWrite(TX_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delayMicroseconds(NUM_OF_START_PULSES*RF_ERR);
  digitalWrite(TX_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delayMicroseconds(NUM_OF_START_PULSES*RF_ERR);
  }
}

//-----------------------------------------------------------------------------------------------------
//Time period checks:
signed long timeLeft = 1600;
unsigned long currentTime = 0;

//-----------------------------------------------------------------------------------------------------
//Getting adresses from childrens' modules
uint8_t getAddr(void)
{
  unsigned long duration = 0;
  uint8_t counter = RF_START_CHILD;
  uint8_t flag = 1;
  do {
    duration = pulseIn(RX_PIN, HIGH, 14 * RF_ERR);
    if ((duration < (2 * RF_ERR)) || (duration > (4 * RF_ERR))) {flag = 0; break;}
  } while(--counter);

  uint8_t data = 1;
  if (flag){
    delayMicroseconds(3 * RF_ERR - 200);
    // read ADDR
    for(uint8_t i=8; i; i--) {
      duration = pulseIn(RX_PIN, HIGH, 14 * RF_ERR);
      data <<= 1;
      if (duration > (2*RF_ERR - 200)) data |= 1;
    }
  }

  if (flag)
    return data;
  else
    return 0;
}

//-----------------------------------------------------------------------------------------------------
//Checking presence of all modules
uint8_t arrPresence[32] = {0};    //If the counter here is more than 5, the module hasn't been available more than for 5 cycles
bool absentModules[32] = {0};     // If the counter is more than 5, so here we will set the flag that shows us absence of the module

void incArr()
{
  for (uint8_t i = 0; i < numberOfModules; i++)
    if(arrPresence[i] <= 5) arrPresence[i]++;
}

bool allModulesPresent()
{
  bool presenceFlag = 1;
  for (uint8_t i = 0; i < numberOfModules; i++)
    if (arrPresence[i] >= 5)
    {
      presenceFlag = 0;
      absentModules[i] = true;
    }

  if(presenceFlag)
    return true;
  else
    return false;
}

void printAbsentModules()
{
  Serial.println("---ALERT!---");
  Serial.println("Absent Modules:");
  for (uint8_t i = 0; i < numberOfModules; i++)
    if(absentModules[i] == true)
    {
      Serial.print("Module:\t"); Serial.println(i+1); Serial.print("\n");
    }
}

//-----------------------------------------------------------------------------------------------------
void setup() {

  Serial.begin(9600);
  
  //Configuring pins
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);

  pinMode(RX_PIN, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //pinMode(LED_PRESENCE_PIN, OUTPUT);    //Uncomment if u need dynamic indicator of modules presence
  //digitalWrite(LED_PRESENCE_PIN, LOW);  //Uncomment if u need dynamic indicator of modules presence

  pinMode(PRESENCE_STATE_PIN, OUTPUT);
  digitalWrite(PRESENCE_STATE_PIN, LOW);

  pinMode(GET_NUMBER_OF_MODULES_PIN, INPUT_PULLUP);   //This is required for "GBUS" library

  while( !(GBUS_read(GET_NUMBER_OF_MODULES_PIN, 3, &numberOfModules, sizeof(numberOfModules))) )  ;
  
  Serial.print("got: "); Serial.println(numberOfModules);

}

void loop(){
  
  teacherMSGSend();
  delay(20);

  //increment all sites for modules in the array
  incArr();

  //start to listen to childrens' modules
  currentTime = millis();
  while(timeLeft > 0)
  {

    //Get addresses
    uint8_t addr = 0;
    if( (addr = getAddr()) != 0){
      Serial.print("Address: "); Serial.println(addr, HEX);
      arrPresence[addr-1] = 0;
      absentModules[addr-1] = false;
    }
    

    //Check if the period of listening to module is out of time
    if(millis() - currentTime >= 70)
    {
      timeLeft -= (millis() - currentTime);
      currentTime = millis();
    }
  }
  
  //Check for presence of all modules
  if(!allModulesPresent())
  {
    digitalWrite(PRESENCE_STATE_PIN, HIGH);   //Allows to send the absence signal to esp8266
    
    printAbsentModules();

    //Uncomment if u need dynamic indicator of modules presence
    /*
    digitalWrite(LED_PRESENCE_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PRESENCE_PIN, LOW);
    */
  }
  else { digitalWrite(PRESENCE_STATE_PIN, LOW); }
  
  timeLeft = 1600;
  delay(200);
}
