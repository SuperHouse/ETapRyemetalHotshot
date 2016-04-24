/**
 * Electronic tap control software to run on the SuperHouse Automation eTap Controller.
 * This version replicates the functionality of the Ryemetal Hotshot V3 electronic tap.
 */

#include <Wire.h>
#include "Adafruit_VCNL4010.h"

Adafruit_VCNL4010 vcnl;

/* Status LED output pins */
#define HOT_LED   13
#define WARM_LED  A0
#define COLD_LED  A1
#define SHORT_LED A2
#define LONG_LED  A3

/* Water solenoid valve output pins */
#define COLD_VALVE 2
#define WARM_VALVE 3
#define HOT_VALVE  4

/* Possible states for the state machine */
#define STATE_OFF        0
#define STATE_SHORT_COLD 1
#define STATE_SHORT_WARM 2
#define STATE_SHORT_HOT  3
#define STATE_LONG_COLD  4
#define STATE_LONG_WARM  5
#define STATE_LONG_HOT   6

byte currentState          = STATE_OFF;
long currentStateStartTime = 0;
long stateInterval         = 500;   // Interval to pause between changing states
long shortInterval         = 3000;  // Run water for a short time
long longInterval          = 10000; // Run water for a long time
byte handPresent           = false;
byte handPreviouslyPresent = false;
byte handHasDeparted       = false;

/* IR range sensor */
long proximity = 0;      // Latest proximity reading
long threshold = 24300;  // Readings above this indicate a hand is nearby

/**
 * Setup
 */
void setup() {
  Serial.begin(9600);
  Serial.println("eTap starting up in Hotshot V3 mode");

  if (! vcnl.begin()){
    Serial.println("VCNL4010 IR proximity sensor not found!");
    while (1);
  }
  Serial.println("Found VCNL4010 IR proximity sensor");

  pinMode(COLD_VALVE, OUTPUT);
  pinMode(WARM_VALVE, OUTPUT);
  pinMode(HOT_VALVE,  OUTPUT);
  pinMode(HOT_LED,    OUTPUT);
  pinMode(WARM_LED,   OUTPUT);
  pinMode(COLD_LED,   OUTPUT);
  pinMode(SHORT_LED,  OUTPUT);
  pinMode(LONG_LED,   OUTPUT);
  digitalWrite(COLD_VALVE, LOW);
  digitalWrite(WARM_VALVE, LOW);
  digitalWrite(HOT_VALVE,  LOW);
  digitalWrite(HOT_LED,    LOW);
  digitalWrite(WARM_LED,   LOW);
  digitalWrite(COLD_LED,   LOW);
  digitalWrite(SHORT_LED,  LOW);
  digitalWrite(LONG_LED,   LOW);
}

/*
 * Loop
 */
void loop() {
  proximity = vcnl.readProximity();
  //Serial.print("Proximity: "); Serial.println(proximity);
   
  handPreviouslyPresent = handPresent;
  if(proximity > threshold)
  {
    handPresent = true;
  } else {
    handPresent = false;
  }

  // Check if a hand was here, but it's gone now (departure transition)
  if(handPreviouslyPresent == true && handPresent == false)
  {
    handHasDeparted = true;
  } else {
    handHasDeparted = false;
  }

  // Check if there was no hand here, but one has appeared (arrival transition)
  if(handPreviouslyPresent == false && handPresent == true)
  {
    // The hand has just arrived, so force to the off state no matter what was happening previously
    currentState = STATE_OFF;
    currentStateStartTime = millis();
  }

  long currentStateDuration = millis() - currentStateStartTime;

  /*
   * Order is:
   *  Off -> short cold -> short warm -> short hot -> long cold -> long warm -> long hot
   */
  switch( currentState )
  {
     case STATE_OFF:
       setActiveLength( 0 );
       setActiveTemperature( 0 );
       digitalWrite(COLD_VALVE, LOW);
       digitalWrite(WARM_VALVE, LOW);
       digitalWrite(HOT_VALVE,  LOW);
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_SHORT_COLD;
         currentStateStartTime = millis();
       }
       break;
     
     case STATE_SHORT_COLD:
       setActiveLength( SHORT_LED );
       setActiveTemperature( COLD_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, HIGH);
         digitalWrite(WARM_VALVE, LOW);
         digitalWrite(HOT_VALVE,  LOW);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_SHORT_WARM;
         currentStateStartTime = millis();
       } else if( currentStateDuration > shortInterval )
       {
         currentState = STATE_OFF;
       }
       break;
        
     case STATE_SHORT_WARM:
       setActiveLength( SHORT_LED );
       setActiveTemperature( WARM_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, LOW);
         digitalWrite(WARM_VALVE, HIGH);
         digitalWrite(HOT_VALVE,  LOW);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_SHORT_HOT;
         currentStateStartTime = millis();
       } else if( currentStateDuration > shortInterval )
       {
         currentState = STATE_OFF;
       }
       break;
        
     case STATE_SHORT_HOT:
       setActiveLength( SHORT_LED );
       setActiveTemperature( HOT_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, LOW);
         digitalWrite(WARM_VALVE, LOW);
         digitalWrite(HOT_VALVE,  HIGH);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_LONG_COLD;
         currentStateStartTime = millis();
       } else if( currentStateDuration > shortInterval )
       {
         currentState = STATE_OFF;
       }
       break;
        
     case STATE_LONG_COLD:
       setActiveLength( LONG_LED );
       setActiveTemperature( COLD_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, HIGH);
         digitalWrite(WARM_VALVE, LOW);
         digitalWrite(HOT_VALVE,  LOW);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_LONG_WARM;
         currentStateStartTime = millis();
       } else if( currentStateDuration > longInterval )
       {
         currentState = STATE_OFF;
       }
       break;
        
     case STATE_LONG_WARM:
       setActiveLength( LONG_LED );
       setActiveTemperature( WARM_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, LOW);
         digitalWrite(WARM_VALVE, HIGH);
         digitalWrite(HOT_VALVE,  LOW);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_LONG_HOT;
         currentStateStartTime = millis();
       } else if( currentStateDuration > longInterval )
       {
         currentState = STATE_OFF;
       }
       break;
        
     case STATE_LONG_HOT:
       setActiveLength( LONG_LED );
       setActiveTemperature( HOT_LED );
       if( handHasDeparted )
       {
         digitalWrite(COLD_VALVE, LOW);
         digitalWrite(WARM_VALVE, LOW);
         digitalWrite(HOT_VALVE,  HIGH);
       }
       if( handPresent && currentStateDuration > stateInterval )
       {
         currentState = STATE_OFF;
         currentStateStartTime = millis();
       } else if( currentStateDuration > longInterval )
       {
         currentState = STATE_OFF;
       }
       break;

     default:
       currentState = STATE_OFF;
       break;
   }
   //delay(150);
}

/**
 * Turn on one length LED and the other off
 */
void setActiveLength( int activeLed )
{
  digitalWrite( SHORT_LED, LOW );
  digitalWrite( LONG_LED,  LOW );
  if( activeLed != 0 )
  {
    digitalWrite( activeLed, HIGH );
  }
}

/**
 * Turn on one temperature LED and the others off
 */
void setActiveTemperature( int activeLed )
{
  digitalWrite( COLD_LED, LOW );
  digitalWrite( WARM_LED, LOW );
  digitalWrite( HOT_LED,  LOW );
  if( activeLed != 0 )
  {
    digitalWrite( activeLed, HIGH );
  }
}
