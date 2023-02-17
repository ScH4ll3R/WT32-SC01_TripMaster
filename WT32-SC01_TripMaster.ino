// Libraries
#include <TinyGPS++.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <EEPROM.h>

// Icons
#include "sat.h"
#include "redsave.h"
#include "greensave.h"
#include "splash.h"

#define GPS_BAUD 9600           // GPS module baud rate. GP3906 defaults to 9600.
#define RXD2 27                 // Connect GPS Tx Here
#define TXD2 16                 // Not used
#define PIN_INCREASE_BTN 25     // Connect trip increase button here
#define PIN_DECREASE_BTN 33     // Connect trip decrease button here
#define EEPROM_SIZE 5           // Memory size to store user params and trip
#define swVersion "V1.0"        // Current SW version
#define intMemPos 4           // Position in memory to store the integer value of the trip
#define decMemPos 5           // Position in memory to store the decimal value of the trip
#define screenMemPos 3        // Position in memory to store the user defined screen rotation

#define SerialMonitor Serial

TFT_eSPI tft = TFT_eSPI();
TinyGPSPlus tinyGPS;

// User params
uint16_t FG_COLOR = TFT_BLACK;  // Screen foreground color
uint16_t BG_COLOR = TFT_WHITE;  // Screen background color
int screenRotation = 1;         // Screen orientation (1 or 3; both landscape)

// GPS Information
double currentLatitude = 0.0;
double currentLongitude = 0.0;
double previousLatitude = 0.0;
double previousLongitude = 0.0;
double currentSpeed = 0.0;
double tripPartial = 0.0;

// GPS Precision
bool gpsFix = true;
int gpsPrecision = 1000;
bool gpsFound = false;

long refreshms = 0;           // Count to perform GPS updates every X cycles
int holdClick = 0;            // Count to perform extra actions holding buttons
bool savedProgress = false;   // Boolean to indicate if the current trip has been stored into memory

/*
   Starts the tft object and sets its color scheme
*/
void initScreen() {
  tft.init();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, 128);
  tft.setTextColor(FG_COLOR, BG_COLOR);
  tft.setRotation(screenRotation);
  tft.setSwapBytes(true);
}

/*
   Main screen of the app
*/
void bgGPS() {
  tft.fillScreen(BG_COLOR);

  tft.drawString("Trip", 10, 20, 4);
  tft.drawString("CAP", 10, 180, 4);

  tft.drawLine( 159, 160, 159, 320, FG_COLOR);
  tft.drawLine( 160, 160, 160, 320, FG_COLOR);
  tft.drawLine( 161, 160, 161, 320, FG_COLOR);

  tft.drawLine( 0, 160, 480, 160, FG_COLOR);
  tft.drawLine( 0, 161, 480, 161, FG_COLOR);
  tft.drawLine( 0, 159, 480, 159, FG_COLOR);

  tft.pushImage(455, 5, 20, 20, sat);
  tft.pushImage(455, 290, 20, 20, greensave);
}

/*
   Waiting screen while there's no fixed GPS satelites
*/
void gpsWaitScreen() {
  tft.fillScreen(BG_COLOR);
  tft.drawString("WAITING FOR", 140, 100, 4);
  tft.drawString("GPS SIGNAL", 150, 150, 4);
}

/*
   Configuration Meny
*/
void printCfgMenu(int option) {
  if (option == 1) {
    tft.setTextColor(BG_COLOR, FG_COLOR);
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.drawString("EXIT", 30, 90, 4);
  } else if (option == 2) {
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.setTextColor(BG_COLOR, FG_COLOR);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
    tft.drawString("EXIT", 30, 90, 4);
  } else if (option == 3) {
    tft.drawString("Screen Rotation", 30, 30, 4);
    tft.drawString("Screen Color", 30, 60, 4);
    tft.setTextColor(BG_COLOR, FG_COLOR);
    tft.drawString("EXIT", 30, 90, 4);
    tft.setTextColor(FG_COLOR, BG_COLOR);
  }
  tft.drawString(swVersion, 455, 290, 3);
  delay(100);
}

void handleCfgMenu() {
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(FG_COLOR, BG_COLOR);
  tft.drawString("ENTERING CFG MODE...", 10, 30, 4);
  delay(2000);
  tft.fillScreen(BG_COLOR);
  boolean exit = false;
  int option = 1;
  while (!exit) {
    tft.drawString("CFG MODE", 30, 30, 3);
    printCfgMenu(option);
    if (digitalRead(PIN_INCREASE_BTN) == LOW) {
      if (option > 3) {
        option = 1;
      } else {
        option++;
      }
    }
    if (digitalRead(PIN_DECREASE_BTN) == LOW) {
      if (option == 3) {
        exit = true;
      }
      else if (option == 1) {
        if (screenRotation == 1) {
          screenRotation = 3;
        } else {
          screenRotation = 1;
        }
        tft.fillScreen(BG_COLOR);
        tft.setRotation(screenRotation);
        EEPROM.write(screenMemPos, screenRotation);
        EEPROM.commit();
      } else if (option == 2) {
        uint16_t flip = BG_COLOR;
        BG_COLOR = FG_COLOR;
        FG_COLOR = flip;
        tft.fillScreen(BG_COLOR);
      }
    }
  }
}


