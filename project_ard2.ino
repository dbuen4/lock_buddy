// Team Number: 18.00
// Dale Buencillo (NetID: dbuen4), Daniel Dedic (NetID: ddedi2), Khi Kidman (NetID: mkima2)
// Project Name: Lock Buddy
// Abstract: 	Lock Buddy is a door security system with 2 forms of authentication. 
// An Arudino is used to take input, authenticate, and detect a user. 
// Another opens the locking mechanism of the door, while a 3rd sends information to the owner's device. 
// The originality of the product revolves around the door handle, which permits the users multiple ways to unlock the door while giving the owner a reliable method to know who has interacted with the door.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// define pins
#define SERVO_PIN 9
#define GREEN_LED 10
#define RED_LED 11
#define CLOSE_BUTTON_PIN 4


// initialize components
Servo doorLock;
LiquidCrystal_I2C lcd(0x27, 16, 2);



// state tracking
bool isUnlocked = false;
String receivedData = "";


// debounce variables
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;


void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(CLOSE_BUTTON_PIN, INPUT_PULLUP);


  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);


  doorLock.attach(SERVO_PIN);
  doorLock.write(0);


  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Door Locked   ");
  lcd.setCursor(0, 1);
  lcd.print("               ");


  Serial1.begin(9600);
}


void loop() {
  unsigned long currentTime = millis();

  // check button press to close door
  if (digitalRead(CLOSE_BUTTON_PIN) == LOW && isUnlocked) {
    if (currentTime - lastButtonPress > debounceDelay) {
      lockDoor();
      lastButtonPress = currentTime;
    }
  }

  // check for button press to turn off alarm
  else if (digitalRead(CLOSE_BUTTON_PIN) == LOW) {
    if (currentTime - lastButtonPress > debounceDelay) {
      Serial1.print("F");
      lastButtonPress = currentTime;
    }
  }

  // Check for message from Arduino 1 to unlock the door
  if (Serial1.available()) {
    receivedData = Serial1.readStringUntil('\n');
    receivedData.trim();

    Serial.println(receivedData);

    if (receivedData.length() > 0 && !isUnlocked) {
      unlockDoor(receivedData);
    }
  }
}


//unlock function. displays user name based on message recieved
void unlockDoor(String message) {
  doorLock.write(90);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  lcd.setCursor(0, 0);
  lcd.print("Door Unlocked ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Welcome " + message);
  isUnlocked = true;

  Serial.println("Door unlocked");
}


//lock function, resets after button is pressed
void lockDoor() {
  doorLock.write(0);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Door Locked   ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  isUnlocked = false;

  Serial.println("Door locked");
}

