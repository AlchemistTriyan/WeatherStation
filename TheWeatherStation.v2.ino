
// PROJECT  : The Weather Station
// PURPOSE  : To show the temperature on a display and city on a LCD screen. 
// AUTHOR   : Mr. D’Arcy
// COURSE   : ICS4U-E
// DATE     : 2024 01 18
// MCU      : 328P (Nano)
// STATUS   : Working

#include <WiFi.h>          // WiFi Library 
#include <HTTPClient.h>    // HTTPClient Library
#include <ArduinoJson.h>   // ArduinoJson Library
#include <LiquidCrystal.h> // LCD Library

// WiFi credentials
const char* ssid = "iPhone (176)";     // Connect to my iPhone hotspot 
const char* password = "triyankhar8";  // Password of my hotspot

// LCD pins
const int rs = 8, en = 9, d4 = 7, d5 = 6, d6 = 5, d7 = 4; // Define LCD pins 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);                // Initialize LCD 

#define CLOCK 12 // Define Clock pin
#define LATCH 11 // Define Latch pin
#define DATA 10  // Define Data pin 

const String API_KEY = "43e8a2cad3704be787703257242711"; // API_Key
const String BASE_URL = "http://api.weatherapi.com/v1/current.json"; // API URL
String cities[] = {"Toronto", "Washington", "Paris", "Madrid", "London"}; // 5 Cities to get weather temp from 
String currentCity = "";      // Current city name
float currentTemperature = 0; // Current temperature

int digits[10][7] = { // Array for common cathode 7-segment display (QA-QG)
  {1, 1, 1, 1, 1, 1, 0}, // digit 0
  {0, 1, 1, 0, 0, 0, 0}, // digit 1
  {1, 1, 0, 1, 1, 0, 1}, // digit 2
  {1, 1, 1, 1, 0, 0, 1}, // digit 3
  {0, 1, 1, 0, 0, 1, 1}, // digit 4
  {1, 0, 1, 1, 0, 1, 1}, // digit 5
  {1, 0, 1, 1, 1, 1, 1}, // digit 6
  {1, 1, 1, 0, 0, 0, 0}, // digit 7
  {1, 1, 1, 1, 1, 1, 1}, // digit 8
  {1, 1, 1, 1, 0, 1, 1}  // digit 9
};

#define CHANGECITY 3 // Define pin that toggles the cities
#define CHANGETEMP 4 // Define pin that changes the temp 

const int debounceDelay = 50;       // Debounce time in milliseconds
int cityIndex = 0;                  // To keep track of the current city
unsigned long lastDebounceTime = 0; // Time of the last button press
bool lastButtonState = LOW;         // Last state of the button
bool buttonState = LOW;             // Current state of the button

void setup() {
  Serial.begin(9600); // Start Serial Monitor
  
  lcd.begin(16, 2);             // Initialize LCD with 16 columns and 2 rows
  lcd.print("Initializing..."); // Print “initializing”
  
  WiFi.begin(ssid, password);               // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi..."); // Print “Connecting to Wi-Fi”
  while (WiFi.status() != WL_CONNECTED) {   // Track the Wifi connection
    delay(500);                             // Delay by 500 ms
    Serial.print(".");                      // Print “.”
  }
  Serial.println("\nConnected to Wi-Fi");   //Print “Connected to Wi-Fi”

  pinMode(CLOCK, OUTPUT);      // Set the clock as output 
  pinMode(LATCH, OUTPUT);      // Set the latch as output
  pinMode(DATA, OUTPUT);       // Set the data as output  
  
  pinMode(CHANGECITY, INPUT); // Set the changecity pin to input
  pinMode(CHANGETEMP, INPUT); // Set the changetemp pin to input

  displayWeather();           // Display the first city's weather initially
}

