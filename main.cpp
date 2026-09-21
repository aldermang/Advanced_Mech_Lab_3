///////////////////////////////////////////////////// SETUP ///////////////////////////////////////////////////// 
//calls in libraries
#include <Arduino.h>
#include <time.h>
#include <Adafruit_ST7789.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"


//define display screen
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240, 135);


//define states for "hvacState", "menuState", and "tempState"
enum hvacState {
  Heating, //defines state 0 as "Heating"
  Cooling, //defines state 1 as "Cooling"
  hCount //defines state 2
};

enum menuState {
  TemperatureMenu, //0
  OperationMenu, //1
  UnitMenu, //2
  mCount //3
};

enum tempState {
  C, //0
  F, //1
  tCount //2
};


//define initial states of the "hvacState", "menuState", and "tempState"
hvacState opMode = Heating; //system starts in heating mode
menuState menuMode = TemperatureMenu; //system starts in temp menu
tempState tempMode = C;


//define misc variables
Adafruit_BME680 bme(&Wire); // I2C
//Adafruit_BME680 bme(&Wire1); // example of I2C on another bus
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

float targetTemp = 25.; //sets "targetTemp" value
volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;
long debounceTime = 50; 
volatile bool changeButtonFlag = false;
volatile bool menuButtonFlag = false;


//defines "change" button (D1)
void IRAM_ATTR buttonToChangeThings() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    changeButtonFlag = true;
    prevChangeTime = now; 
  }
}


//defines "menu" button (D2)
void IRAM_ATTR buttonToChangeMenu() {
  long now = millis();
  if (now > prevChangeTimeTwo + debounceTime) {
    menuButtonFlag = true;
    prevChangeTimeTwo = now; 
  }
}


//define "getCurrentTemp" variable
float getCurrentTemp() {
  if(tempMode == tempState::C) {
   return bme.temperature;
  }

  else if(tempMode == tempState::F) {
    return bme.temperature * 9. / 5. + 32.;
  }

  return -1100.;
}


//main setup
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }

  pinMode(1, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToChangeThings, RISING);

  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangeMenu, RISING);

  //set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_2X);

  //turn on screen
  display.init(135, 240);
  display.setRotation(3);
  canvas.setTextColor(ST77XX_GREEN);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, 1);

}


///////////////////////////////////////////////////// MAIN CODE ///////////////////////////////////////////////////// 
void loop() {
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  //float currentTemp = bme.temperature;
  float currentTemp = getCurrentTemp(); 

  //print message on screen 
  canvas.fillScreen(ST77XX_BLUE); //sets background to blue
  canvas.setCursor(0, 20); //sets start position of text
  canvas.print(" Temperature = ");
  canvas.print(currentTemp);
  canvas.print(" ");
  canvas.print((int)tempMode);
  canvas.print(" with target ");
  canvas.println(targetTemp);
  canvas.print(" operating in mode ");
  canvas.print(opMode);
  canvas.print(" in menu ");
  canvas.println(menuMode);
  canvas.println (""); 

  //changes menu mode if button D2 is pressed
  if (menuButtonFlag) {
    menuButtonFlag = false;
    menuMode = (menuState) (((int)menuMode + 1) % (int)menuState::mCount); //button press cycles between menu options (temp/operation/units)
    canvas.print("Moving to menu: ");
    canvas.print(menuMode);
    delay(1000);
  }

  //D1; changes options within the current menu mode as selected above
  if (changeButtonFlag) {
    
    changeButtonFlag = false;

    //menu mode is 0, D1 cycles through "targetTemp" values
    if (menuMode == TemperatureMenu) {
      targetTemp += 1.0;
      if (targetTemp > 30.0) {
        targetTemp = targetTemp - 10.;
      }
    }

    //menu mode is 1, D1 changes between heat(0) or cool(1)
    if (menuMode == OperationMenu) {
      opMode = (hvacState) (((int)opMode + 1) % (int)hvacState::hCount); //button press toggles between heating/cooling

      if (opMode == Heating) {
        if (currentTemp < targetTemp) {
          //Serial.print ("Heat");
          canvas.println("Now in heating mode");
          delay(1000);
        }
      }

      else if (opMode == Cooling) {
        if (currentTemp > targetTemp) {
          //Serial.print ("AC");
          canvas.println("Now in AC mode");
          delay(1000);
        }
      }

    }

    //menu mode is 2, D1 changes between temp units of C(0) or F(1)
    if (menuMode == UnitMenu) {
      tempMode = (tempState) (((int)tempMode + 1) % (int)tempState::tCount);
      getCurrentTemp;
    }

  Serial.println();
  }

  //having this at the end of the code ensures that all messages print to the screen correctly
  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);
  delay(50);

}