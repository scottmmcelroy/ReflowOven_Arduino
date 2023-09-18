#include <ESP32_C3_TimerInterrupt.h>
#include <ESP32_C3_ISR_Timer.h>
#include <ESP32_C3_ISR_Timer.hpp>
#include <Adafruit_MAX31865.h>

int TOP_HEATER = D0; //Top Heater connection
int BOTTOM_HEATER = D1; //Bottom Heater connection 
int BUTTON = D2; //button press for reset
//******MAX31865 inits***********
int CS = D7;
int spiMOSI = D10;
int spiMISO = D9;
int spiCLK = D8;
Adafruit_MAX31865 thermo = Adafruit_MAX31865(CS, spiMOSI, spiMISO, spiCLK);
#define RREF  430 //reference resistor for the PT100
#define RNOMINAL 100.0 //nominal value for the PT100 at 0°C
float rtd = 0;
#define TEMPERATURE_MATCH 29 //28C
float temperature_profile = 0;
//***********************
//Initialize the timer
ESP32Timer ITimer0(0); //start a class of ITimer0
int read_flag = 0;
int time_count = 0;
int arrayPosition = 0;
#define TIMER0_INTERVAL_MS        1000000 //1 second interval
//*************************
//*****Timer ISR Handler***
bool IRAM_ATTR TimerHandler0(void * timerNo){
  //at the timer interrupt interval, trigger read
  read_flag = 1;
}

//*************************
//*******kill switch*******
int KILL_SWITCH = D3; //kill switch
int killSwitchStatus = 0;
//*******Kill Switch interrupt
void killSwitchISR(void){
  //reset all variables and reset
  killSwitchStatus = LOW;
}

//*************************
#define reflowDataMax 270 //270 elements
float reflowData[reflowDataMax];
//reflow data
int numberOfItems = 5;
float tempInterval[] = {25, 150, 175, 217, 249, 217};
float timeInterval[] = {0, 90, 180, 210, 240, 270};
//************************
void setup() {
  int i = 0;
  // put your setup code here, to run once:
  Serial.begin(115200);

  //reflowData array fill based on inputs
  reflowDataArrayFill(numberOfItems);

  //delay for the filling of the array
  delay(10000);

  //debug: check if the array filled properly
  Serial.println("***************************************");
  for(i=0; i<270; i++){
    
    Serial.print("reflowData[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(reflowData[i]);
  }
  Serial.println("***************************************");

  //Serial.println("test!!!");
  //Pin set for LED
  pinMode(TOP_HEATER, OUTPUT);
  pinMode(BOTTOM_HEATER, OUTPUT);
  digitalWrite(TOP_HEATER, LOW);
  digitalWrite(BOTTOM_HEATER, LOW);
  //button press for the reset
  pinMode(BUTTON, INPUT);
  //kill switch interrupt
  pinMode(KILL_SWITCH, INPUT);
  attachInterrupt(digitPinToInterrupt(KILL_SWITCH), killSwitchISR, LOW);

  //wait for button to be triggered
  Serial.println("Waiting for button press");
  while(digitalRead(BUTTON));

  //setup the max31865 as 3 wire
  thermo.begin(MAX31865_3WIRE);
  //pinMode(TEST, OUTPUT);
  
  //turn on the 1 second timer interrupt
  ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS, TimerHandler0); 
} 	

