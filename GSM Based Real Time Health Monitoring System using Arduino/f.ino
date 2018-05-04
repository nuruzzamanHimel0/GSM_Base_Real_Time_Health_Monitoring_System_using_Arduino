#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
String myName;
String a;
String num;
int sensor_pin = 0;                
int led_pin = 13;                  
volatile int heart_rate;          
volatile int analog_data;              
volatile int time_between_beats = 600;            
volatile boolean pulse_signal = false;    
volatile int beat[10];         //heartbeat values will be sotred in this array    
volatile int peak_value = 512;          
volatile int trough_value = 512;        
volatile int thresh = 525;              
volatile int amplitude = 100;                 
volatile boolean first_heartpulse = true;      
volatile boolean second_heartpulse = false;    
volatile unsigned long samplecounter = 0;   //This counter will tell us the pulse timing
volatile unsigned long lastBeatTime = 0;

SoftwareSerial mySerial(9, 10);
int temp_sensor = 7;       // Pin DS18B20 Sensor is connected to
float temp;      //Variable to store the temperature in
int lowerLimit = 80;      //define the lower threshold for the temperature
int upperLimit = 97;      //define the upper threshold for the temperature
float Temp_alert_val,Temp_shut_val;
int sms_count=0,d_Set;
OneWire oneWirePin(temp_sensor);
DallasTemperature sensors(&oneWirePin);
void setup()
{
  
  mySerial.begin(9600);   
  Serial.begin(9600);   
  pinMode(led_pin,OUTPUT);         
  Serial.println("Enter character for control option:");
  Serial.println("n : Enter your name & mobile num"); 
  Serial.println("i : measure tempareture."); 
  Serial.println("p : measure heart rate.");
  Serial.println("s : send message");
  Serial.println("c : to make a call");
  Serial.println("h : to disconnect a call");
   
 
  
  
  sensors.begin(); 
  interruptSetup();      

}

void loop()
{

   
  if (Serial.available()>0)
   switch(Serial.read())
  {
    case 'c':
      MakeCall();
      break;
    case 'h':
      HangupCall();
      break;
    case 'i':
      for(int b=0;b<=10;b++){
      CheckFire();
      }
      break;
    case 's':
      SendMessage();
      break;
    case 'p':
      CheckPluse();
      break;
    case 'n':
      enter_num();
      break;
      
      
  }
 if (mySerial.available()>0)
Serial.write(mySerial.read());
//CheckFire();

}

void CheckFire()
{
sensors.requestTemperatures(); 
temp = sensors.getTempFByIndex(0);

Serial.print("Temperature :");
Serial.print(temp);
Serial.print("*F");
Serial.println();

}
void MakeCall()
{

  //mySerial.println("ATD+8801775164443;"); // ATDxxxxxxxxxx; -- watch out here for semicolon at the end!!
  mySerial.print("ATD+88"); 
  mySerial.print(num); 
  mySerial.println(";"); 
  Serial.println("Calling  "); // print response over serial port
  delay(1000);
}

void HangupCall()
{
  mySerial.println("ATH");
  Serial.println("Hangup Call");
  delay(1000);
}
void SendMessage()
{
  if(temp>upperLimit){
    a="High";
  }
  else if(temp>lowerLimit && temp<upperLimit){
    a="Normal";
  }
  else if(temp<lowerLimit){
    a="Low";
  }
   mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second
 // mySerial.println("AT+CMGS=\"+8801775164443\"\r"); // Replace x with mobile number
  mySerial.print("AT+CMGS=");
  mySerial.print("\"+88");
  mySerial.print(num);
  mySerial.print("\"");
  mySerial.println("\r");
  delay(1000);
  mySerial.print("Name : ");
  mySerial.print(myName);
  mySerial.print("\nTemperature : ");
  mySerial.print(temp);
  mySerial.print("\nState : ");
  mySerial.print(a);
  mySerial.print("\nHeart-rate :");
  mySerial.println(heart_rate);
  delay(100);
   mySerial.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
}
void CheckPluse(){

  for(int i=0;i<=80;i++){
  Serial.print("BPM: ");
  Serial.println(heart_rate);
  delay(200); //  take a break
  }
}
void interruptSetup()
{    

TCCR2A = 0x02;  // This will disable the PWM on pin 3 and 11
OCR2A = 0X7C;   // This will set the top of count to 124 for the 500Hz sample rate
TCCR2B = 0x06;  // DON'T FORCE COMPARE, 256 PRESCALER
TIMSK2 = 0x02;  // This will enable interrupt on match between OCR2A and Timer
sei();          // This will make sure that the global interrupts are enable
}
ISR(TIMER2_COMPA_vect)
{ 
  cli();                                     
  analog_data = analogRead(sensor_pin);            
  samplecounter += 2;                        
  int N = samplecounter - lastBeatTime;      
  if(analog_data < thresh && N > (time_between_beats/5)*3)
    {     
      if (analog_data < trough_value)
      {                       
        trough_value = analog_data;
      }
    }
  if(analog_data > thresh && analog_data > peak_value)
    {        
      peak_value = analog_data;
    }                          
   if (N > 250)
  {                            
    if ( (analog_data > thresh) && (pulse_signal == false) && (N > (time_between_beats/5)*3) )
      {       
        pulse_signal = true;          
        digitalWrite(led_pin,HIGH);
        time_between_beats = samplecounter - lastBeatTime;
        lastBeatTime = samplecounter;     
       if(second_heartpulse)
        {                        
          second_heartpulse = false;   
          for(int i=0; i<=9; i++)    
          {            
            beat[i] = time_between_beats; //Filling the array with the heart beat values                    
          }
        }
        if(first_heartpulse)
        {                        
          first_heartpulse = false;
          second_heartpulse = true;
          sei();            
          return;           
        }  
      word runningTotal = 0;  
      for(int i=0; i<=8; i++)
        {               
          beat[i] = beat[i+1];
         runningTotal += beat[i];
        }
      beat[9] = time_between_beats;             
      runningTotal += beat[9];   
      runningTotal /= 10;        
      heart_rate = 60000/runningTotal;
    }                      
  }
  if (analog_data < thresh && pulse_signal == true)
    {  
      digitalWrite(led_pin,LOW); 
      pulse_signal = false;             
      amplitude = peak_value - trough_value;
      thresh = amplitude/2 + trough_value; 
      peak_value = thresh;           
      trough_value = thresh;
    }
  if (N > 2500)
    {                          
      thresh = 512;                     
      peak_value = 512;                 
      trough_value = 512;               
      lastBeatTime = samplecounter;     
      first_heartpulse = true;                 
      second_heartpulse = false;               
    }
  sei();                                
}

void enter_num(){
 Serial.println("Please enter your num: "); //Prompt User for input
  while (Serial.available()==0) {             //Wait for user input
   }
  num=Serial.readString(); 
  Serial.println(num);
  Serial.println("Please enter your Name: ");
   while (Serial.available()==0) {             //Wait for user input
   }
   myName=Serial.readString(); 
  Serial.println(myName);
  
}

