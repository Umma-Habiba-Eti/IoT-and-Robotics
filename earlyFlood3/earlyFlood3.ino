//Tech Trends Shameer
#define BLYNK_TEMPLATE_ID "TMPL6EBvx9GVW"
#define BLYNK_TEMPLATE_NAME "earlyFlood"
#define BLYNK_AUTH_TOKEN "RLxz6KpM2rkwDIEpuKeeQUWyhvWFNlrP"

#define BLYNK_PRINT Serial
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
//#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "Habiba";  // type your WiFi name
char pass[] = "1122334455";  // type your WiFi password

BlynkTimer timer;

#define DHTPIN 27 // Connect Out pin to D2 in NODE MCU
#define DHTTYPE DHT11
#define flowSensorPin 2
#define buzzerPin 32  
  // Buzzer connected to GPIO 32
#define ledPin 12
     // LED connected to GPIO 33

DHT dht(DHTPIN, DHTTYPE);

#define echo 25
#define trig 26

volatile int pulseCount = 0;
float flowRate = 0.0;
float h;
float t;
long duration;
int distance;
float totalLiters = 0.0;
unsigned long previousMillis = 0;
const float calibrationFactor = 4.5;

// Thresholds for flood detection
const int floodDistanceThreshold = 10; // Distance in cm
const float floodFlowRateThreshold = 10.0; // Flow rate in L/min

void pulseCounter() {
  pulseCount++;
}

void flow() {
  unsigned long currentMillis = millis();

  // Calculate flow rate every second (adjust as needed)
  if (currentMillis - previousMillis >= 1000) {
    // Disable interrupts while calculating
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));

    // Calculate flow rate in L/min
    flowRate = ((1000.0 / (currentMillis - previousMillis)) * pulseCount) / calibrationFactor;

    // Add to total liters
    totalLiters += (flowRate / 60);

    // Print results to the Serial Monitor
    Serial.print("Flow rate: ");
    Serial.print(flowRate);
    Serial.print(" L/min");
    Serial.print(" | Total: ");
    Serial.print(totalLiters);
    Serial.println(" L");
    Blynk.virtualWrite(V3, flowRate);
    display.print("Flow: ");
   display.print(flowRate);
   display.println(" L/min");

    // Reset pulse count and time
    pulseCount = 0;
    previousMillis = currentMillis;

    // Re-enable interrupts
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
  }

 
}

void ultrasonic() {


  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  duration = pulseIn(echo, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Blynk.virtualWrite(V2, distance);
  display.print("Distance: ");
  display.print(distance);
  display.println(" cm");

  

  delay(500);
}


void sendSensor() {
   h = dht.readHumidity();
   t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Send data to Blynk
   display.print("Temp: ");
    display.print(t);
    display.println(" C");
    display.print("Humidity: ");
    display.print(h);
    display.println(" %");
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Serial.print("Temperature : ");
  Serial.print(t);
  Serial.print("    Humidity : ");
  Serial.println(h);
}

void triggerFloodWarning() {
  // Blink LED and sound buzzer
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzzerPin, HIGH);
  Blynk.virtualWrite(V4, 1);
  delay(1000);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  delay(500);

  Serial.println("FLOOD DETECTED! Warning triggered.");
}

void setup() {
  Serial.begin(115200);
   if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  

  Blynk.begin(auth, ssid, pass);

  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);
  pulseCount = 0;
  previousMillis = millis();

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  dht.begin();

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  timer.setInterval(100L, sendSensor);
}

void loop() {
  Blynk.run();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  sendSensor();
  ultrasonic();
  flow();
  display.display();


  // Flood condition: Check distance
  if (distance < floodDistanceThreshold) {
    triggerFloodWarning();
  }

   // Flood condition: Check flow rate
  if (flowRate > floodFlowRateThreshold) {
    triggerFloodWarning();
  }
}
