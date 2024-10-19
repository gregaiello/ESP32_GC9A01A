#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <NimBLEDevice.h>

#define TFT_DC  2
#define TFT_MOSI  7
#define TFT_CS 10
#define TFT_CLK 6
#define TFT_BL 3

NimBLEUUID serviceUUID("180D");  // Heart Rate Service
NimBLEUUID charUUID("2A37");     // Heart Rate Measurement Characteristic

NimBLEAdvertisedDevice* myDevice;

int x; 
int y; 
int r;
int hr;

typedef struct {
    const char *name;
    unsigned int value;
} Color;

// Define the colors as an array of Color structs
Color colors[] = {
    {"BLACK", GC9A01A_BLACK},
    {"NAVY", GC9A01A_NAVY},
    {"DARKGREEN", GC9A01A_DARKGREEN},
    {"DARKCYAN", GC9A01A_DARKCYAN},
    {"MAROON", GC9A01A_MAROON},
    {"PURPLE", GC9A01A_PURPLE},
    {"OLIVE", GC9A01A_OLIVE},
    {"LIGHTGREY", GC9A01A_LIGHTGREY},
    {"DARKGREY", GC9A01A_DARKGREY},
    {"BLUE", GC9A01A_BLUE},
    {"GREEN", GC9A01A_GREEN},
    {"CYAN", GC9A01A_CYAN},
    {"RED", GC9A01A_RED},
    {"MAGENTA", GC9A01A_MAGENTA},
    {"YELLOW", GC9A01A_YELLOW},
    {"WHITE", GC9A01A_WHITE},
    {"ORANGE", GC9A01A_ORANGE},
    {"GREENYELLOW", GC9A01A_GREENYELLOW},
    {"PINK", GC9A01A_PINK}
};

#define NUM_COLORS (sizeof(colors) / sizeof(colors[0]))

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK);

bool doConnect = false;
bool connected = false;
bool doScan = false;
NimBLEClient* pClient = nullptr;

// Callback to handle notifications from the HRM
class MyClientCallback : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("Connected to HRM");
        tft.fillScreen(GC9A01A_BLACK);
        tft.setCursor(50, 50);
        tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
        tft.setTextSize(1);
        tft.println("HRM Connected");
    }

    void onDisconnect(NimBLEClient* pClient) {
        Serial.println("Disconnected from HRM");
        tft.fillScreen(GC9A01A_BLACK);
        tft.setCursor(50, 50);
        tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
        tft.setTextSize(1);
        tft.println("HRM Disonnected");
        connected = false;
        doScan = true;
    }
};

// Callback to handle found devices during scanning
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.print("Device found: ");
        Serial.println(advertisedDevice->toString().c_str());

        // Check if the device is a heart rate monitor
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID)) {
            Serial.println("Heart Rate Monitor found, stopping scan...");
            NimBLEDevice::getScan()->stop();
            myDevice = advertisedDevice;
            doConnect = true;
        }
    }
};

// Function to connect to the server (HRM)
bool connectToServer() {
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the HRM
    if (!pClient->connect(myDevice)) {
        Serial.println("Failed to connect.");
        return false;
    }

    // Once connected, we search for the Heart Rate service
    NimBLERemoteService* pService = pClient->getService(serviceUUID);  // Use NimBLERemoteService here
    if (pService == nullptr) {
        Serial.println("Heart Rate service not found.");
        pClient->disconnect();
        return false;
    }

    NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(charUUID);
    if (pCharacteristic == nullptr) {
        Serial.println("Heart Rate characteristic not found.");
        pClient->disconnect();
        return false;
    }

    connected = true;
    return true;
}

// Callback function to handle notifications from the HRM
void onNotifyHRM(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length > 0) {
        // The first byte contains the flags, and the heart rate data follows
        tft.setCursor(110, 110);
        tft.setTextColor(GC9A01A_BLACK);  tft.setTextSize(5);
        tft.setTextSize(5);
        tft.println(hr);
        hr = pData[1];  // Heart rate is usually in the second byte
        tft.setCursor(110, 110);
        tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(5);
        tft.setTextSize(5);
        tft.println(hr);
    }
}

void setup() {
  Serial.begin(115200);

  Serial.println("Start LCD Init ..");

  tft.begin();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Backlight on
  tft.fillScreen(GC9A01A_BLACK);

  Serial.println("Start BLE Init ..");

  // Initialize BLE
  NimBLEDevice::init("");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); 
  // Start scanning for devices
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(5, false);  // Scan for 5 seconds
}

void loop(void) {
  // If we need to connect
  if (doConnect) {
    if (connectToServer()) {
      tft.fillScreen(GC9A01A_BLACK);
      Serial.println("Connected to HRM and ready to read data.");
      tft.setCursor(50, 50);
      tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
      tft.setTextSize(1);
      tft.println("HRM Connected");
      delay(500);
    } else {
      tft.fillScreen(GC9A01A_BLACK);
      Serial.println("Failed to connect to the HRM.");
      tft.setCursor(50, 50);
      tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
      tft.setTextSize(1);
      tft.println("HRM Not Connected");
      delay(500);
    }
    doConnect = false;
  }

  // If we are connected, we can read data
  if (connected) {
    NimBLERemoteCharacteristic* pRemoteCharacteristic = pClient->getService(serviceUUID)->getCharacteristic(charUUID);
      if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->subscribe(true, onNotifyHRM);
      }
  }

  if (doScan) {
    NimBLEDevice::getScan()->start(0, false);  // Start scan forever
    doScan = false;
  }

  delay(500);
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  tft.fillScreen(GC9A01A_RED);
  yield();
  tft.fillScreen(GC9A01A_GREEN);
  yield();
  tft.fillScreen(GC9A01A_BLUE);
  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  return micros() - start;
}

unsigned long testText() {
  tft.fillScreen(GC9A01A_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(GC9A01A_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(GC9A01A_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(GC9A01A_BLACK);
  yield();

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);

  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(GC9A01A_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(GC9A01A_BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
    yield();
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(GC9A01A_BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(i, i, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(0, i*10, i*10));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(i*10, i*10, 0));
    yield();
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(GC9A01A_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
    yield();
  }

  return micros() - start;
}
