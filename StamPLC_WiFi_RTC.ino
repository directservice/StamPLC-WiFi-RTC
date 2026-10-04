/*
 * StamPLC Wi-Fi & RTC Auto-Sync Firmware
 * 
 * This code demonstrates how to use the M5StamPLC library with:
 * - Wi-Fi connectivity
 * - Automatic RTC time synchronization via NTP
 * - PLC input/output monitoring
 * - Environmental sensor monitoring (LM75B, INA226)
 * - Status display and LED indication
 * - IP Address and MAC Address display
 * 
 * Hardware: M5StamPLC with Stamp-S3A control module
 * Required Library: M5StamPLC (install via Arduino IDE Library Manager)
 * 
 * Author: Copilot
 * Date: 2025
 */

#include <M5StamPLC.h>
#include <WiFi.h>
#include <time.h>

// ============================================================================
// CONFIGURATION SECTION
// ============================================================================

// Wi-Fi Credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// NTP Server Configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 0;              // Change for your timezone (e.g., 3600 for GMT+1)
const int DAYLIGHT_OFFSET_SEC = 0;          // Daylight saving offset in seconds

// Update intervals (milliseconds)
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;   // Update display every 1 second
const unsigned long SENSOR_READ_INTERVAL = 5000;      // Read sensors every 5 seconds
const unsigned long RTC_SYNC_INTERVAL = 3600000;      // Sync RTC every 1 hour

// Display mode (rotate between screens)
const unsigned long SCREEN_ROTATION_INTERVAL = 5000;  // Switch screen every 5 seconds

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastRtcSync = 0;
unsigned long lastScreenRotation = 0;

bool wifiConnected = false;
bool rtcSynced = false;

// Display screen index (0 = main, 1 = network, 2 = sensors)
uint8_t displayScreenIndex = 0;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void initWiFi();
void connectWiFi();
void disconnectWiFi();
bool isWiFiConnected();
void syncRTCWithNTP();
void updateDisplay();
void readSensors();
void controlOutputs();
void handleButtonInput();
void displayStartupScreen();
void displayMainScreen();
void displayNetworkInfoScreen();
void displaySensorDataScreen();
void displayIOStatusScreen();
void setStatusLED(uint8_t r, uint8_t g, uint8_t b);
String getMacAddress();

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n=== StamPLC Wi-Fi & RTC Firmware ===");
  Serial.println("Initializing M5StamPLC...");
  
  // Initialize M5StamPLC (includes display, buttons, sensors)
  M5StamPLC.begin();
  
  // Set LCD brightness
  M5StamPLC.setBacklight(true);
  
  // Display startup screen
  displayStartupScreen();
  
  // Initialize Wi-Fi
  initWiFi();
  
  // Connect to Wi-Fi
  connectWiFi();
  
  // Sync RTC with NTP if Wi-Fi is connected
  if (wifiConnected) {
    syncRTCWithNTP();
  }
  
  Serial.println("Setup complete!");
  delay(2000);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Update M5StamPLC (handles button input, display refresh)
  M5StamPLC.update();
  
  // Handle user button input
  handleButtonInput();
  
  // Update display at regular intervals
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentTime;
    updateDisplay();
  }
  
  // Read sensors at regular intervals
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentTime;
    readSensors();
  }
  
  // Periodically sync RTC with NTP
  if (wifiConnected && (currentTime - lastRtcSync >= RTC_SYNC_INTERVAL)) {
    lastRtcSync = currentTime;
    syncRTCWithNTP();
  }
  
  // Monitor Wi-Fi connection status
  if (!isWiFiConnected()) {
    wifiConnected = false;
    setStatusLED(255, 0, 0);  // Red: disconnected
  } else {
    wifiConnected = true;
    setStatusLED(0, 255, 0);  // Green: connected
  }
  
  delay(10);  // Small delay to prevent watchdog timeout
}

// ============================================================================
// WI-FI FUNCTIONS
// ============================================================================

