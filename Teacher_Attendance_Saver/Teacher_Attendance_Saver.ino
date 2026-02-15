#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <EEPROM.h>  // Include EEPROM library

#define RST_PIN 5    // RST pin for NodeMCU (D1 = GPIO 5)
#define SS_PIN 4     // SS pin for NodeMCU (D2 = GPIO 4)
#define SERVO_PIN 2  // GPIO pin for Servo (D4)
#define EEPROM_SIZE 64 // Define the EEPROM size

MFRC522 rfid(SS_PIN, RST_PIN);  // Create MFRC522 instance
Servo myServo;                  // Create Servo object

// Predefined admin password
String adminPassword = "1234";  // Admin password

String registeredRFID = "";  // Variable to store the single allowed RFID tag

bool servoOpened = false;         // Flag to check if the servo is opened
unsigned long servoOpenTime = 0;  // Stores the time when the servo was opened
unsigned long openDuration = 5000; // Servo stays open for 5 seconds

void setup() {
  Serial.begin(115200);  // Initialize serial communications
  SPI.begin();           // Init SPI bus
  rfid.PCD_Init();       // Init MFRC522
  EEPROM.begin(EEPROM_SIZE);  // Initialize EEPROM
  
  myServo.attach(SERVO_PIN);  // Attach servo to the defined pin
  myServo.write(0);           // Set initial servo position to 0 degrees (closed)

  // Load stored RFID from EEPROM
  loadRFIDFromEEPROM();
  
  if (registeredRFID != "") {
    Serial.println("RFID tag loaded from EEPROM.");
  } else {
    Serial.println("No RFID tag registered. Enter 'admin' to register a new RFID.");
  }
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
    Serial.println("Password accepted. Admin mode activated.");

    Serial.println("Place the RFID card to register.");

    // Wait for a new RFID card scan
    while (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
      // Loop until a card is detected
    }

    // Read the RFID tag
    String rfidTag = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      rfidTag += String(rfid.uid.uidByte[i], HEX);  // Convert each byte to hex string
    }

    // Store the RFID tag as the authorized one
    registeredRFID = rfidTag;
    Serial.println("RFID tag registered successfully.");

    // Save the RFID tag to EEPROM
    saveRFIDToEEPROM();

    // Halt PICC to stop reading
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  } else {
    Serial.println("Invalid password. Access denied.");
  }
}

// Function to open the servo (turn to 90 degrees)
void openServo() {
  if (!servoOpened) {
    Serial.println("Gate opening");
    myServo.write(90);   // Turn servo to 90 degrees (open position)
    servoOpenTime = millis();  // Record the time when the servo was opened
    servoOpened = true;   // Set the flag to indicate that the servo is opened
  }
}

// Function to close the servo (return to 0 degrees)
void closeServo() {
  Serial.println("Gate closing (Servo to 0 degrees)...");
  myServo.write(0);   // Return servo to 0 degrees (closed position)
  servoOpened = false;  // Reset the flag
}

// Save the registered RFID tag to EEPROM
void saveRFIDToEEPROM() {
  Serial.println("Saving RFID to EEPROM...");
  for (int i = 0; i < registeredRFID.length(); i++) {
    EEPROM.write(i, registeredRFID[i]);
  }
  EEPROM.write(registeredRFID.length(), '\0');  // Add a null character to indicate end of string
  EEPROM.commit();  // Commit the changes to EEPROM
  Serial.println("RFID saved.");
}

// Load the registered RFID tag from EEPROM
void loadRFIDFromEEPROM() {
  Serial.println("Loading RFID from EEPROM...");
  char rfidTag[64];
  for (int i = 0; i < EEPROM_SIZE; i++) {
    char ch = EEPROM.read(i);
    if (ch == '\0') {
      break;
    }
    rfidTag[i] = ch;
  }
  registeredRFID = String(rfidTag);
}
