// ── Smart Home Firmware — Kishore Raja K ──────────────────────────────────
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// ── Pin definitions ────────────────────────────────────────────────────────
#define PIR_PIN       D5    // Digital GPIO — motion sensor interrupt
#define MQ2_PIN       A0    // Analog ADC — smoke/gas sensor
#define RELAY_LOCK    D1    // Relay CH1 — door lock solenoid
#define RELAY_FAN     D2    // Relay CH2 — fan control
#define SMOKE_THRESH  400   // ADC threshold (~calibrated to MQ-2 curve)
#define IDLE_TIMEOUT  600000 // 10 min no-motion → auto power-off

const char* ssid       = "HOME_WIFI";
const char* password   = "*****";
const char* iftttKey   = "YOUR_IFTTT_KEY";

volatile bool motionFlag  = false;
unsigned long lastMotion  = 0;

// ── ISR: motion detected ───────────────────────────────────────────────────
void ICACHE_RAM_ATTR onMotion() { motionFlag = true; }

void sendIFTTT(String event, String val1) {
  if (WiFi.status() != WL_CONNECTED) {
    // Reconnect logic if needed
  }
  HTTPClient http;
  String url = "http://ifttt.com" + event +
               "/with/key/" + iftttKey +
               "?value1=" + val1;
  http.begin(url);
  http.GET();   // fire-and-forget telemetry POST
  http.end();
}

void setup() {
  pinMode(PIR_PIN,    INPUT);
  pinMode(RELAY_LOCK, OUTPUT); digitalWrite(RELAY_LOCK, HIGH); // active LOW — locked
  pinMode(RELAY_FAN,  OUTPUT); digitalWrite(RELAY_FAN,  HIGH);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), onMotion, RISING);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  // ── Motion event ──────────────────────────────────────────────────────
  if (motionFlag) {
    motionFlag  = false;
    lastMotion  = millis();
    sendIFTTT("motion_detected", "PIR_D5");
  }

  // ── Smoke / fire check ────────────────────────────────────────────────
  int smoke = analogRead(MQ2_PIN);
  if (smoke > SMOKE_THRESH) {
    sendIFTTT("fire_alert", String(smoke));
    digitalWrite(RELAY_FAN, LOW);  // turn on exhaust fan
  }

  // ── Idle energy cut-off ───────────────────────────────────────────────
  if (millis() - lastMotion > IDLE_TIMEOUT) {
    digitalWrite(RELAY_FAN,  HIGH); // all non-essential loads OFF
  }

  delay(500);
}
