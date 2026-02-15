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

// Structure to store student info
struct Student {
  String rfidTag;
  String name;
  String roll;
  String course;
  String examRoom;
  bool attendance;
};

// Array to hold up to 10 students
Student students[10];
int studentCount = 0;  // Counter to track the number of students

bool servoOpened = false;         // Flag to check if the servo is opened
unsigned long servoOpenTime = 0;  // Stores the time when the servo was opened
unsigned long openDuration = 5000; // Servo stays open for 5 seconds

bool adminModeActive = false;  // Flag to indicate admin mode is active

void setup() {
  Serial.begin(115200);  // Initialize serial communications
  SPI.begin();           // Init SPI bus
  rfid.PCD_Init();       // Init MFRC522

  myServo.attach(SERVO_PIN);  // Attach servo to the defined pin
  myServo.write(0);           // Set initial servo position to 0 degrees (closed)

  Serial.println("Place your RFID card.");
}

void loop() {
  // Check for input from the Serial Monitor
  if (Serial.available()) {
    String input = Serial.readString();
    input.trim();

    if (input == "admin") {
      enterAdminMode();
    } else if (input == "attendance") {
      viewAttendance();
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

      // Check if the RFID belongs to a student and record attendance if not in admin mode
      if (!adminModeActive) {
        studentMode(rfidTag);
      } else {
        // In admin mode, check if the RFID is already registered
        bool rfidExists = false;
        for (int i = 0; i < studentCount; i++) {
          if (students[i].rfidTag == rfidTag) {
            Serial.println("RFID already registered.");
            return;  // Exit if RFID is already registered
          }
        }
        // Register new student in admin mode
        addNewStudent(rfidTag);  
        adminModeActive = false;  // Exit admin mode after registering the student
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

// Admin mode: Enter password and edit student data
void enterAdminMode() {
  Serial.println("Enter admin password:");
  while (Serial.available() == 0) {
    // Wait for password input
  }
  String inputPassword = Serial.readString();
  inputPassword.trim();

  if (inputPassword == adminPassword) {
    Serial.println("Place your RFID card");
    adminModeActive = true;  // Set admin mode flag
  } else {
    Serial.println("Invalid password. Access denied.");
    adminModeActive = false; // Reset admin mode flag
  }
}

// Function to add or edit student data
void addNewStudent(String rfidTag) {
  if (studentCount < 10) {
    Serial.println("Enter student name:");
    while (Serial.available() == 0) {
      // Wait for input
    }
    String name = Serial.readString();
    name.trim();

    Serial.println("Enter student ID:");
    while (Serial.available() == 0) {
      // Wait for input
    }
    String roll = Serial.readString();
    roll.trim();

    Serial.println("Enter course name:");
    while (Serial.available() == 0) {
      // Wait for input
    }
    String course = Serial.readString();
    course.trim();

    Serial.println("Enter exam room:");
    while (Serial.available() == 0) {
      // Wait for input
    }
    String examRoom = Serial.readString();
    examRoom.trim();

    // Store the new student data
    students[studentCount].rfidTag = rfidTag;
    students[studentCount].name = name;
    students[studentCount].roll = roll;
    students[studentCount].course = course;
    students[studentCount].examRoom = examRoom;
    students[studentCount].attendance = false;  // Mark attendance as false initially
    studentCount++;

    Serial.println("Student details added successfully.");
  } else {
    Serial.println("Student limit reached, cannot add more.");
  }
}

// Student mode: Check student attendance and open the gate
void studentMode(String rfidTag) {
  bool studentFound = false;
  for (int i = 0; i < studentCount; i++) {
    if (students[i].rfidTag == rfidTag) {
      Serial.print("Welcome, ");
      Serial.println(students[i].name);
      Serial.print("Course: ");
      Serial.println(students[i].course);
      Serial.print("Exam Room: ");
      Serial.println(students[i].examRoom);

      if (!students[i].attendance) {
        students[i].attendance = true;  // Mark attendance
      }

      openServo();  // Open the gate for the student
      studentFound = true;
      break;
    }
  }

  if (!studentFound) {
    Serial.println("Student not found.");
  }
}

// Function to open the servo (turn to 90 degrees)
void openServo() {
  if (!servoOpened) {
    Serial.println("Please Enter");
    myServo.write(120);   // Turn servo to 90 degrees
    servoOpenTime = millis();  // Record the time when the servo was opened
    servoOpened = true;   // Set the flag to indicate that the servo is opened
  }
}

// Function to close the servo (return to 0 degrees)
void closeServo() {
  myServo.write(0);   // Return servo to 0 degrees
  servoOpened = false;  // Reset the flag
}

// Function to view attendance list
void viewAttendance() {
  Serial.println("Enter admin password to view attendance:");
  while (Serial.available() == 0) {
    // Wait for password input
  }
  String inputPassword = Serial.readString();
  inputPassword.trim();

  if (inputPassword == adminPassword) {
    Serial.println("Attendance List:");
    for (int i = 0; i < studentCount; i++) {
      Serial.print("Name: ");
      Serial.print(students[i].name);
      Serial.print(", Roll: ");
      Serial.print(students[i].roll);
      Serial.print(", Attendance: ");
      Serial.println(students[i].attendance ? "Present" : "Absent");
    }
  } else {
    Serial.println("Invalid password. Access denied.");
  }
}
