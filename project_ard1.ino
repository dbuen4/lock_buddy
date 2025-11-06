#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

const uint8_t RST_PIN = 9;
const uint8_t SDA_PIN = 10;

// SoftwareSerial arduino2(2, 3); // removed due to UNO R4 limitations
SoftwareSerial arduino3(4, 5);


MFRC522 rfidReader(SDA_PIN, RST_PIN);

#define mySerial Serial1

Adafruit_Fingerprint fingerprintScanner = Adafruit_Fingerprint(&mySerial);

int capacativeTouchSendPin = 6;  // Pin used to charge the sensor
int capacativeTouchSensePin = 7; // Pin used to read capacitance
long baselineCapacitance = 0; // Stores the baseline capacitance value

int ledPin = 8;

unsigned long previousTouchMillis = 0;
unsigned long capacativeTouchDetectDelayMillis = 3000;

unsigned long previousRFIDReadMillis = 0;
unsigned long RFIDReadDelayMillis = 2000;

unsigned long previousFingerprintAttemptMillis = 0;
unsigned long fingerprintAttemptDelay = 3000;

String uidString;
int key[] = {131,161,38,20};
int keyRead = 0;

struct User {
  char name[8];
  int key[4];
  uint8_t id;
};

struct User admin = {"Admin", {131,161,38,20}, 1};
struct User user69 = {"User 69", {0, 0, 0, 0}, 69};
struct User dan = {"Dan", {4, 87, 26, 194}, 42};

struct User users[] = {admin, user69, dan};

bool readRFID() {
    rfidReader.PICC_ReadCardSerial();
    //Serial.println("Scanned PICC's UID:"); // debug purposes
    //printDec(rfidReader.uid.uidByte, rfidReader.uid.size); // debug purposes

    uidString = String(rfidReader.uid.uidByte[0])+" "+String(rfidReader.uid.uidByte[1])+" "+String(rfidReader.uid.uidByte[2])+" "+String(rfidReader.uid.uidByte[3]);

    //Serial.print("UID: "); // debug purposes
    //Serial.print(uidString); // debug purposes

    int i = 0;
    bool match = true;
    while (i < rfidReader.uid.size) {
        if (rfidReader.uid.uidByte[i] != key[i]) {
            match = false;
        }
        i++;
    }

    // Debug printing
    // if (match) {
    //     Serial.println("Key recognized");
    //     return true;
    // } else {
    //     Serial.println("Unknown key");
    //     return false;
    // }
}

void printDec(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}

void calibrateCapacitiveTouch() {
    digitalWrite(capacativeTouchSendPin, LOW);

    // Calibration Step
    Serial.println("Calibrating... Do not touch the doorknob.");
    delay(2000); // Wait 2 seconds to ensure it's untouched

    long total = 0;
    const int samples = 20; // Number of calibration samples

    for (int i = 0; i < samples; i++) {
        total += measureCapacitance();
        delay(50); // Short delay between measurements
    }

    baselineCapacitance = total / samples; // Calculate baseline average
    Serial.print("Calibration complete. Baseline: ");
    Serial.println(baselineCapacitance);
}

bool detectCapacativeTouch() {
    long capValue = measureCapacitance();
    long change = capValue - baselineCapacitance; // Detect change from baseline

    // Debug printing
    // if (change > 1) { // Adjust threshold as needed
    //     Serial.println("Touch detected!");
    // }
    //digitalWrite(ledPin, change > 3);
    return change > 2;
}

// Function to measure capacitance
long measureCapacitance() {
    pinMode(capacativeTouchSensePin, INPUT);
    digitalWrite(capacativeTouchSendPin, HIGH);
    delayMicroseconds(10);

    pinMode(capacativeTouchSensePin, OUTPUT);
    digitalWrite(capacativeTouchSensePin, LOW);

    pinMode(capacativeTouchSensePin, INPUT);
    return pulseIn(capacativeTouchSensePin, HIGH, 5000); // Measure charge decay time
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintID() {
  uint8_t p = fingerprintScanner.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = fingerprintScanner.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = fingerprintScanner.fingerFastSearch();
  if (p == FINGERPRINT_NOTFOUND) {
    return 0;
  } else if (p != FINGERPRINT_OK) {
    return -1;
  }
  // found a match!
  return fingerprintScanner.fingerID;
}

void setup() {
    // arduino2.begin(9600); // removed due to UNO R4 limitations
    arduino3.begin(9600);
    Serial.begin(9600);
    pinMode(capacativeTouchSendPin, OUTPUT);
    pinMode(capacativeTouchSensePin, INPUT);
    pinMode(ledPin, OUTPUT);

    SPI.begin();
    rfidReader.PCD_Init();

    fingerprintScanner.begin(57600);
    delay(5);
    if (fingerprintScanner.verifyPassword()) {
      Serial.println("Found fingerprint sensor!");
    } else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1) { delay(1); }
    }

    calibrateCapacitiveTouch();
}

void loop() {

    unsigned long currentMillis = millis();

    if (detectCapacativeTouch()) {
      if (currentMillis - previousTouchMillis > capacativeTouchDetectDelayMillis) {
        arduino3.print("{touch}");
      }
      digitalWrite(ledPin, HIGH);
      previousTouchMillis = currentMillis;
    }

    if (currentMillis - previousTouchMillis > capacativeTouchDetectDelayMillis) {
      digitalWrite(ledPin, LOW);
    }

    if (rfidReader.PICC_IsNewCardPresent()) {
      if (currentMillis - previousRFIDReadMillis > RFIDReadDelayMillis) {
        readRFID();
        Serial.println(uidString);
        bool validUserFound = false;
        for (User user : users) {
          bool isValid = true;
          for (int i = 0; i < 4; i++) {
            if (user.key[i] != rfidReader.uid.uidByte[i]) {
              isValid = false;
            }
          }
          if (isValid) {
            validUserFound = true;
            arduino3.print("{"); arduino3.print(user.name); arduino3.print("}");
            break;
          }
        }
        if (!validUserFound) {
          arduino3.print("{fail}");
        }
        previousRFIDReadMillis = currentMillis;
      }

    }

    if (currentMillis - previousFingerprintAttemptMillis > fingerprintAttemptDelay) {
      int fingerprintID = getFingerprintID();
      switch (fingerprintID) {
        case 0:
          Serial.println("Unauthorized fingerprint access");
          arduino3.print("{fail}");
          previousFingerprintAttemptMillis = currentMillis;
          break;
        case -1:
          break;
        default:
          Serial.print("Fingerprint access: ID #"); Serial.println(fingerprintID);
          for (User user : users) {
            if (user.id == fingerprintID) {
              arduino3.print("{"); arduino3.print(user.name); arduino3.print("}");
              // arduino2.print(user.name); // removed due to UNO R4 limitations
              Serial.print("Welcome "); Serial.println(user.name);
            }
          }
          previousFingerprintAttemptMillis = currentMillis;
          break;
      }     
    }

    // if (currentMillis - previousRFIDReadMillis > RFIDReadDelayMillis) {
    //   if (rfidReader.PICC_IsNewCardPresent()) {
    //       if (readRFID()) {
    //           digitalWrite(ledPin, LOW);
    //       }
    //   }
    //   previousRFIDReadMillis = currentMillis;
    // }
}
