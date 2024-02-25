#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define FIREBASE_HOST "arduino-test-324dd-default-rtdb.firebaseio.com" // Replace with your Firebase project URL
#define FIREBASE_AUTH "AIzaSyC_WxgQSBu_FoU9s6xnHI2fpIBMcbm4NGA" // Replace with your Firebase authentication token
#define WIFI_SSID "Techgik"
#define WIFI_PASSWORD "powerfulmercy@2024"

#define password_length 5 // Maximum password length is 4 characters + NULL char

LiquidCrystal_I2C lcd(0x3F, 16, 2); // Adjust the address according to your LCD module

char Data[password_length]; // 4 is the number of chars it can hold + the null char = 5
char UserPassword[password_length]; // To store user-set password
byte data_count = 0;
char customKey;

bool chargingSystemEnabled = false;

int relay = 14;

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};
byte rowPins[ROWS] = {19, 18, 5, 17}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {16, 4, 0};      // connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS); // initialize an instance of class Keypad

FirebaseData fbdo;

void setup() {
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  lcd.init();        // Initialize the LCD
  lcd.backlight();   // Turn on the backlight
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome"); // Display welcome message
  lcd.setCursor(0, 1);
  lcd.print("Enter Password");

  // Sign up if not already signed up
  if (!Firebase.signUp(&fbdo)) {
    Serial.println("Sign up failed.");
    Serial.println(fbdo.errorReason());
  } else {
    Serial.println("Sign up successful.");
  }
}

void loop() {
  if (!chargingSystemEnabled) {
    customKey = customKeypad.getKey();

    if (customKey == '#') {
      if (data_count > 0) {
        Data[data_count] = '\0'; // Null terminate the entered password
        strncpy(UserPassword, Data, password_length); // Copy the entered password to UserPassword
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Password Set!");
        Serial.println("Password Set!");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Welcome");
        lcd.setCursor(0, 1);
        lcd.print("Enter Password");
        clearData();
      }
    } else if (customKey != NO_KEY) { // If a key is pressed (excluding NO_KEY)
      lcd.setCursor(data_count, 1);
      lcd.print('*');
      Data[data_count] = customKey;
      data_count++;
      if (data_count == password_length - 1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Checking Password");
        Serial.println("Checking Password");

        delay(1000);

        if (!strcmp(Data, UserPassword)) {
          lcd.clear();
          lcd.print(" ACCESS GRANTED");
          Serial.println("ACCESS GRANTED");
          delay(1000);
          chargingSystemEnabled = true;
        } else {
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("ACCESS DENIED!");
          Serial.println("ACCESS DENIED!");
          delay(1500);
          clearData();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Welcome");
          lcd.setCursor(0, 1);
          lcd.print("Enter Password");
        }
      }
    }
  }

  if (chargingSystemEnabled) {
    digitalWrite(relay, HIGH);
    Serial.println("Charging System Enabled");

    // Read analog input (assuming a 12-bit ADC)
    float voltage = (float)analogRead(A1) / 4096 * 24 * 47000 / 92931;

    // Convert voltage to percentage
    float percentage = (voltage / 12.6) * 100;

    // Set data at the specified database path
    if (Firebase.RTDB.setFloat(&fbdo, "voltage", voltage)) {
      Serial.println("Voltage Data Sent Successfully");
    } else {
      Serial.println("Failed to Send Voltage Data");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "battery_percentage", percentage)) {
      Serial.println("Percentage Data Sent Successfully");
    } else {
      Serial.println("Failed to Send Percentage Data");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, "deviceType", "Battery Charger")) {
      Serial.println("Device Type Data Sent Successfully");
    } else {
      Serial.println("Failed to Send Device Type Data");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    // Code to perform charging operation goes here
    // Once charging is done, you can disable the charging system by setting chargingSystemEnabled = false;
  } else {
    digitalWrite(relay, LOW);
    Serial.println("Charging System Disabled");
    // Code to handle when the charging system is disabled goes here
  }

  // Delay for a while
  delay(200);
}

void clearData() {
  while (data_count != 0) {
    Data[data_count--] = 0;
  }
}
