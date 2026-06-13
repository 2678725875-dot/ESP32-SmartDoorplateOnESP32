#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <WiFiClientSecure.h>

//WiFi和后端配置
const char* ssid = "YOUR_WIFI_SSID";           // 请替换为你的 WiFi 名称
const char* password = "YOUR_WIFI_PASSWORD";     // 请替换为你的 WiFi 密码
const char* serverBase = "YOUR_SERVER_URL";      // 请替换为你的后端服务地址

//HC-SR04引脚
#define TRIG_PIN 26
#define ECHO_PIN 27
#define DISTANCE_THRESHOLD 10
#define COOLDOWN_TIME 1000
#define DETECT_DEBOUNCE 2

TFT_eSPI tft = TFT_eSPI();
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool welcomeMode = false;
unsigned long welcomeStartTime = 0;
unsigned long lastDetectionTime = 0;
bool visitorRecorded = false;
bool welcomeDrawn = false; 

String weatherStr = "25C Sunny";
String messagesList = "No messages";
String lastMessagesList = "";
String lastWeatherStr = "";
unsigned long lastFetchTime = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastDistanceCheck = 0;
bool tftNeedsUpdate = true;

//RB反转，故预先声明
#define FACE_YELLOW   0x07FF
#define FACE_DARK     0x04F0
#define CHEEK_PINK    0xF810

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  tft.println("WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    tft.println("WiFi OK!");
  } else {
    Serial.println("\nWiFi failed!");
    tft.println("WiFi Failed!");
  }
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = duration / 58.0;
  if (duration == 0 || distance > 400) return 999;
  return distance;
}

bool recordVisitor() {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String(serverBase) + "/api/visitors");
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST("{}");
  bool success = false;
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(128);
    deserializeJson(doc, payload);
    int count = doc["count"];
    Serial.println("Visitor recorded! Total: " + String(count));
    success = true;
  } else {
    Serial.println("Visitor record failed, code: " + String(httpCode));
  }
  http.end();
  return success;
}

void drawSmileyFace(int cx, int cy, int faceR) {
  tft.fillCircle(cx, cy, faceR, FACE_YELLOW);
  tft.drawCircle(cx, cy, faceR, FACE_DARK);
  tft.drawCircle(cx, cy, faceR + 1, FACE_DARK);

  tft.fillCircle(cx - 24, cy - 18, 10, 0x0000);
  tft.fillCircle(cx + 24, cy - 18, 10, 0x0000);

  int mcx = cx;
  int mcy = cy + 22;
  int mr = 20;
  tft.fillCircle(mcx, mcy, mr, 0x0000);
  tft.fillRect(mcx - mr - 2, mcy - mr, mr * 2 + 4, mr + 5, FACE_YELLOW);
}

//Welcome界面（只画一次，纯静态）
void drawWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, 240, 42, 0xE2A0);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(52, 10);
  tft.print("WELCOME!");
  tft.fillRect(0, 42, 240, 3, TFT_YELLOW);

  drawSmileyFace(120, 155, 60);

  tft.setTextColor(0x8410);
  tft.setTextSize(1);
  tft.setCursor(70, 220);
  tft.print("Have a nice day!");
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(String(serverBase) + "/api/weather");
  http.setTimeout(5000);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);
    float temp = doc["temp"];
    const char* text = doc["text"];
    String newWeather = String((int)temp) + "C " + String(text);
    if (newWeather != lastWeatherStr) {
      lastWeatherStr = newWeather;
      tftNeedsUpdate = true;
    }
    weatherStr = newWeather;
    Serial.println("Weather: " + weatherStr);
  } else {
    Serial.println("Weather fetch failed, code: " + String(httpCode));
  }
  http.end();
}

void fetchMessages() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(String(serverBase) + "/api/messages");
  http.setTimeout(5000);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, payload);
    JsonArray arr = doc.as<JsonArray>();
    String newMessages = "";
    int count = 0;
    for (JsonObject msg : arr) {
      const char* content = msg["content"];
      const char* createdAt = msg["created_at"];
      String timeStr = String(createdAt).substring(11, 16);
      newMessages += timeStr + " " + String(content) + "\n";
      count++;
    }
    if (count == 0) newMessages = "No messages";
    if (newMessages != lastMessagesList) {
      lastMessagesList = newMessages;
      tftNeedsUpdate = true;
    }
    messagesList = newMessages;
    Serial.println("Messages count: " + String(count));
  } else {
    Serial.println("Messages fetch failed, code: " + String(httpCode));
  }
  http.end();
}

void updateLCD() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    lcd.setCursor(0, 0);
    lcd.print("Waiting Time...");
    return;
  }
  char timeStr[9];
  strftime(timeStr, 9, "%H:%M:%S", &timeinfo);
  lcd.setCursor(0, 0);
  lcd.print(timeStr);
  for (int i = strlen(timeStr); i < 16; i++) lcd.print(" ");

  lcd.setCursor(0, 1);
  lcd.print(weatherStr);
  for (int i = weatherStr.length(); i < 16; i++) lcd.print(" ");
}

