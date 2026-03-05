/*
 * LG NeoChef Inverter Control (EBR82899210)
 * ESP32-S3 BLE Presets + Emergency OFF + Status Notify
 *
 * Commands:
 *   "E" -> 0%
 *   "1" -> 50%
 *   "2" -> 70%
 *   "3" -> 90/95 toggle
 *   "0" -> 25%
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// --- PWM ---
const int PWM_PIN = 1;
const int PWM_FREQ = 220;
const int PWM_RES = 10; // 0..1023

// --- BLE ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic = nullptr;
BLEServer *pServer = nullptr;
volatile bool deviceConnected = false;

volatile int targetPercent = 25;
int currentPercent = 25;
bool toggle95 = false;
String lastCmd = "boot";

unsigned long lastReportMs = 0;
const unsigned long REPORT_INTERVAL_MS = 350;

int pwmFromPercent(int p) {
  p = constrain(p, 0, 100);
  return map(p, 0, 100, 0, 950);
}

void applyPwmPercent(int p) {
  ledcWrite(PWM_PIN, pwmFromPercent(p));
}

void sendStatus(bool forceLog = false) {
  int pwm = pwmFromPercent(currentPercent);
  char line[96];
  snprintf(line, sizeof(line), "STAT cmd=%s target=%d current=%d pwm=%d",
           lastCmd.c_str(), targetPercent, currentPercent, pwm);

  Serial.println(line);

  if (deviceConnected && pCharacteristic) {
    pCharacteristic->setValue((uint8_t *)line, strlen(line));
    pCharacteristic->notify();
  } else if (forceLog) {
    Serial.println("STAT notify skipped (no BLE client)");
  }
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    deviceConnected = true;
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* s) override {
    deviceConnected = false;
    Serial.println("BLE client disconnected, restarting advertising...");
    delay(100);
    s->startAdvertising();
  }
};

class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String v = c->getValue();
    if (v.length() == 0) return;
    v.trim();

    if (v == "E") {
      targetPercent = 0;
    } else if (v == "1") {
      targetPercent = 50;
    } else if (v == "2") {
      targetPercent = 70;
    } else if (v == "3") {
      targetPercent = toggle95 ? 95 : 90;
      toggle95 = !toggle95;
    } else if (v == "0") {
      targetPercent = 25;
    } else {
      return;
    }

    lastCmd = v;
    sendStatus();
  }
};

void setup() {
  Serial.begin(115200);
  delay(1200);

  if (!ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES)) {
    Serial.println("PWM Init Failed!");
    while (1) { delay(100); }
  }

  applyPwmPercent(currentPercent);

  BLEDevice::init("LG-Inverter-Control");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new CharCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  Serial.println("BLE Ready: LG-Inverter-Control");
  Serial.println("Commands: E=OFF, 1=50%, 2=70%, 3=90/95, 0=25%");
  sendStatus(true);
}

void loop() {
  if (currentPercent < targetPercent) currentPercent++;
  else if (currentPercent > targetPercent) currentPercent--;

  applyPwmPercent(currentPercent);

  unsigned long now = millis();
  if (now - lastReportMs >= REPORT_INTERVAL_MS) {
    lastReportMs = now;
    sendStatus();
  }

  delay(25);
}
