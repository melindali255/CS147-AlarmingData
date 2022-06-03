#include <Arduino.h>
#include <WiFi.h>
//#include <HttpClient.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <TFT_eSPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

/* Humidity and temperature sensor */
#define DHTPIN 26
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
#define DATA_COLLECTION_MILLIS 1000

/* Light Sensor */
const int SENSORPIN = 36;

/* Buzzer and Button */
#define BUZZER_PIN 12
#define BUZZER_ON_MILLIS 100
#define BUTTON_PIN 33

/* Time Stamp Info */
#define NTP_OFFSET -25200  // In seconds
#define NTP_INTERVAL 60000 // In miliseconds
#define NTP_ADDRESS "north-america.pool.ntp.org"

/* Create time client */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

/* Buzzer Variables */
unsigned long buzzer_timer;
bool buzzerOn = false;
bool buttonPressed = false;
bool buzzerActive = false;
bool buzzerTimerActive = false;

// Pulse Width Modulation Variables for Buzzer
const int freq = 5000;
const int buzzerChannel = 0;
const int resolution = 8;

unsigned long data_collection_timer; // Timer for DHT collection

int setHour;
int setMinute;

/* Connection to AWS Variables */
const char kHostname[] = "44.205.13.56"; // Name of the server we want to connect to
const int kNetworkTimeout = 30 * 1000;   // Number of milliseconds to wait without receiving any data before we give up
const int kNetworkDelay = 1000;          // Number of milliseconds to wait if no data is available before trying again
String temperatures = "";                // List of temperatures collected
String humidities = "";                  // List of humidities collected
String lights = "";                      // List of light values collected

// APARTMENT WIFI
char ssid[] = "VDCN-Resident"; // your network SSID (name)
char pass[] = "AC86fm!6";      // your network password (use for WPA, or use as key for WEP)

// School
// char ssid[] = "UCInet Mobile Access"; // your network SSID (name)
// char pass[] = "";                     // your network password (use for WPA, or use as key for WEP)

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Initialize Display
TFT_eSPI display = TFT_eSPI(135, 240);

void setup()
{

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  ledcSetup(buzzerChannel, freq, resolution);
  ledcAttachPin(BUZZER_PIN, buzzerChannel);
  ledcWriteTone(buzzerChannel, 2000);

  buzzer_timer = millis() + BUZZER_ON_MILLIS;

  data_collection_timer = millis() + DATA_COLLECTION_MILLIS;

  // Turn buzzer off
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
  {
    ledcWrite(buzzerChannel, dutyCycle);
  }

  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));

  // Connect to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("COULD NOT CONNECT.");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());

  // Initializes the display board
  display.TFT_eSPI::init();
  display.fillScreen(TFT_BLACK);
  display.setTextColor(0xFFFF);
  display.drawNumber(0, 35, 100, 7);
  display.setRotation(1);

  dht.begin();
  timeClient.begin();

  // Setting the buzzer
  int err = 0;

  WiFiClient c;
  HttpClient http = HttpClient(c, kHostname, 5000);

  // Redefine kPath to include variables
  http.beginRequest();

  err = http.get("/time"); // Send the actual POST request
  http.endRequest();

  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);
      String response = http.responseBody(); // Get the response to the request
      Serial.println(response);              // Print request answer

      // Parse the response
      int hour = response.substring(20, 22).toInt();
      int minutes = response.substring(23, 25).toInt();

      setHour = hour;
      setMinute = minutes;
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
}

void loop()
{
  // Update time
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();

  // Print out time on display
  display.fillScreen(TFT_BLACK);
  display.drawString(formattedTime, 7, 5, 7);

  int currentHour = formattedTime.substring(0, 2).toInt();
  int currentMinute = formattedTime.substring(3, 5).toInt();
  int currentSecond = formattedTime.substring(6, 8).toInt();

  // Check for alarm
  if (!buzzerTimerActive && setHour == currentHour && setMinute == currentMinute && 0 == currentSecond)
  {
    Serial.println("Time matches set time - turning on buzzer");
    buzzerActive = true;
    buzzerTimerActive = true;
  }

  // Play the buzzer
  if (buzzerActive)
  {

    if (buzzerOn && millis() < buzzer_timer)
    {
      for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++)
      {
        ledcWrite(buzzerChannel, dutyCycle);
      }
    }
    else if (!buzzerOn && millis() < buzzer_timer)
    {
      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
      {
        ledcWrite(buzzerChannel, 0);
      }
    }
    else if (buzzerOn && millis() > buzzer_timer)
    {
      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
      {
        ledcWrite(buzzerChannel, 0);
      }
      buzzer_timer = millis() + BUZZER_ON_MILLIS;
      buzzerOn = false;
    }
    else if (!buzzerOn && millis() > buzzer_timer)
    {
      for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++)
      {
        ledcWrite(buzzerChannel, dutyCycle);
      }
      buzzer_timer = millis() + BUZZER_ON_MILLIS;
      buzzerOn = true;
    }

    // Check push button state
    int pushButtonState = digitalRead(BUTTON_PIN);

    if (pushButtonState == LOW)
    {
      Serial.println("Button pushed");
      buttonPressed = true;
      buzzerActive = false;
      // buzzerTimerActive = false;

      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--)
      {
        ledcWrite(buzzerChannel, dutyCycle);
      }

      /* Sending data to AWS */

      WiFiClient c;
      HttpClient http = HttpClient(c, kHostname, 5000);

      // Repeat three times: temperature, humidity, and light
      for (int i = 0; i < 3; i++)
      {
        int err = 0;

        http.beginRequest();

        // Redefine kPath to include variables
        char kPath[1000] = "";
        String temps = "";

        if (i == 0)
        {
          temps = "/plot_temp?temp=" + temperatures;
        }
        else if (i == 1)
        {
          temps = "/plot_humidity?humidity=" + humidities;
        }
        else if (i == 2)
        {
          temps = "/plot_light?light=" + lights;
        }

        strcpy(kPath, temps.c_str());

        Serial.println("query: " + temps);

        err = http.get(kPath);

        http.endRequest();

        if (err == 0)
        {
          Serial.println("startedRequest ok");

          err = http.responseStatusCode();
          if (err >= 0)
          {
            Serial.print("Got status code: ");
            Serial.println(err);
          }
          else
          {
            Serial.print("Getting response failed: ");
            Serial.println(err);
          }
        }
        else
        {
          Serial.print("Connect failed: ");
          Serial.println(err);
        }
      }
    }
  }

  if (buzzerActive && millis() > data_collection_timer)
  {
    float h = dht.readHumidity();
    float f = dht.readTemperature(true); // Fahrenheit

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(f))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    temperatures += String(f) + "%2C";
    humidities += String(f) + "%2C";

    // Read the light value
    int lightVal = analogRead(SENSORPIN);
    lights += String(lightVal) + "%2C";

    data_collection_timer = data_collection_timer + DATA_COLLECTION_MILLIS;
  }
}