void updateTFT() {
  if (!tftNeedsUpdate) return;
  if (welcomeMode) return;

  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, 240, 28, 0x1082);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 4);
  tft.print("SMART DOORPLATE");
  tft.fillRect(0, 28, 240, 2, TFT_CYAN);

  String tempStr = weatherStr;
  String descStr = "";
  int cIdx = weatherStr.indexOf("C");
  if (cIdx > 0) {
    tempStr = weatherStr.substring(0, cIdx);
    descStr = weatherStr.substring(cIdx + 2);
    descStr.trim();
  }
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(10, 36);
  tft.print(tempStr);
  tft.print("\370C");
  tft.setTextColor(0x94B2);
  tft.setTextSize(1);
  tft.setCursor(10, 64);
  tft.print(descStr);

  tft.drawFastHLine(10, 78, 220, 0x4208);

  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(1);
  tft.setCursor(10, 84);
  tft.print("MESSAGES");
  tft.fillRect(10, 96, 50, 2, TFT_CYAN);

  int y = 106;
  int xContent = 56;
  int maxContentChars = 11;
  int lineSpace = 20;
  int start = 0;
  while (start < messagesList.length() && y < 220) {
    int nlPos = messagesList.indexOf('\n', start);
    String line;
    if (nlPos >= 0) {
      line = messagesList.substring(start, nlPos);
      start = nlPos + 1;
    } else {
      line = messagesList.substring(start);
      start = messagesList.length();
    }
    line.trim();
    if (line.length() == 0) continue;

    if (line == "No messages") {
      tft.setTextColor(0x632C);
      tft.setTextSize(2);
      tft.setCursor(10, y);
      tft.print("No messages");
      break;
    }

    int spacePos = line.indexOf(' ');
    if (spacePos > 0) {
      String timePart = line.substring(0, spacePos);
      String contentPart = line.substring(spacePos + 1);

      tft.setTextColor(TFT_CYAN);
      tft.setTextSize(1);
      tft.setCursor(10, y + 4);
      tft.print(timePart);

      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      String remainingContent = contentPart;
      bool firstLine = true;
      while (remainingContent.length() > 0 && y < 220) {
        remainingContent.trim();
        if (remainingContent.length() == 0) break;
        int cx = firstLine ? xContent : 10;
        int maxC = firstLine ? maxContentChars : 15;
        String chunk = remainingContent.substring(0,
          remainingContent.length() > maxC ? maxC : remainingContent.length());
        tft.setCursor(cx, y);
        tft.print(chunk);
        if (remainingContent.length() <= maxC) break;
        remainingContent = remainingContent.substring(maxC);
        y += lineSpace;
        firstLine = false;
      }
    } else {
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.setCursor(10, y);
      tft.print(line.substring(0, 15));
    }
    y += lineSpace;
  }

  tftNeedsUpdate = false;
  Serial.println("TFT updated");
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Booting...");

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");

  connectWiFi();
  configTime(8 * 3600, 0, "ntp.aliyun.com");

  fetchWeather();
  fetchMessages();

  tftNeedsUpdate = true;
  updateTFT();
  updateLCD();
}

void loop() {
  if (millis() - lastDistanceCheck > 200) {
    float distance = getDistance();
    static int closeCount = 0;

    if (distance < DISTANCE_THRESHOLD && distance > 0) {
      closeCount++;
    } else {
      closeCount = 0;
    }

    if (closeCount >= DETECT_DEBOUNCE &&
        (millis() - lastDetectionTime > COOLDOWN_TIME) &&
        !welcomeMode) {
      Serial.println("Person detected! Distance: " + String(distance) + "cm");
      welcomeMode = true;
      welcomeStartTime = millis();
      visitorRecorded = false;
      welcomeDrawn = false;
      lastDetectionTime = millis();
      closeCount = 0;
    }
    lastDistanceCheck = millis();
  }

  if (welcomeMode) {
    unsigned long elapsed = millis() - welcomeStartTime;

    if (!welcomeDrawn) {
      drawWelcomeScreen();
      welcomeDrawn = true;
    }

    //记录访客
    if (!visitorRecorded && elapsed < 2000) {
      static unsigned long lastRetryTime = 0;
      if (millis() - lastRetryTime > 500) {
        if (recordVisitor()) {
          visitorRecorded = true;
          Serial.println("Visitor count saved successfully");
        }
        lastRetryTime = millis();
      }
    }

    //2秒后退出
    if (elapsed > 2000) {
      if (!visitorRecorded) recordVisitor();
      welcomeMode = false;
      tftNeedsUpdate = true;
      updateTFT();
    }
  }

  //数据拉取
  if (!welcomeMode && millis() - lastFetchTime > 10000) {
    fetchWeather();
    fetchMessages();
    lastFetchTime = millis();
  }

  if (millis() - lastLCDUpdate > 1000) {
    updateLCD();
    lastLCDUpdate = millis();
  }

  if (tftNeedsUpdate && !welcomeMode) {
    updateTFT();
  }
}