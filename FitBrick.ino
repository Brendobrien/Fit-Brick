/*
 * 
 * Brendan O'Brien & Drew Freyberger
 * BME354L: Commented Arduino Code
 * 
 */

// Each portion is broken into two three main categories based on hardware
// A. Accelerometer; B. LCD Shield; C. Memory (SD Card/EEPROM)

/*

I.  Libraries
 
*/

  // Import the necessary libraries

  // Wire Library - the fundamental Arduino library
  #include <Wire.h>

  // A. Accelerometer
    #include <Adafruit_MMA8451.h>
    #include <Adafruit_Sensor.h>

  // B. LCD Shield
    #include <utility/Adafruit_MCP23017.h>
    #include <Adafruit_RGBLCDShield.h>

  // C. Memory (SD Card/EEPROM)
    #include <SD.h>
    #include <SPI.h>
    #include <EEPROM.h>


/*

II. Global Variables

*/

  // A. Accelerometer
    // a. Define the accelerometer
     Adafruit_MMA8451 mma = Adafruit_MMA8451();

    // b. Define the event of the accelerometer
    // This contains relevant info like the acceleration vectors
     sensors_event_t event;

    // c. Accelartion Vectors
    // i. Footfalls
    // These values are updated every 250 ms
    // From an average of vectors every 10 ms
    // Their comparison determines if a footfall occurs
    float footfallAcclCurr;
    float footfallAcclPrev;

    // Constants for each two footfalls
    // This is based on test data for the acceleration of the hand
  float deltaStep = 0.4;

    // Footfalls are global variables to be printed and incremented
    long LifeFalls;
    int CurrFalls = 0;

    // ii. Activity Level
    // This value is updated every 500 ms
    // From an average of vectors every 10ms
    float AccelTotal;

    // Constants for each activity level of acceleration
    // Low = AccelTotal < lowAct
    // Moderate = lowAct <= AccelTotal <= highAct
    // Intense = AccelTotal > highAct
  float lowAct =  1.15;
  float highAct = 1.4;

    // Activity State variable
    // 1 = Low
    // 2 = Moderate
    // 3 = Intense
    // Initialize at Low
     int currActivity = 1;
     int prevActivity = 1;

  // B. LCD Shield
    // a. Define the lcd shield for displaying info
    Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

    // b. Define the Arduino Buttons as a boolean
  uint8_t buttons;

    // c. Determine the LCD's current state for live updating
    // Initialize at 0
  int LCDState = 0;

  // C. Memory (SD Card/EEPROM)
  const int chipSelect = 4;
  int addressEEPROM = 0;


/*

III. Setup

*/

 void setup(void) {
  // Debugging output
  Serial.begin(9600);

  // A. Accelerometer
    // Debugging Accelerometer
    Serial.println("Adafruit MMA8451 test!");

    if (! mma.begin()) {
      Serial.println("Couldnt start");
      while (1);
    }
    Serial.println("MMA8451 found!");

    mma.setRange(MMA8451_RANGE_2_G);

    Serial.print("Range = "); Serial.print(2 << mma.getRange());  
    Serial.println("G");

  // B. LCD Shield
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
  
  // C. Memory (SD Card/EEPROM)
    // Debugging SD Card
    Serial.print("Initializing SD card...");

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      return;
    }
    Serial.println("card initialized.");

    // make a string for assembling the data to log:
    // This is the initial value
    String dataString = "";

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("times.txt", FILE_WRITE);

    // Write to file
    dataString += String(millis()); dataString += ",";
    dataString += "Low";

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
      // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }

    // Load footfalls from Memory
    EEPROM.get(addressEEPROM,LifeFalls);
  }

