#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>

// WiFi credentials
const char* ssid = "iPhone (176)";
const char* password = "triyankhar8";

// LCD pins
const int rs = 8, en = 9, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Shift register pins for 7-segment display
#define CLOCK 12
#define LATCH 11
#define DATA 10

// WeatherAPI settings
const String API_KEY = "43e8a2cad3704be787703257242711";
const String BASE_URL = "http://api.weatherapi.com/v1/current.json";
String cities[] = {"Toronto", "Washington", "Paris", "Madrid", "London"};
String currentCity = "";      // Current city name
float currentTemperature = 0; // Current temperature

// Digit patterns for common cathode 7-segment display (QA-QG)
int digits[10][7] = { 
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

// Button and debounce settings
#define CHANGECITY 3
#define CHANGETEMP 4

const int debounceDelay = 50; // Debounce time in milliseconds
int cityIndex = 0;            // To keep track of the current city
unsigned long lastDebounceTime = 0; // Time of the last button press
bool lastButtonState = LOW;   // Last state of the button
bool buttonState = LOW;       // Current state of the button

void setup() {
  Serial.begin(9600); // Start Serial Monitor
  
  // Initialize LCD
  lcd.begin(16, 2); // 16 columns, 2 rows
  lcd.print("Initializing...");
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");

  // Set pin modes for shift register
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(DATA, OUTPUT); 
  
  pinMode(CHANGECITY, OUTPUT); // Button is active LOW
  pinMode(CHANGETEMP, OUTPUT);

  // Display the first city's weather initially
  displayWeather();
}

void loop() {
  // Handle city change button
  int reading = digitalRead(CHANGECITY); // Read the city change button state
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // Reset debounce timer
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) { // Check for button press
      cityIndex = (cityIndex + 1) % 5; // Cycle through 5 cities
      Serial.println("Button pressed! Switching city...");
      displayWeather(); // Display weather for the current city
    }
  }
  lastButtonState = reading; // Save the button state for the next loop iteration

  // Handle temperature toggle button
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

void changeTemp(float tempCelsius) {
  return (tempCelsius * 1.8) + 32;
}

void displayWeather() {
  currentCity = cities[cityIndex]; // Get the city name based on cityIndex
  getWeatherData(currentCity); // Get weather data for the current city

  // Display city name and temperature on LCD
  lcd.clear();
  lcd.print("City: ");
  lcd.setCursor(0,1);
  lcd.print(currentCity);
  delay(2000); // Wait before showing the temperature


  // Display temperature on 7-segment display
  displayTemperature(currentTemperature);
}

void getWeatherData(String city) {
  HTTPClient http;
  String url = BASE_URL + "?key=" + API_KEY + "&q=" + city;

  Serial.println("Requesting weather data for: " + city);
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Response received:");
    Serial.println(payload);
    parseWeatherData(payload);
  } 

  http.end();
}

void parseWeatherData(String payload) {
  DynamicJsonDocument doc(2048);

  // Deserialize JSON response
  DeserializationError error = deserializeJson(doc, payload);

  // Extract temperature
  currentTemperature = doc["current"]["temp_c"].as<float>();

  // Print city name and temperature to Serial Monitor
  Serial.println("City: " + currentCity);
  Serial.println("Temperature: " + String(currentTemperature) + " °C");
}

void showTemp(int digit) {
  // Convert the digit's segment data into a byte
  byte value = 0;
  for (int i = 0; i < 7; i++) {
    value |= (digits[digit][i] << i);
  }
  
  // Send the byte to the shift register
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK, MSBFIRST, value);
  digitalWrite(LATCH, HIGH);
}

void displayTemperature(float temperature) {
  int absTemp = abs((int)temperature); // Handle negative temperatures
  int tens = absTemp / 10;             // Extract the tens place
  int units = absTemp % 10;            // Extract the units place

  // Display tens digit
  showTemp(tens);
  delay(1000);

  // Display units digit
  showTemp(units);
  delay(1000);
}