//*****************************************************
//***************Loop***********
//*****************************************************
void loop() {
  // put your main code here, to run repeatedly:
  int buttonVal = HIGH;

  //if the kill switch is trigger jump to program trap
  if(killSwitchStatus == 0){
    //set the array position to run into the trap
    arrayPosition = 300;
    Serial.println("Kill Switch Engaged...completing program");
  }

  //if the read_flag is triggered, read RTD
  if(read_flag == 1){
    //read the temperature
    read_flag = 0;
    
    //move the position and time count except for the first one
    if(time_count == 0){
      time_count++;
      arrayPosition = 0;
    }else{
      //update each position count
      time_count++;
      arrayPosition++;
    }

    //********************
    //****program trap****
    //********************
    if(arrayPosition >= 270){
      //terminate program if the entire cycle is complete
      digitalWrite(TOP_HEATER, LOW);
      digitalWrite(BOTTOM_HEATER, LOW);
      Serial.println("Cycle Complete");
      Serial.println("Waiting for button press to restart");
      //input a button press to reset everything
      while(buttonVal == HIGH){
        //If a button is press, break out of the loop
        buttonVal = digitalRead(BUTTON);
        //read the rtd temperature so that the user know the oven temp while cooling down
        readRTD();
        Serial.println("*********");
        //if the button was pressed and goes low, reset everything
        if(buttonVal == LOW){
          //reset positional values
          arrayPosition = 0;
          time_count = 0;
          //reload the reflowArrays - future if new reflow is needed
        }
      }
    }//arrayPosition is at max value
    
    //read the rtd temp
    rtd = readRTD();
    
    temperature_profile = reflowData[arrayPosition];
    Serial.print("temperature_profile = ");
    Serial.println(temperature_profile);
    //Comparing temperature value with the RTD read
    if(rtd >= (temperature_profile+1) ){
      //turn off switch
      digitalWrite(TOP_HEATER, LOW);
      digitalWrite(BOTTOM_HEATER, LOW);
      Serial.println("Switch-off");
      //test
      Serial.println(digitalRead(BUTTON));
    }else if(rtd <= (temperature_profile-1) ){
      //turn on switch
      digitalWrite(TOP_HEATER, HIGH);
      digitalWrite(BOTTOM_HEATER, HIGH);
      Serial.println("Switch-on");
      //test
      Serial.println(digitalRead(BUTTON));
    }

    Serial.println("  ");
  }//end read flag

}//end of loop




//*************************************************************************
//***********Functions******************
//*************************************************************************
//function calculates the time interval vs temp slope for each interval
//requires a global array timeIntervals and TempIntervals
float timeIntervalCount(int arrayItemNumber){
  float timeIntervalStep = 0;
  float tempIntervalStep = 0;
  float result = 0;
  //calculate the number of steps 
  //timeIntervalStep = (timeInterval[arrayItemNumber] - timeInterval[arrayItemNumber - 1]);
  timeIntervalStep = (timeInterval[arrayItemNumber] - timeInterval[ (arrayItemNumber-1) ]);
  //tempIntervalStep = (tempInterval[arrayItemNumber] - tempInterval[arrayItemNumber - 1]);
  tempIntervalStep = (tempInterval[arrayItemNumber] - tempInterval[ (arrayItemNumber-1) ] );
  Serial.print("timeIntervalStep = ");
  Serial.println(timeIntervalStep);
  Serial.print("tempIntervalStep = ");
  Serial.println(tempIntervalStep);
  //result temperature step per second
  result = (tempIntervalStep / timeIntervalStep);
  return result;
}//end of timeIntervalCount

//fill the complete array to be used for comparison
//need to have a global array of reflowData
void arrayFill(int arrayItemNumber, float tempIntervalStep){
  int i = 0;
  //loop to fill the array for the interval in the data array
  for(i=timeInterval[ (arrayItemNumber - 1) ]; i<(timeInterval[arrayItemNumber]); i++){
    //fill the next interval with the previous + the temperature step
    if(i==0){
      //start the first value at 25C
      reflowData[i] = 25;
    }else{
      reflowData[i] = (reflowData[i-1] + tempIntervalStep);
    }
  }
}//end of arrayFill

//global array fill
bool reflowDataArrayFill(int numberOfItems){
  float arrayItemNumber = 1;
  float stepVal = 0;
  //fill the complete array
  for(arrayItemNumber=1 ; arrayItemNumber<=numberOfItems; arrayItemNumber++){
    //calculate stepInterval
    stepVal = timeIntervalCount(arrayItemNumber);
    arrayFill(arrayItemNumber, stepVal);
  }

  return true;
}

//read the temp sensor function
float readRTD(void){
  float rtd_local = 0.0;
  rtd_local = thermo.temperature(RNOMINAL, RREF);
  Serial.print("RTD Temperature Reading = "); 
  Serial.println(rtd_local);

  return rtd_local;
}

//*******************************************************************************
//*********************End of file******************
//*******************************************************************************