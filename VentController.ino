#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <SimpleDHT.h>

LiquidCrystal_PCF8574 lcd(0x27);  // LCD address

const byte dhtPin = 10;
SimpleDHT11 dht11;

byte temperature = 0;
byte humidity = 0;

byte ldrState = 0;
const byte ldrPin = 9;

const byte pwrSrcPin = 8;
const byte fanPin = 7;
const byte jump1Pin = 2;
const byte jump2Pin = 3;
const byte jump3Pin = 4;

byte tick=0;

byte seconds=0;
int minutes=0;

boolean ledState;

enum DeviceState{
  Intro,
  Measure,
  AwaitStart,
  Running,
  Done
};
static const char *strState[] = {"Intro", "Measure", "Await", "Run ", "Done"};

DeviceState deviceState = Intro;
const int IntroTime = 3;  // byte?
const int FanRunLimit = 3660; // 60min * 60s = 3660
int fanRuntime = 0;
int dhtScan = 0;

const byte HumidityThreshold = 70;

void setup() {
  setupTimer();

  Wire.begin();
  Wire.beginTransmission(0x27);
  // error = Wire.endTransmission();
  lcd.begin(16, 2);  
  lcd.setBacklight(1);  

  pinMode(ldrPin, INPUT);

  pinMode(jump1Pin, INPUT_PULLUP);
  pinMode(jump2Pin, INPUT_PULLUP);
  pinMode(jump3Pin, INPUT_PULLUP);

  pinMode(pwrSrcPin, OUTPUT);
  digitalWrite(pwrSrcPin, LOW);
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);

  if(!digitalRead(jump2Pin)){
    lcd.print("Diag");
    while(true){
      digitalWrite(pwrSrcPin, HIGH);
      delay(5000);
      digitalWrite(pwrSrcPin, LOW);
      delay(5000);
    }
  }
  
}

void setupTimer(){
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0  
  TCCR1B |= (1 << WGM12);   // turn on CTC mode
  
  // set compare match register for 0.5 Hz increments
  //OCR1A = 31249; // = 16000000 / (1024 * 0.5) - 1 (must be <65536)  *** 2s  
  // TCCR1B |= (1 << CS12) | (0 << CS11) | (1 << CS10); // Set CS12, CS11 and CS10 bits for 1024 prescaler

  OCR1A = 62499; // = 16000000 / (256 * 1) - 1 (must be <65536)
  TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);  // Set CS12, CS11 and CS10 bits for 256 prescaler
  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

void loop() {
  if (tick == 1){
    secondsTick();
    
    ldrState = digitalRead(ldrPin);

    if(dhtScan<15){
      dhtScan++;
    }
    else{
      if (dht11.read(dhtPin, &temperature, &humidity, NULL)!=0){
        humidity=100;
      }
      dhtScan=0;
    }

    if (deviceState == Intro){
      if(!digitalRead(jump1Pin)){
        digitalWrite(pwrSrcPin, HIGH);
        digitalWrite(fanPin, HIGH);
        deviceState = Running;
      }      
    }

    switch(deviceState){
      case Intro:
          if(minutes >= IntroTime){
            deviceState = Measure;
          }
          break;
      case Measure:
          if(humidity >= HumidityThreshold && humidity < 100){
            digitalWrite(pwrSrcPin, HIGH);
            deviceState = AwaitStart;
          }
          break;
      case AwaitStart:
          if(ldrState == 0){
            digitalWrite(fanPin, HIGH);
            deviceState = Running;
          }
          break;
      case Running:
          fanRuntime++;
          if (fanRuntime >= FanRunLimit){
            digitalWrite(fanPin, LOW);
            digitalWrite(pwrSrcPin, LOW);
            deviceState = Done; 
          }
          break;
      default:
          break;
    }

    lcdShow();
    tick = 0;  
  }
}

void lcdShow(){
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print(minutes);
  lcd.print(":");
  if(seconds<10){
    lcd.print("0");
  }
  lcd.print(seconds);

  // line 1 half 2
  lcd.setCursor(7,0);
  lcd.print(strState[deviceState]);
  if (deviceState == Running){
    lcd.print(fanRuntime);
  }

  // line 2
  lcd.setCursor(0,1);
  lcd.print("L:");
  lcd.print(ldrState);

  lcd.setCursor(4, 1);
  if (humidity < 100){
    lcd.print("H:");
    lcd.print((int)humidity);
    lcd.print(" T:");
    lcd.print((int)temperature);
  }
  else {
    lcd.print("DHT ERR");
  }
}

void secondsTick(){
  seconds++;
  if(seconds==60){
    minutes++;
    seconds=0;
  }  
}

SIGNAL(TIMER1_COMPA_vect){  
  tick=1;
}