void loop() {
  int reading = digitalRead(CHANGECITY); // Read the city change button state
  if (reading != lastButtonState) { // Handle button debounce
    lastDebounceTime = millis(); // Reset debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay) { // Create button debounce 
    if (reading == LOW && lastButtonState == HIGH) { // Check for button press
      cityIndex = (cityIndex + 1) % 5; // Cycle through 5 cities
      Serial.println("Button pressed! Switching city...");
      displayWeather(); // Display weather for the current city
    }
  }
  lastButtonState = reading; // Save the button state for the next loop iteration
  int tempReading = digitalRead(CHANGETEMP); // Read the temperature toggle button state
  if (tempReading != lastTempButtonState) {
    lastDebounceTime = millis(); // Reset debounce timer for temperature button
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (tempReading == LOW && lastTempButtonState == HIGH) { // Check for button press
      int fahrenheitTemp = changeTemp(currentTemperature); // Convert to Fahrenheit
      Serial.println("Temperature in Fahrenheit: " + String(fahrenheitTemp) + " °F");
    }
  }
  lastTempButtonState = tempReading; // Save the temperature button state for the next loop iteration
}

void changeTemp(float tempCelsius) { // Function to convert Celsius to Fahrenheit
  return (tempCelsius * 1.8) + 32;   // Converts Celsius to Fahrenheit
}

void displayWeather() {            // Displays the weather 
  currentCity = cities[cityIndex]; // Get the city name based on cityIndex
  getWeatherData(currentCity); // Get weather data for the current city
  lcd.clear();                // Clear the LCD
  lcd.print("City: ");        // Print “City:”
  lcd.setCursor(0,1);         // Set the cursor to (0,1)
  lcd.print(currentCity);     // Print the current city
  delay(2000);               // Wait before showing the temperature
  displayTemperature(currentTemperature); // Display temperature on 7-segment display
}

void getWeatherData(String city) {   // Get weather data 
  HTTPClient http;                   // Create an http client 
  String url = BASE_URL + "?key=" + API_KEY + "&q=" + city; // Create the string for the API
  Serial.println("Requesting weather data for: " + city); // Print the city
  http.begin(url);                  // Begin HTTP communication 
  int httpResponseCode = http.GET();// Get an httpPresponseCode (1 is successful)
  if (httpResponseCode > 0) { // When HTTP Response code is successful 
    String payload = http.getString();// Get the required info for the specific city
    Serial.println("Response received:"); // Print “Response received:
    Serial.println(payload);              // Print the JSON formatted response 
    parseWeatherData(payload);            // Parse the data for the ESP32
  } 
  http.end();                             // End HTTP communication
}

void parseWeatherData(String payload) { // Parse the data for the city
  DynamicJsonDocument doc(2048); // Create a document of 2048 bytes 
  DeserializationError error = deserializeJson(doc, payload);  // Deserialize JSON response
  currentTemperature = doc["current"]["temp_c"].as<float>(); // Extract temperature
  Serial.println("City: " + currentCity);  // Print city to Serial Monitor  Serial.println("Temperature: " + String(currentTemperature) + " °C"); // Print temp
}

void showTemp(int digit) {
  byte value = 0; // Convert the digit's segment data into a byte
  for (int i = 0; i < 7; i++) {
    value |= (digits[digit][i] << i); // Turn corresponding segments on for each digit
  }
  digitalWrite(LATCH, LOW);                // Set the latch LOW
  shiftOut(DATA, CLOCK, MSBFIRST, value);  // shiftOut to displays 
  digitalWrite(LATCH, HIGH);               // Set the latch HIGH
}

void displayTemperature(float temperature) {// display the temperature on displays
  int absTemp = abs((int)temperature); // Handle negative temperatures
  int tens = absTemp / 10;             // Extract the tens place
  int units = absTemp % 10;            // Extract the units place

  showTemp(tens);  // Display tens digit
  delay(1000);     // Wait a second

  showTemp(units); // Display units digit
  delay(1000);     // Wait a second 
}

