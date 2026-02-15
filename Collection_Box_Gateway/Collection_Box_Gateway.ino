#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define RST_PIN 5    // RST pin for NodeMCU (D1 = GPIO 5)
#define SS_PIN 4     // SS pin for NodeMCU (D2 = GPIO 4)
#define SERVO_PIN 2  // GPIO pin for Servo (D4)

MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance
Servo myServo;                  // Create Servo object

// Predefined admin password
String adminPassword = "1234";  // Admin password

// Variable to store the single allowed RFID tag
String registeredRFID = "";  // RFID tag will be stored here

bool servoOpened = false;         // Flag to check if the servo is opened
unsigned long servoOpenTime = 0;  // Stores the time when the servo was opened
unsigned long openDuration = 5000; // Servo stays open for 5 seconds

void setup() {
  Serial.begin(115200);  // Initialize serial communications
  SPI.begin();           // Init SPI bus
  rfid.PCD_Init();       // Init MFRC522

  myServo.attach(SERVO_PIN);  
  myServo.write(0);           

  Serial.println("Place your RFID card");
}

void loop() {
  // Check for input from the Serial Monitor
  if (Serial.available()) {
    String input = Serial.readString();
    input.trim();

    if (input == "admin") {
      enterAdminMode();
    }
  }

  // Check if a new card is present
  if (rfid.PICC_IsNewCardPresent()) {
    // Select one of the cards
    if (rfid.PICC_ReadCardSerial()) {
      // Read the RFID tag
      String rfidTag = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        rfidTag += String(rfid.uid.uidByte[i], HEX);  // Convert each byte to hex string
      }

      // Check if the RFID tag matches the registered RFID
      if (rfidTag == registeredRFID) {
        openServo();  // Open the door (servo) if RFID matches
      } else {
        Serial.println("Unauthorized RFID.");
      }

      // Halt PICC to stop reading
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  // Check if the servo has been opened and if enough time has passed to close it
  if (servoOpened && millis() - servoOpenTime >= openDuration) {
    closeServo();  // Close the servo (return to 0 degrees)
  }
}

// Admin mode: Enter password and register a single RFID tag
void enterAdminMode() {
  Serial.println("Enter admin password:");
  while (Serial.available() == 0) {
    // Wait for password input
  }
  String inputPassword = Serial.readString();
  inputPassword.trim();

  if (inputPassword == adminPassword) {
    if (registeredRFID == "") {
      Serial.println("Admin mode activated.");
      Serial.println("Place the RFID card to register.");

      // Wait for a new RFID card scan
      while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        // Loop until a card is detected
      }

      // Read the RFID tag
      String rfidTag = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        rfidTag += String(rfid.uid.uidByte[i], HEX); 
      }

      // Store the RFID tag as the authorized one
      registeredRFID = rfidTag;
      Serial.println("RFID tag registered successfully.");

      // Halt PICC to stop reading
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    } else {
      Serial.println("Only one RFID can be registered.");
    }
  } else {
    Serial.println("Invalid password. Access denied.");
  }
}

// Function to open the servo (turn to 90 degrees)
void openServo() {
  if (!servoOpened) {
    Serial.println("Gate opening");
    myServo.write(120);   // Turn servo to 90 degrees (open position)
    servoOpenTime = millis();  // Record the time when the servo was opened
    servoOpened = true;   // Set the flag to indicate that the servo is opened
  }
}

// Function to close the servo (return to 0 degrees)
void closeServo() {
  myServo.write(0);   // Return servo to 0 degrees (closed position)
  servoOpened = false;  // Reset the flag
}
