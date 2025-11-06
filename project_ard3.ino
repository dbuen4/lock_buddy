#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include "secrets.h"
#include <SoftwareSerial.h>
char ssid[] = SECRET_SSID_HOME;  
char pass[] = SECRET_PASS_HOME;   
int status = WL_IDLE_STATUS;
char server[] = "bqgbsomhldpfsbgfkgsb.supabase.co";    

WiFiSSLClient client;
const int trigPin = 10;
const int echoPin = 9;
const int buzzer = 8;
long duration;
int distance;
bool gathering = false;
char data[64] = {0};
int index=0;
unsigned long timer = 0;
unsigned long sonicReportTimer = 0;

unsigned long sonicPinTimer = 0;

unsigned long touchTimer = 0;
int sonicStage = 0;
int numberAttempts = 0;

unsigned long chirpTimer = 0;
int numberChirp = 0;

SoftwareSerial mySerial(2,3);

void checkReconnect() {
  if (!client.connected()) {
  	Serial.print(".");
  	client.connect(server, 443);
	}
}
void updateLockState(bool state) {
	checkReconnect();
    
	char nm[8] = {0};
	if(state) {
  	strcpy(nm, "true");
	} else {
  	strcpy(nm, "false");
	}
	client.println("PATCH /rest/v1/lockstate?select=locked&id=eq.1 HTTP/1.1");
	client.println("Host: bqgbsomhldpfsbgfkgsb.supabase.co");
	client.print("apikey: ");
	client.println(SUPABASE_API_ANON);
	client.println("Content-Type: application/json");
	client.println("Connection: keep-alive");
	client.print("Content-Length: ");
	client.println((strlen(nm)+11));
	client.println();
	client.print("{\"locked\":");

	client.print(nm);
	client.println("}");
	client.println();
}



void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer,OUTPUT);
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial1.begin(9600);
  while (!Serial) {
	; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
	Serial.println("Communication with WiFi module failed!");
	// don't continue
	while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
	Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
int attemptConnect = 0;
  while (status != WL_CONNECTED) {
	attemptConnect += 1;
	if( attemptConnect > 3) {
		exit(0); // end after 3 failed attempts to connect
	}
	Serial.print("Attempting to connect to SSID: ");
	Serial.println(ssid);
	// Connect to WPA/WPA2 network.
	status = WiFi.begin(ssid, pass);

	// wait 10 seconds for connection:
	// NOTE: This usage of delay is for waiting for the WiFi connection to begin! This usage of delay will not have the program continue, allowed with https://piazza.com/class/m5vcnhr6sb74je/post/446 
	delay(10000);
  }

  printWifiStatus();
  client.connect(server, 443);
  timer = millis();
  sonicReportTimer = millis() + 60*1000;
  sonicPinTimer = millis();
  touchTimer = millis();
}
void reportEvent(String event) {
	client.println("POST /rest/v1/log HTTP/1.1");
	client.println("Host: bqgbsomhldpfsbgfkgsb.supabase.co");
	client.print("apikey: ");
	client.println(SUPABASE_API_ANON);
	client.println("Content-Type: application/json");
	client.print("Content-Length: ");
	client.print((15+event.length()));
	client.println();
	client.println();
	client.print("{\"eventID\": \"");
	client.print(event);
	client.print("\"");
	client.println("}");
	client.println();
}
void evaluateParams(char** a) {
 
   if(strcmp(*a, "touch") == 0) {
	if(millis() - touchTimer > 10 * 1000) {
  	reportEvent("touch");
  	touchTimer = millis();
	}
  } else if(strcmp(*a, "fail") == 0) {
	Serial.println("ERR TO GET IN");
	numberChirp=3;
	chirpTimer = millis();
	numberAttempts+=1;
	reportEvent("fail");
  } else if(strcmp(*a, "alarm") == 0) {
	digitalWrite(buzzer, HIGH);
  } else if(strcmp(*a, "silence") == 0) {
	Serial.println("silenced..");
	digitalWrite(buzzer, LOW);
  } else {
	Serial.println("ITS OPEN!");
	numberChirp=1;
	chirpTimer = millis();
	updateLockState(false);
	Serial1.println(*a);
	numberAttempts = 0;
	reportEvent(*a);
  }
}


void loop() {
  //read_response();
  if (Serial1.available()) {
	char b = Serial1.read();
	Serial.println(b);
	numberAttempts = 0;
	digitalWrite(buzzer, LOW);
  }
  if(numberAttempts >= 5) {
	digitalWrite(buzzer, HIGH);
  }
  if(millis() - timer > 5000) {
	client.println("GET /rest/v1/lockstate?select=locked&id=eq.1 HTTP/1.1");
	client.println("Host: bqgbsomhldpfsbgfkgsb.supabase.co");
	client.print("apikey: ");
	client.println(SUPABASE_API_ANON);
	client.println();
	timer = millis();
  }
  if(numberChirp >= 0) {
	if(millis() - chirpTimer < 50) {
  	digitalWrite(buzzer, HIGH);
	} else if(millis() - chirpTimer > 100) {
  	//Serial.println("decr");
  	chirpTimer = millis();
  	numberChirp--;
	} else if(millis() - chirpTimer > 50) {
  	digitalWrite(buzzer, LOW);
	}
  }

  if(sonicStage == 0) {
	digitalWrite(trigPin, LOW);
	sonicStage = 1;
	sonicPinTimer = millis();
  } else if(millis() - sonicPinTimer > 2 && sonicStage == 1) {
	digitalWrite(trigPin, HIGH);
	sonicStage = 2;
	sonicPinTimer = millis();
  } else if(millis() - sonicPinTimer > 10 && sonicStage == 2) {
	digitalWrite(trigPin, HIGH);
	sonicStage = 0;
	sonicPinTimer = millis();
	// Sets the trigPin on HIGH state for 10 micro seconds

	digitalWrite(trigPin, LOW);
	// Reads the echoPin, returns the sound wave travel time in microseconds
	duration = pulseIn(echoPin, HIGH);
	//  Calculating the distance
	distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
	Serial.println(distance);
	if(distance < 50 && millis()-sonicReportTimer > 60 * 1000) {
    	reportEvent("moti");
    	sonicReportTimer = millis();
	}

  	int i;
  	if(mySerial.available()) {
    	char ch=mySerial.read();
    	Serial.print(ch);
    	if(ch=='{'){
      	data[0]=index=0;
      	if(gathering == false) {
        	gathering = true;
      	}
    	} else if(ch != '}' && gathering == true) {
      	data[index++]=ch; data[index]=0;
    	} else if(gathering==true){
      	gathering = false;
      	data[index++]='\0';
      	char* g = data;
      	evaluateParams(&g);
      	data[0]=index=0;
    	}
  	}
  }
}
/* just wrap the received data up to 80 columns in the serial print*/
/* -------------------------------------------------------------------------- */
void read_response() {
/* -------------------------------------------------------------------------- */
  uint32_t received_data_num = 0;
  while (client.available()) {
	/* actual data reception */
	char c = client.read();
	/* print data to serial port */
	Serial.print(c);
    
  }
 
}


/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