/*

IV.  Loop
 
*/

  void loop() {
    // It's as easy as 1-2-3
    // A
    // Check the Accelerometer; this process takes 1s
    accelerometer();

    // B
    // Check the Buttons; This is quick
    lcdShield();

    // C
    // Save to the SD Card; This is quick
    sdCard();
  }

  // A. Accelerometer
    void accelerometer(){
      // Set the initial values for values used in 500ms loop
      // i. Footfall
      // Number of samples (25) * Sampling rate (10ms) = 250ms acceleration vector average
      int numFoot = 25;

      // Sum that holds 25 samples for each accelartion vectors
      // Added to iteratively
      float footfallAcclTotal = 0;
      
      // ii. Activity Level
      // Number of samples (50) * Sampling rate (10ms) = 500ms acceleration vector average
      int numAct = 50;

      // Sum that holds 50 samples for each accelartion vectors
      // Added to iteratively
      float activityAcclTotal = 0;

      // Set the previous as the initial value
      // We implemented the square root of the squares of the acceleration in each direction
      footfallAcclPrev = sqrt((event.acceleration.x*event.acceleration.x) + (event.acceleration.y*event.acceleration.y) + (event.acceleration.z*event.acceleration.z));;

      // Loop: 10 ms*50 = 500ms
      for(int i = 0; i < numAct; i++){
        // Read the 'raw' data in 14-bit counts
        mma.read();
        mma.getEvent(&event);
        
        // Calculate the current acceleration vector
        // We implemented the square root of the squares of the acceleration in each direction
        float currAccel = sqrt((event.acceleration.x*event.acceleration.x) + (event.acceleration.y*event.acceleration.y) + (event.acceleration.z*event.acceleration.z));
        activityAcclTotal = activityAcclTotal + currAccel;
        footfallAcclTotal = footfallAcclTotal + currAccel;

        // Update Footfalls
        // This will be 0 every 25 iterations (250ms)
        int footfallCounter = (i+1) % numFoot;
        
        if(footfallCounter == 0){
          // Calculate the average accelartion vector
          footfallAcclCurr = footfallAcclTotal/numFoot;

          // Check if this vectors represents a step
          footfallCheck();

          // Reset the partial sum
          footfallAcclTotal = 0;
        }
        
        // Delay 10ms
        // 50*10ms = 500 ms Total
        // This was found to be an optimal waiting period
        delay(10);
      }
      // Update Acceleraation for Activity Level
      AccelTotal = activityAcclTotal/numAct;

      // Update the LCD every 500ms   
      // 2. Right = Live Accelartions
      if (LCDState == 2) {
        displayLCDLive();
      }

      // 3. Up = Activity Level
      if (LCDState == 3) {
        displayLCDActivity();
      }

      // 4. Left = Footfalls
      if (LCDState == 4) {
        displayLCDFootfalls();
      }

      // Check the activity
      activityCheck();
    }

  // B. LCD Shield
    void lcdShield(){
      // Read the Arduino Buttons as a boolean
      buttons = lcd.readButtons();

      // Start-up Screen
      if (LCDState == 0) {
        startUp();
      }

      // Check to see if any buttons changed
      // If so, change LCDState to refect the new state
      if (buttons) {
        // If you are in the settings pages, use the settings buttons
        if (LCDState == 1){
          settingsScreenButttons();    
        }
        // IOtherwise, use the normal buttons
        else {
          mainScreenButtons();
        }
      }
    }

    // These are the responses to button presses in the main screen
    void mainScreenButtons(){

      // Each of these determines the new display of the Arduino LCD Shield
      // 1.  Select = Settings page
      if (buttons & BUTTON_SELECT) {
        lcd.clear();
        settings();
        LCDState = 1;
      }

      // 2. Right = Live Accelartions
      if (buttons & BUTTON_RIGHT) {
        lcd.clear();  
        displayLCDLive();
        LCDState = 2;
      }

      // 3. Up = Activity Level
      if (buttons & BUTTON_UP) {
        lcd.clear();
        displayLCDActivity();
        LCDState = 3;
      }

      // 4. Left = Footfalls
      if (buttons & BUTTON_LEFT) {
        lcd.clear();
        displayLCDFootfalls();
        LCDState = 4;
      }
    }

  // C. SD Card
    void sdCard(){
      // If the acitivity level has changed
      if(currActivity != prevActivity){
      // SD Card Setup
      //
      // make a string for assembling the data to log:
      String dataString = "";

      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File dataFile = SD.open("times.txt", FILE_WRITE);

      // Write the time to the file since the arduino has been on
      // Write the new Activity State
      if(currActivity == 1){
        dataString += String(millis()); dataString += ",";
        dataString += "Low";
      }
      
      if(currActivity == 2){
        dataString += String(millis()); dataString += ",";
        dataString += "Moderate";
      }
      
      if(currActivity == 3){
        dataString += String(millis()); dataString += ",";
        dataString += "High";
      }

      // if the file is available, write to it:
      // Print to the Serial to show this
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
      // print to the serial port too:
        Serial.println(dataString);
      }
        
        // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening datalog.txt");
      }
      }
    } 

