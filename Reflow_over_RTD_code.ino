#include <ESP32_C3_TimerInterrupt.h>
#include <ESP32_C3_ISR_Timer.h>
#include <ESP32_C3_ISR_Timer.hpp>
#include <Adafruit_MAX31865.h>

int LED = D0; //external LED connection
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
//***********************
//Initialize the tiemr
ESP32Timer ITimer0(0); //start a class of ITimer0
int read_flag = 0;
#define TIMER0_INTERVAL_MS        1000000 //1 second interval
//*************************
//*****Timer ISR Handler***
bool IRAM_ATTR TimerHandler0(void * timerNo){
  //at the timer interrupt interval, trigger read
  read_flag = 1;
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
  float timeInterval_temp1 = 0;
  float timeInterval_temp2 = 0;
  float timeInterval_temp3 = 0;
  float timeInterval_temp4 = 0;
  float timeInterval_temp5 = 0;

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

  Serial.println("test!!!");
  //Pin set for LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  //debug
  //while(1);

  //setup the max31865 as 3 wire
  //thermo.begin(MAX31865_3WIRE);
  //pinMode(TEST, OUTPUT);
  ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS, TimerHandler0); 
} 	

//************************
void loop() {
  // put your main code here, to run repeatedly:
  
  //if the read_flag is triggered, read RTD
  if(read_flag == 1){
    //read the temperature
    read_flag = 0;
    //read the rtd temp
    //rtd = thermo.temperature(RNOMINAL, RREF);
    Serial.print("Temperature = "); Serial.println(rtd);

    //Comparing temperature value with the RTD read
    if(rtd >= (TEMPERATURE_MATCH+1) ){
      //turn off switch
      digitalWrite(LED, LOW);
    }else if(rtd <= (TEMPERATURE_MATCH-1) ){
      //turn on switch
      digitalWrite(LED, HIGH);
    }
  }//end read flag

}//end of loop

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