/*
   Main setup method, initializes Serial interfaces, memory storage
   sets button pins and handles the entry to the configuration menu
   by holding the trip increase button during boot
*/
void setup()
{
  // Serial interfaces to read GPS information
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  SerialMonitor.begin(9600);

  // Read stored Trip and screen rotation
  EEPROM.begin(EEPROM_SIZE);
  int storedInt = int(EEPROM.read(intMemPos));
  double storedDec = double(EEPROM.read(decMemPos)) / 100;
  tripPartial = storedInt + storedDec;
  screenRotation = int(EEPROM.read(screenMemPos));

  // Increase and decrease button pins
  pinMode(PIN_DECREASE_BTN, INPUT_PULLUP);
  pinMode(PIN_INCREASE_BTN, INPUT_PULLUP);

  // Screen initialization
  initScreen();
  tft.pushImage(0, 0, 480, 320, splash);
  delay(800);

  // Check if during boot the increase button is pressed
  int increaseBtnVal = digitalRead(PIN_INCREASE_BTN);
  if (increaseBtnVal == LOW) {
    handleCfgMenu();
  }

  // Stop the ESP32 bluetooth to avoid interference
  btStop();


  //  gpsWaitScreen();
  //
  //  while (!gpsFound) {
  //    updateGpsValues();
  //    smartDelay(100);
  //    if (tinyGPS.satellites.value() > 0) {
  //      gpsFound = true;
  bgGPS();
  //    }
  //  }
}

/*
   Obtains the information from the GPS and stores it in global variables
*/
void updateGpsValues() {
  if (tinyGPS.location.age() > 3000 || !tinyGPS.location.isValid()) {
    gpsFix = false;
  } else {
    gpsFix = true;
  }

  if (tinyGPS.hdop.isValid()) {
    gpsPrecision = tinyGPS.hdop.value();
  } else {
    gpsPrecision = 1000;
  }

  if (tinyGPS.location.isValid()) {
    currentLatitude = tinyGPS.location.lat();
    currentLongitude = tinyGPS.location.lng();
    currentSpeed = tinyGPS.speed.kmph();
    updateDistance();
  } else {
    currentLatitude = 0;
    currentLongitude = 0;
  }
}

/*
   Updates the total trip only when the position changes and the vehicle speed
   is over 5 kph in order to filter GPS jitter

   If the vehicle moves over 5 kph it will save the current trip to memory when the
   vehicle comes to a halt.
*/
void updateDistance() {
  if (currentLatitude != previousLatitude || currentLongitude != previousLongitude) {
    // Position has changed. Let's calculate the distance between points.

    double distanceMts =
      tinyGPS.distanceBetween(
        previousLatitude,
        previousLongitude,
        currentLatitude,
        currentLongitude);

    double distanceKms = distanceMts / 1000.0;

    // Update distances
    if (previousLatitude != 0) { // This fixes a big jump in the first update
      if (currentSpeed > 5 && gpsPrecision < 500 && gpsFix) { // This fixes the GPS jitter
        // Update the distance only if I have a decent signal and speed is higher than 5 km/h
        tripPartial += distanceKms;
        savedProgress = false;
        tft.pushImage(455, 290, 20, 20, redsave);
      }
    }

    // If the vehicle stops and the value has not been stored to memory it stores the value
    if (currentSpeed < 5 && !savedProgress) {
      // Save Trip
      int intTrip = tripPartial;
      double decTrip = (tripPartial - intTrip) * 100;
      EEPROM.write(intMemPos, intTrip);
      EEPROM.write(decMemPos, decTrip);
      EEPROM.commit();
      savedProgress = true;
      tft.pushImage(455, 290, 20, 20, greensave);
    }

    // Update previous data
    previousLatitude = currentLatitude;
    previousLongitude = currentLongitude;
  }
}

/*
 * Method to handle button presses.
 * Trip increase button:
 *  - Press once to increase 0.01 km
 *  - Hold for a while to increase by 0.1 km each loop
 *  - Keep holding to increase by 1 km each loop
 * Trip decrease button:
 *  - Press once to decrease 0.01 km
 *  - Hold for a while to decrease by 0.1 km each loop
 *  - Keep holding to decrease by 1 km each loop
 * Hold both buttons to store the curent value to memory manually
 * Hold both buttons for a while to reset the trip to 0 (also saves it)
 */
