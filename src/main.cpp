#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

// --- WIFI config ---
const char *ssid = "gPBL-AP1";
const char *password = "";

// --- BLYNK API ---
String blynkToken = "dR7I5WVupqfN98dRMJGBUDvXhntZRuMy";
String serverName = "http://blynk.cloud/external/api/batch/update?token=" + blynkToken;
String lastCommand = "";

const int MPU_ADDR = 0x68; // I2C address of MPU6050

void setup()
{
  Serial.begin(9600);

  // Force ESP32 use correct I2C pin (SDA=21, SCL=22)
  Wire.begin(21, 22);

  // Wake MPU6050 (write 0 at register 0x6B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  byte error = Wire.endTransmission(true);

  if (error == 0)
  {
    Serial.println("\nMPU6050 da thuc day va san sang!");
  }
  else
  {
    Serial.println("\nLoi ket noi MPU6050! Kiem tra lai day cam.");
    while (1)
      ;
  }

  // WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Dang ket noi WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi da ket noi thanh cong!");
}

byte mapAccelToByte(int16_t accelValue)
{
  int mapped = map(accelValue, -17000, 17000, 0, 254);
  return (byte)constrain(mapped, 0, 254);
}

// Send HTTP GET to Blynk
void sendBlynkCommand(int v1, int v2, int v3, int v4)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = serverName + "&V0=255&V1=" + String(v1) + "&V2=" + String(v2) + "&V3=" + String(v3) + "&V4=" + String(v4);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("Sent API, Ma: ");
      Serial.println(httpResponseCode);
    }
    else
    {
      Serial.println("Error, cannot send API!");
    }
    http.end();
  }
}

void loop()
{
  // Request MPU6050 to send X and Y axis data (Starting from register 0x3B)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 4, true); // Request 4 bytes (2 bytes for X, 2 bytes for Y)

  // Combine the bytes into actual values
  int16_t ax = (Wire.read() << 8 | Wire.read());
  int16_t ay = (Wire.read() << 8 | Wire.read());

  byte xVal = mapAccelToByte(ax);
  byte yVal = mapAccelToByte(ay);

  String currentCommand = "stop";
  int v1 = 0, v2 = 0, v3 = 0, v4 = 0;

  // --- CONTROL LOGIC ---
  if (yVal <= 75)
  {
    currentCommand = "forward";
    v1 = 1;
  }
  else if (yVal >= 175)
  {
    currentCommand = "backward";
    v2 = 1;
  }
  else if (xVal <= 75)
  {
    currentCommand = "left";
    v3 = 1;
  }
  else if (xVal >= 175)
  {
    currentCommand = "right";
    v4 = 1;
  }

  // Only send API when hand movement changes
  if (currentCommand != lastCommand)
  {
    Serial.print("Huong di chuyen: ");
    Serial.println(currentCommand);
    sendBlynkCommand(v1, v2, v3, v4);
    lastCommand = currentCommand;
  }

  delay(100);
}