/*

V. Baseline software functionality
 
*/
  // This portion is broken into seven parts based on the seven bullet points
  // found in the lab manual.
  //
  // 1.
  // Monitor the accelerometer and determine when footfalls occur, regardless
  // of  the  orientation  of your device.
  // a. Compare the current acceleration vector with the previous one
  void footfallCheck(){
    // Determine if the difference between the accelerations of the current step
    // and the previous is larger than deltaStep
    // deltaStep => determined by test data to quantify a step
    //
    // Also, if the previous footfalll Acceleration is zero (which has yet to occur in vitro)
    // then don't increment since this is the first value
    if(abs(footfallAcclCurr - footfallAcclPrev) >  deltaStep && footfallAcclPrev != 0){
      incrementFootfalls();
    }
    footfallAcclPrev = footfallAcclCurr;
  }

  // b. Increment Footfalls
  void incrementFootfalls(){
    // Add one to the previous number
    CurrFalls = CurrFalls + 1;
    LifeFalls = LifeFalls + 1;

  }

  // 2.
  // LCD  display  shows  Total  lifetime  footfalls  (like a  car's  odometer).
  // This  value  must  survive  a shutdown
  // 3.
  // LCD display shows a resettable number of footfalls (like a car's trip odometer).
  // This need not survive a shutdown.
  // a. Dynamically update display
  void displayLCDFootfalls(){
    // Top line of Display
    lcd.setCursor(0,0);
    lcd.print("Lifetime: ");
    lcd.print(LifeFalls);

    // Bottom line of Display
    lcd.setCursor(0,1);
    lcd.print("Current:  ");
    lcd.print(CurrFalls);
  }

  // 4.
  // LCD display shows current level of activity, "low," "moderate," or "intense."
  // a. Update Display
  void displayLCDActivity(){
    // Top line of Display
    lcd.setCursor(0,0);
    lcd.print("Activity Level: ");

    // Bottom line of Display
    lcd.setCursor(0,1);
  }

  // b. Compare the bounds
  void activityCheck(){
    if(AccelTotal < lowAct){
      prevActivity = currActivity;
      currActivity = 1;
      // If in Activity LCD state, update the lcd
      if(LCDState == 3){
        lcd.print("Low     ");
      }
    }
    else if(AccelTotal > highAct){
      prevActivity = currActivity;
      currActivity = 3;
      if(LCDState == 3){
        lcd.print("Intense ");
      }
    }
    else{
      prevActivity = currActivity;
      currActivity = 2;
      if(LCDState == 3){
        lcd.print("Moderate");
      }
    }
  }

  // 5.
  // Every time the level of activity changes, record the millis() time to a file
  // on the SD card along with  the  new  level  of  activity.   The  file  format
  // should  be  readable  in  Excel  for later analysis.

  // see sdCard() above

  // 6.
  // LCD can be put into a mode that shows moment-to-moment  acceleration  
  // on  all  three  axes,  updated at least once per second.
  void displayLCDLive(){

    // Top Line
    lcd.setCursor(0,0);
    lcd.print("a(m/s^2)"); lcd.print(" ");
    lcd.print("X:"); lcd.print(event.acceleration.x);
    
    // Bottom Line
    lcd.setCursor(0,1);
    lcd.print("Y:"); lcd.print(event.acceleration.y); lcd.print(" ");lcd.print(" ");lcd.print(" ");
    lcd.print("Z:"); lcd.print(event.acceleration.z);

  }

  // 7.
  // Code is formatted and commented appropriately.
  // You betcha ;)

/*

 VI. Extras

*/

  // This is the screen you see first
  void startUp(){
    // Top line of Display
    lcd.setCursor(0,0);
    lcd.print("Welcome to");

    // Bottom line of Display
    lcd.setCursor(0,1);
    lcd.print("FitBrick!");
  }

  // Select = Settings page
  //
  // Here one can
  // a. reset the current footfall count
  // b. save the lifetime footfall count (prepare for shutdown)
  void settings(){
    // Top line of Display
    lcd.setCursor(0,0);
    lcd.print("Left: Store");

    // Bottom line of Display
    lcd.setCursor(0,1);
    lcd.print("Right: Reset");
  }

  // These are the responses to button presses in the settings screen
  // The settings screen always returns to the footfalls screen by default
  void settingsScreenButttons(){

    // 1. Select = Exit
    if (buttons & BUTTON_SELECT) {
      lcd.clear();
      displayLCDFootfalls();
      LCDState = 4;
    }

    // 2. Right = Reset the Current Footfalls
    if (buttons & BUTTON_RIGHT) {
      lcd.clear();
      CurrFalls = 0;
      displayLCDFootfalls();
      LCDState = 4;
    }

    // 3. Left = Store the Lifetime Footfalls
    if (buttons & BUTTON_LEFT) {
      lcd.clear();  
      // Store Lifetime Footfalls in memory
      EEPROM.put(addressEEPROM, LifeFalls);

      // Store Activity Level to SD Card()
      // SD Card Setup
      //
      // make a string for assembling the data to log:
      String dataString = "";

      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File dataFile = SD.open("times.txt", FILE_WRITE);

      // Write the time to the file since the arduino has been on
      // Write the new Activity State
      if(currActivity == 1){
        dataString += String(millis()); dataString += ",";
        dataString += "Low";
      }
      
      if(currActivity == 2){
        dataString += String(millis()); dataString += ",";
        dataString += "Moderate";
      }
      
      if(currActivity == 3){
        dataString += String(millis()); dataString += ",";
        dataString += "High";
      }

      // if the file is available, write to it:
      // Print to the Serial to show this
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
      // print to the serial port too:
        Serial.println(dataString);
      }
        
        // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening datalog.txt");
      }
      displayLCDFootfalls();
      LCDState = 4;
    }
  }