void handleButtons() {
  int decreaseBtnVal = digitalRead(PIN_DECREASE_BTN);
  int increaseBtnVal = digitalRead(PIN_INCREASE_BTN);

  // Store or reset trip
  if (decreaseBtnVal == LOW && increaseBtnVal == LOW) {
    if (holdClick > 40) {
      // Reset trip and store it
      tripPartial = 0;
      EEPROM.write(intMemPos, 0);
      EEPROM.write(decMemPos, 0);
      EEPROM.commit();
      tft.pushImage(455, 290, 20, 20, greensave);
    } else if (holdClick == 0) {
      // Save Trip to memory
      int intTrip = tripPartial;
      double decTrip = (tripPartial - intTrip) * 100;
      EEPROM.write(intMemPos, intTrip);
      EEPROM.write(decMemPos, decTrip);
      EEPROM.commit();
      savedProgress = true;
      tft.pushImage(455, 290, 20, 20, greensave);
      holdClick ++;
    } else {
      holdClick ++;
    }

  }
  // Decrease values
  else if (decreaseBtnVal == LOW && tripPartial > 0.01) {
    tft.pushImage(455, 290, 20, 20, redsave);
    if (holdClick > 40) {
      tripPartial -= 1;
    }
    else if (holdClick > 40) {
      tripPartial -= 0.1;
    } else {
      tripPartial -= 0.01;
    }
    holdClick ++;
  }
  // Increase values
  else if (increaseBtnVal == LOW) {
    tft.pushImage(455, 290, 20, 20, redsave);
    if (holdClick > 80) {
      tripPartial += 1;
    }
    else if (holdClick > 40) {
      tripPartial += 0.1;
    } else {
      tripPartial += 0.01;
    }
    holdClick ++;
  }
  else {
    holdClick = 0;
  }
}

/*
 * Main loop of the program
 * - Checks for button presses
 * - Updates the GPS values on screen
 * - Updates the clock
 */
void loop() {
  handleButtons();
  printGPSInfo();
  printTime();

  if (refreshms == 500) {
    smartDelay(0);
    updateGpsValues();
    printTime();
    refreshms = 0;
  } else {
    delay(50);
  }
  refreshms += 50;
}

/*
 * Method to format CAP (direction) to three digits
 * so that it refreshes correctly on screen
 */
String formatCAP() {
  int cap = tinyGPS.course.deg();
  if (cap < 10) {
    return "00" + String(int(tinyGPS.course.deg()));
  } else if (cap < 100) {
    return "0" + String(int(tinyGPS.course.deg()));
  } else {
    return String(int(tinyGPS.course.deg()));
  }
}

/*
 * Method to format vehicle speed to three digits
 * so that it refreshes correctly on screen
 */
String formatSpeed() {
  int fspeed = tinyGPS.speed.kmph();
  if (fspeed < 10) {
    return "00" + String(fspeed);
  } else if (fspeed < 100) {
    return "0" + String(fspeed);
  } else {
    return String(fspeed);
  }

}

/*
 * Method to print GPS information on screen
 */
void printGPSInfo()
{
  // Main trip
  tft.drawString(String(tripPartial) + "    ", 30 , 60, 8);
  // Vehicle speed
  tft.drawString(formatSpeed() + " KPH  ", 290 , 190, 8);
  // CAP (direction)
  tft.drawString(formatCAP(), 30 , 220, 7);
  // Number of satellites (red number if the quality is low)
  if (gpsPrecision > 500) {
    tft.setTextColor(TFT_RED, BG_COLOR);
  }
  tft.drawString("  " + String(tinyGPS.satellites.value()), 436 , 6, 2);
  // GPS signal quality (red if the quality is low)
  if(gpsPrecision == 9999){
    tft.drawString("NO SIGNAL",  180 , 290, 4);
  } else {
    tft.drawString("Q: " + String(gpsPrecision) + "        ",  180 , 290, 4);
  }
  tft.setTextColor(FG_COLOR, BG_COLOR);
}

/*
 * Method that ensures that the GPS information is being fed
 */
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (Serial2.available())
      tinyGPS.encode(Serial2.read());
  } while (millis() - start < ms);
}

/*
 * Prints the time in HH:mm:SS format
 */
void printTime()
{
  String time = "";
  time = time + (tinyGPS.time.hour() + 2);
  time = time + ":";
  if (tinyGPS.time.minute() < 10) time = time + '0';
  time = time + tinyGPS.time.minute();
  time = time + ":";
  if (tinyGPS.time.second() < 10) time = time + '0';
  time = time + tinyGPS.time.second();
  tft.drawString(String(time), 350 , 150, 4);

}
