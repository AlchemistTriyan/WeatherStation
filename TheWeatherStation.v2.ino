// PROJECT  : The Weather Station
// PURPOSE  : To show the temperature on a display and city on a LCD screen. 
// AUTHOR   : Triyan Khare
// COURSE   : ICS4U-E
// DATE     : 2024 01 18
// MCU      : 328P (Nano)
// STATUS   : Working

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
unsigned long lastDebounceTimeCity = 0; // Time of the last city button press
unsigned long lastDebounceTimeTemp = 0; // Time of the last temp button press
bool lastCityButtonState = HIGH;   // Last state of the city button
bool lastTempButtonState = HIGH;   // Last state of the temp button

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

  // Set pin modes
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(DATA, OUTPUT);
  
  pinMode(CHANGECITY, INPUT_PULLUP); // Button is active LOW
  pinMode(CHANGETEMP, INPUT_PULLUP);

  // Display the first city's weather initially
  displayWeather();
}

void loop() {
  // Handle city change button
  int readingCity = digitalRead(CHANGECITY);
  if (readingCity != lastCityButtonState) {
    lastDebounceTimeCity = millis(); // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeCity) > debounceDelay) {
    if (readingCity == LOW && lastCityButtonState == HIGH) {
      cityIndex = (cityIndex + 1) % 5; // Cycle through 5 cities
      Serial.println("Switching city...");
      displayWeather(); // Update the weather display
    }
  }
  lastCityButtonState = readingCity;

  // Handle temperature toggle button
  int readingTemp = digitalRead(CHANGETEMP);
  if (readingTemp != lastTempButtonState) {
    lastDebounceTimeTemp = millis(); // Reset debounce timer
  }

  if ((millis() - lastDebounceTimeTemp) > debounceDelay) {
    if (readingTemp == LOW && lastTempButtonState == HIGH) {
      int fahrenheitTemp = changeTemp(currentTemperature);
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(fahrenheitTemp);
      lcd.print(" F  ");
    }
  }
  lastTempButtonState = readingTemp;
}

int changeTemp(float tempCelsius) {
  return (tempCelsius * 1.8) + 32;
}

void displayWeather() {
  currentCity = cities[cityIndex];
  getWeatherData(currentCity);

  lcd.clear();
  lcd.print("City: ");
  lcd.setCursor(0, 1);
  lcd.print(currentCity);

  delay(2000); // Display city name briefly
  lcd.clear();
  lcd.print("Temp: ");
  lcd.print(currentTemperature);
  lcd.print(" C");

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
    parseWeatherData(payload);
  } else {
    Serial.println("Error in HTTP request");
  }

  http.end();
}

void parseWeatherData(String payload) {
  DynamicJsonDocument doc(2048);

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  currentTemperature = doc["current"]["temp_c"].as<float>();
  Serial.println("City: " + currentCity);
  Serial.println("Temperature: " + String(currentTemperature) + " °C");
}

void showTemp(int digit) {
  byte value = 0;
  for (int i = 0; i < 7; i++) {
    value |= (digits[digit][i] << i);
  }
  
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK, MSBFIRST, value);
  digitalWrite(LATCH, HIGH);
}

void displayTemperature(float temperature) {
  int absTemp = abs((int)temperature);
  int tens = absTemp / 10;
  int units = absTemp % 10;

  showTemp(tens);
  delay(1000);

  showTemp(units);
  delay(1000);
}