void initWiFi() {
  Serial.println("Initializing Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("StamPLC");
}

void connectWiFi() {
  Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
  
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().setTextSize(2);
  M5StamPLC.Lcd().setCursor(0, 10);
  M5StamPLC.Lcd().println("Connecting WiFi...");
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  const int MAX_ATTEMPTS = 40;  // 20 seconds timeout
  
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    M5StamPLC.Lcd().print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("Wi-Fi connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("MAC Address: %s\n", getMacAddress().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    
    M5StamPLC.Lcd().clear();
    M5StamPLC.Lcd().setTextColor(TFT_GREEN);
    M5StamPLC.Lcd().setTextSize(2);
    M5StamPLC.Lcd().println("WiFi Connected!");
    M5StamPLC.Lcd().setTextSize(1);
    M5StamPLC.Lcd().setTextColor(TFT_WHITE);
    M5StamPLC.Lcd().println(WiFi.localIP().toString().c_str());
    M5StamPLC.Lcd().println(getMacAddress().c_str());
    
    setStatusLED(0, 255, 0);  // Green LED
  } else {
    wifiConnected = false;
    Serial.println("Wi-Fi connection failed!");
    Serial.printf("Status: %d\n", WiFi.status());
    
    M5StamPLC.Lcd().clear();
    M5StamPLC.Lcd().setTextColor(TFT_RED);
    M5StamPLC.Lcd().setTextSize(2);
    M5StamPLC.Lcd().println("WiFi Failed!");
    M5StamPLC.Lcd().setTextSize(1);
    M5StamPLC.Lcd().println("Check credentials");
    
    setStatusLED(255, 0, 0);  // Red LED
  }
  
  delay(2000);
}

void disconnectWiFi() {
  WiFi.disconnect(true);  // true = turn off Wi-Fi radio
  wifiConnected = false;
  Serial.println("Wi-Fi disconnected");
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// ============================================================================
// RTC & NTP FUNCTIONS
// ============================================================================

void syncRTCWithNTP() {
  if (!wifiConnected) {
    Serial.println("Wi-Fi not connected, cannot sync RTC");
    return;
  }
  
  Serial.println("Syncing RTC with NTP server...");
  
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setTextColor(TFT_CYAN);
  M5StamPLC.Lcd().setTextSize(2);
  M5StamPLC.Lcd().println("Syncing RTC...");
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().println(NTP_SERVER);
  
  setStatusLED(0, 255, 255);  // Cyan LED: syncing
  
  // Configure time with NTP
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  // Wait for time to be set (max 15 seconds)
  int attempts = 0;
  time_t now = time(nullptr);
  
  while (now < 24 * 3600 && attempts < 30) {
    delay(500);
    Serial.print(".");
    M5StamPLC.Lcd().print(".");
    now = time(nullptr);
    attempts++;
  }
  
  Serial.println();
  
  if (now > 24 * 3600) {
    rtcSynced = true;
    struct tm* timeinfo = localtime(&now);
    
    Serial.println("RTC synced successfully!");
    Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo->tm_year + 1900,
      timeinfo->tm_mon + 1,
      timeinfo->tm_mday,
      timeinfo->tm_hour,
      timeinfo->tm_min,
      timeinfo->tm_sec);
    
    // Set the hardware RTC
    M5StamPLC.setRtcTime(timeinfo);
    
    M5StamPLC.Lcd().clear();
    M5StamPLC.Lcd().setTextColor(TFT_GREEN);
    M5StamPLC.Lcd().setTextSize(2);
    M5StamPLC.Lcd().println("RTC Synced!");
    M5StamPLC.Lcd().setTextSize(1);
    M5StamPLC.Lcd().printf("%04d-%02d-%02d\n", 
      timeinfo->tm_year + 1900, 
      timeinfo->tm_mon + 1, 
      timeinfo->tm_mday);
    M5StamPLC.Lcd().printf("%02d:%02d:%02d\n", 
      timeinfo->tm_hour, 
      timeinfo->tm_min, 
      timeinfo->tm_sec);
    
    setStatusLED(0, 255, 0);  // Green LED
  } else {
    rtcSynced = false;
    Serial.println("RTC sync failed (timeout)");
    
    M5StamPLC.Lcd().clear();
    M5StamPLC.Lcd().setTextColor(TFT_ORANGE);
    M5StamPLC.Lcd().setTextSize(2);
    M5StamPLC.Lcd().println("RTC Sync Failed!");
    M5StamPLC.Lcd().setTextSize(1);
    M5StamPLC.Lcd().println("Check network");
    
    setStatusLED(255, 165, 0);  // Orange LED
  }
  
  delay(2000);
}

// ============================================================================
// DISPLAY & UI FUNCTIONS
// ============================================================================

void displayStartupScreen() {
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().setTextSize(3);
  M5StamPLC.Lcd().setCursor(0, 10);
  M5StamPLC.Lcd().println("StamPLC");
  
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().println("Wi-Fi & RTC Firmware");
  M5StamPLC.Lcd().println("Version 1.1");
  M5StamPLC.Lcd().println("");
  M5StamPLC.Lcd().println("Initializing...");
}

void updateDisplay() {
  unsigned long currentTime = millis();
  
  // Rotate between different display screens
  if (currentTime - lastScreenRotation >= SCREEN_ROTATION_INTERVAL) {
    lastScreenRotation = currentTime;
    displayScreenIndex = (displayScreenIndex + 1) % 3;
  }
  
  // Display the appropriate screen
  switch (displayScreenIndex) {
    case 0:
      displayMainScreen();
      break;
    case 1:
      displayNetworkInfoScreen();
      break;
    case 2:
      displaySensorDataScreen();
      break;
  }
}

void displayMainScreen() {
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setCursor(0, 0);
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  
  // Display time
  struct tm timeinfo;
  M5StamPLC.getRtcTime(&timeinfo);
  
  M5StamPLC.Lcd().printf("Time: %02d:%02d:%02d\n", 
    timeinfo.tm_hour, 
    timeinfo.tm_min, 
    timeinfo.tm_sec);
  
  M5StamPLC.Lcd().printf("Date: %04d-%02d-%02d\n", 
    timeinfo.tm_year + 1900, 
    timeinfo.tm_mon + 1, 
    timeinfo.tm_mday);
  
  // Display Wi-Fi status
  M5StamPLC.Lcd().print("WiFi: ");
  if (wifiConnected) {
    M5StamPLC.Lcd().setTextColor(TFT_GREEN);
    M5StamPLC.Lcd().printf("ON (%d dBm)\n", WiFi.RSSI());
  } else {
    M5StamPLC.Lcd().setTextColor(TFT_RED);
    M5StamPLC.Lcd().println("OFF");
  }
  
  // Display RTC sync status
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().print("RTC Synced: ");
  M5StamPLC.Lcd().setTextColor(rtcSynced ? TFT_GREEN : TFT_ORANGE);
  M5StamPLC.Lcd().println(rtcSynced ? "Yes" : "No");
  
  // Display temperature
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  float temp = M5StamPLC.getTemp();
  M5StamPLC.Lcd().printf("Temp: %.2f°C\n", temp);
  
  // Display voltage
  float voltage = M5StamPLC.getPowerVoltage();
  M5StamPLC.Lcd().printf("Volt: %.2fV\n", voltage);
  
  // Display current
  float current = M5StamPLC.getIoSocketOutputCurrent();
  M5StamPLC.Lcd().printf("Current: %.3fA\n", current);
}

void displayNetworkInfoScreen() {
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setCursor(0, 0);
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().setTextColor(TFT_YELLOW);
  
  M5StamPLC.Lcd().println("=== NETWORK INFO ===");
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().println("");
  
  // Display network status
  M5StamPLC.Lcd().print("Status: ");
  if (wifiConnected) {
    M5StamPLC.Lcd().setTextColor(TFT_GREEN);
    M5StamPLC.Lcd().println("CONNECTED");
  } else {
    M5StamPLC.Lcd().setTextColor(TFT_RED);
    M5StamPLC.Lcd().println("DISCONNECTED");
  }
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  
  // Display SSID
  M5StamPLC.Lcd().print("SSID: ");
  M5StamPLC.Lcd().println(WIFI_SSID);
  
  // Display IP Address
  M5StamPLC.Lcd().print("IP: ");
  if (wifiConnected) {
    M5StamPLC.Lcd().setTextColor(TFT_CYAN);
    M5StamPLC.Lcd().println(WiFi.localIP().toString().c_str());
  } else {
    M5StamPLC.Lcd().setTextColor(TFT_GRAY);
    M5StamPLC.Lcd().println("0.0.0.0");
  }
  
  // Display MAC Address
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().print("MAC: ");
  M5StamPLC.Lcd().setTextColor(TFT_CYAN);
  M5StamPLC.Lcd().println(getMacAddress().c_str());
  
  // Display signal strength
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().print("Signal: ");
  if (wifiConnected) {
    M5StamPLC.Lcd().setTextColor(TFT_ORANGE);
    M5StamPLC.Lcd().printf("%d dBm\n", WiFi.RSSI());
  } else {
    M5StamPLC.Lcd().setTextColor(TFT_GRAY);
    M5StamPLC.Lcd().println("N/A");
  }
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().println("");
  M5StamPLC.Lcd().setTextSize(2);
  M5StamPLC.Lcd().setTextColor(TFT_LIGHTGREY);
  M5StamPLC.Lcd().println("(Page 1 of 3)");
}

void displaySensorDataScreen() {
  M5StamPLC.Lcd().clear();
  M5StamPLC.Lcd().setCursor(0, 0);
  M5StamPLC.Lcd().setTextSize(1);
  M5StamPLC.Lcd().setTextColor(TFT_YELLOW);
  
  M5StamPLC.Lcd().println("=== SENSOR DATA ===");
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().println("");
  
  // Read and display sensor values
  float temp = M5StamPLC.getTemp();
  float voltage = M5StamPLC.getPowerVoltage();
  float current = M5StamPLC.getIoSocketOutputCurrent();
  
  M5StamPLC.Lcd().print("Temperature: ");
  M5StamPLC.Lcd().setTextColor(TFT_CYAN);
  M5StamPLC.Lcd().printf("%.2f°C\n", temp);
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().print("Voltage: ");
  M5StamPLC.Lcd().setTextColor(TFT_CYAN);
  M5StamPLC.Lcd().printf("%.2fV\n", voltage);
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().print("Current: ");
  M5StamPLC.Lcd().setTextColor(TFT_CYAN);
  M5StamPLC.Lcd().printf("%.3fA\n", current);
  
  M5StamPLC.Lcd().setTextColor(TFT_WHITE);
  M5StamPLC.Lcd().println("");
  
  // Display input/output status
  M5StamPLC.Lcd().print("Inputs:  ");
  for (int i = 0; i < 8; i++) {
    M5StamPLC.Lcd().print(M5StamPLC.readPlcInput(i) ? "1" : "0");
  }
  M5StamPLC.Lcd().println("");
  
  M5StamPLC.Lcd().print("Relays:  ");
  for (int i = 0; i < 4; i++) {
    M5StamPLC.Lcd().print(M5StamPLC.readPlcRelay(i) ? "1" : "0");
  }
  
  M5StamPLC.Lcd().println("");
  M5StamPLC.Lcd().setTextSize(2);
  M5StamPLC.Lcd().setTextColor(TFT_LIGHTGREY);
  M5StamPLC.Lcd().println("(Page 2 of 3)");
}

void readSensors() {
  float temp = M5StamPLC.getTemp();
  float voltage = M5StamPLC.getPowerVoltage();
  float current = M5StamPLC.getIoSocketOutputCurrent();
  
  Serial.printf("Temperature: %.2f°C\n", temp);
  Serial.printf("Voltage: %.2fV\n", voltage);
  Serial.printf("Current: %.3fA\n", current);
  
  // Read input states
  Serial.print("Inputs: ");
  for (int i = 0; i < 8; i++) {
    Serial.print(M5StamPLC.readPlcInput(i) ? "1" : "0");
  }
  Serial.println();
}

void controlOutputs() {
  // Example: Toggle relay 0 every 5 seconds
  static unsigned long lastToggle = 0;
  static bool relayState = false;
  
  if (millis() - lastToggle > 5000) {
    lastToggle = millis();
    relayState = !relayState;
    M5StamPLC.writePlcRelay(0, relayState);
    Serial.printf("Relay 0: %s\n", relayState ? "ON" : "OFF");
  }
}

void handleButtonInput() {
  if (M5StamPLC.BtnA().wasPressed()) {
    Serial.println("Button A pressed");
    M5StamPLC.tone(1000, 100);  // Beep
    
    if (wifiConnected) {
      syncRTCWithNTP();
    } else {
      connectWiFi();
    }
  }
  
  if (M5StamPLC.BtnB().wasPressed()) {
    Serial.println("Button B pressed");
    M5StamPLC.tone(1000, 100);  // Beep
    
    // Toggle relay 0
    static bool relayToggle = false;
    relayToggle = !relayToggle;
    M5StamPLC.writePlcRelay(0, relayToggle);
    Serial.printf("Relay 0: %s\n", relayToggle ? "ON" : "OFF");
  }
  
  if (M5StamPLC.BtnC().wasPressed()) {
    Serial.println("Button C pressed");
    M5StamPLC.tone(1000, 100);  // Beep
    
    // Manually rotate to next screen
    displayScreenIndex = (displayScreenIndex + 1) % 3;
    lastScreenRotation = millis();
  }
}

void setStatusLED(uint8_t r, uint8_t g, uint8_t b) {
  M5StamPLC.setStatusLight(r, g, b);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

String getMacAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
}

// ============================================================================
// OPTIONAL: Serial Command Handler
// ============================================================================

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "wifi_scan") {
      Serial.println("Scanning Wi-Fi networks...");
      int networks = WiFi.scanNetworks();
      for (int i = 0; i < networks; i++) {
        Serial.printf("%d: %s (%d dBm)\n", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      }
    } 
    else if (command == "wifi_status") {
      Serial.printf("Wi-Fi Status: %s\n", wifiConnected ? "Connected" : "Disconnected");
      if (wifiConnected) {
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("MAC: %s\n", getMacAddress().c_str());
        Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
      }
    } 
    else if (command == "sync_rtc") {
      syncRTCWithNTP();
    } 
    else if (command.startsWith("relay ")) {
      int channel = command.substring(6, 7).toInt();
      bool state = command.substring(8).toInt() ? true : false;
      M5StamPLC.writePlcRelay(channel, state);
      Serial.printf("Relay %d set to: %s\n", channel, state ? "ON" : "OFF");
    } 
    else if (command == "help") {
      Serial.println("Commands:");
      Serial.println("  wifi_scan   - Scan available Wi-Fi networks");
      Serial.println("  wifi_status - Show Wi-Fi connection status");
      Serial.println("  sync_rtc    - Manually sync RTC with NTP");
      Serial.println("  relay <ch> <0/1> - Control relay (e.g., relay 0 1)");
      Serial.println("  help        - Show this help message");
    }
  }
}
