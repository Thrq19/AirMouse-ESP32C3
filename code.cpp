#include <BleMouse.h>
#include <Wire.h>

BleMouse bleMouse("AirMouse C3", "ESP32", 100);

// ================= PENGATURAN PIN =================
const int PIN_LEFT_CLICK  = 10;
const int PIN_RIGHT_CLICK = 7;
const int SDA_PIN = 8;
const int SCL_PIN = 9;

const int PIN_SCROLL_UP   = 4;
const int PIN_MID_CLICK   = 3; 
const int PIN_SCROLL_DOWN = 2; 
const int PIN_BATTERY     = 1; 

// ================= VARIABEL SENSOR =================
const int MPU_ADDR = 0x68; 
const int deadzone = 100;     
const float sensitivity = 350.0; 

RTC_DATA_ATTR long gyroXOffset = 0;
RTC_DATA_ATTR long gyroYOffset = 0;
RTC_DATA_ATTR long gyroZOffset = 0;
RTC_DATA_ATTR bool isCalibrated = false; 

float remainX = 0, remainY = 0;

bool lastLeftState = HIGH;
bool lastRightState = HIGH;
bool lastMidState = HIGH;

unsigned long bothPressedTimer = 0;
bool isBothPressed = false;
unsigned long lastScrollTime = 0;
const int scrollDelay = 50; 

// ================= POWER MANAGEMENT (STANDBY MODE) =================
unsigned long lastActivityTime = 0;
bool isIdle = false; // Status apakah mouse sedang istirahat

bool wasConnected = false;
unsigned long disconnectTimer = 0;
unsigned long lastBatteryCheck = 0;

unsigned long clickFreezeTimer = 0;
const int clickFreezeDuration = 150; 

// ================= FUNGSI KALIBRASI =================
void calibrateSensor() {
  Serial.println("\n--- MULAI KALIBRASI ---");
  delay(2000); 
  
  for (int i = 0; i < 100; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    Wire.read(); Wire.read(); Wire.read(); 
    Wire.read(); Wire.read(); Wire.read();
    delay(3);
  }

  long sumX = 0, sumY = 0, sumZ = 0;
  int numSamples = 500; 

  for (int i = 0; i < numSamples; i++) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    sumX += (int16_t)(Wire.read() << 8 | Wire.read());
    sumY += (int16_t)(Wire.read() << 8 | Wire.read());
    sumZ += (int16_t)(Wire.read() << 8 | Wire.read());
    delay(3);
  }
  
  gyroXOffset = sumX / numSamples;
  gyroYOffset = sumY / numSamples;
  gyroZOffset = sumZ / numSamples;
  
  remainX = 0; remainY = 0;
  isCalibrated = true; 
}

// ================= FUNGSI BATERAI =================
void checkBattery() {
  int rawADC = analogRead(PIN_BATTERY);
  float voltage = (rawADC / 4095.0) * 3.3 * 2.0; 
  int batteryPercentage = (int)((voltage - 3.2) / (4.2 - 3.2) * 100.0);
  
  batteryPercentage = constrain(batteryPercentage, 0, 100);
  bleMouse.setBatteryLevel(batteryPercentage);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_LEFT_CLICK, INPUT_PULLUP);
  pinMode(PIN_RIGHT_CLICK, INPUT_PULLUP);
  pinMode(PIN_SCROLL_UP, INPUT_PULLUP);
  pinMode(PIN_MID_CLICK, INPUT_PULLUP);
  pinMode(PIN_SCROLL_DOWN, INPUT_PULLUP);

  checkBattery();
  bleMouse.begin();
  delay(3000); 

  Wire.begin(SDA_PIN, SCL_PIN);
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A); 
  Wire.write(0x04); 
  Wire.endTransmission(true);

  if (!isCalibrated) {
    calibrateSensor();
  } else {
    delay(500); 
  }

  lastActivityTime = millis();
}

void loop() {
  if (bleMouse.isConnected()) {
    wasConnected = true; 
    disconnectTimer = 0; 

    if (millis() - lastBatteryCheck > 10000) {
      checkBattery();
      lastBatteryCheck = millis();
    }

    // --- CEK APAKAH WAKTUNYA ISTIRAHAT (5 DETIK TANPA GERAKAN/KLIK) ---
    if (!isIdle && (millis() - lastActivityTime > 5000)) {
      isIdle = true;
    }

    if (isIdle) {
      // ========================================================
      // 1. MODE STANDBY / MENGINTIP (HEMAT BATERAI)
      // ========================================================
      
      // Bangunkan sensor sekejap
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x6B); 
      Wire.write(0x00); 
      Wire.endTransmission(true);

      delay(10); // Tunggu sensor stabil
      
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x43); 
      Wire.endTransmission(false);
      Wire.requestFrom(MPU_ADDR, 6, true); 

      int16_t rawX = (int16_t)(Wire.read() << 8 | Wire.read()); 
      int16_t rawY = (int16_t)(Wire.read() << 8 | Wire.read()); 
      int16_t rawZ = (int16_t)(Wire.read() << 8 | Wire.read()); 

      long finalGyroY = rawY - gyroYOffset;
      long finalGyroZ = rawZ - gyroZOffset;

      // Cek apakah ada tombol yang ditekan saat alat istirahat
      bool isAnyButtonPressed = (digitalRead(PIN_LEFT_CLICK) == LOW || digitalRead(PIN_RIGHT_CLICK) == LOW || 
                                 digitalRead(PIN_MID_CLICK) == LOW || digitalRead(PIN_SCROLL_UP) == LOW || 
                                 digitalRead(PIN_SCROLL_DOWN) == LOW);

      // Jika alat diguncang (threshold dikali 2 agar tidak mudah salah bangun) ATAU tombol ditekan
      if (abs(finalGyroZ) > (deadzone * 2) || abs(finalGyroY) > (deadzone * 2) || isAnyButtonPressed) {
        isIdle = false; // Bangun penuh
        lastActivityTime = millis();
        
        // Kembalikan filter Ultra-Smooth
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x1A); 
        Wire.write(0x04); 
        Wire.endTransmission(true);
      } else {
        // Jika tidak ada getaran, suruh sensor tidur lagi
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x6B); 
        Wire.write(0x40); // 0x40 adalah mode Sleep MPU6500
        Wire.endTransmission(true);
        
        // Jeda agak panjang agar ESP32 juga hemat daya
        delay(100); 
      }

    } else {
      // ========================================================
      // 2. MODE AKTIF (PERGERAKAN ULTRA-SMOOTH)
      // ========================================================
      bool leftState = digitalRead(PIN_LEFT_CLICK);
      bool rightState = digitalRead(PIN_RIGHT_CLICK);
      bool midState = digitalRead(PIN_MID_CLICK);
      bool isScrollUp = (digitalRead(PIN_SCROLL_UP) == LOW);
      bool isScrollDown = (digitalRead(PIN_SCROLL_DOWN) == LOW);

      // Reset timer istirahat jika ada tombol yang ditekan
      if (leftState == LOW || rightState == LOW || midState == LOW || isScrollUp || isScrollDown) {
        lastActivityTime = millis();
      }

      // --- KALIBRASI MANUAL ---
      if (leftState == LOW && rightState == LOW) {
        if (!isBothPressed) {
          isBothPressed = true;
          bothPressedTimer = millis(); 
        } else if (millis() - bothPressedTimer >= 2000) {
          bleMouse.release(MOUSE_LEFT);
          bleMouse.release(MOUSE_RIGHT);
          calibrateSensor(); 
          isBothPressed = false; 
          lastActivityTime = millis();
          delay(500); 
          return; 
        }
      } else {
        isBothPressed = false; 
      }

      // --- KLIK NORMAL & ANTI-SHAKE ---
      if (!isBothPressed) {
        if (leftState == LOW && lastLeftState == HIGH) {
          bleMouse.press(MOUSE_LEFT);
          clickFreezeTimer = millis(); 
        }
        if (leftState == HIGH && lastLeftState == LOW) bleMouse.release(MOUSE_LEFT);
        
        if (rightState == LOW && lastRightState == HIGH) {
          bleMouse.press(MOUSE_RIGHT);
          clickFreezeTimer = millis();
        }
        if (rightState == HIGH && lastRightState == LOW) bleMouse.release(MOUSE_RIGHT);

        if (midState == LOW && lastMidState == HIGH) {
          bleMouse.press(MOUSE_MIDDLE);
          clickFreezeTimer = millis();
        }
        if (midState == HIGH && lastMidState == LOW) bleMouse.release(MOUSE_MIDDLE);
      }
      
      lastLeftState = leftState;
      lastRightState = rightState;
      lastMidState = midState;

      // --- BACA SENSOR GERAK ---
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x43); 
      Wire.endTransmission(false);
      Wire.requestFrom(MPU_ADDR, 6, true); 

      int16_t rawX = (int16_t)(Wire.read() << 8 | Wire.read()); 
      int16_t rawY = (int16_t)(Wire.read() << 8 | Wire.read()); 
      int16_t rawZ = (int16_t)(Wire.read() << 8 | Wire.read()); 

      long finalGyroX = rawX - gyroXOffset;
      long finalGyroY = rawY - gyroYOffset;
      long finalGyroZ = rawZ - gyroZOffset;

      float exactMoveX = 0, exactMoveY = 0;

      if (abs(finalGyroZ) > deadzone) exactMoveX = -(finalGyroZ / sensitivity);
      if (abs(finalGyroY) > deadzone) exactMoveY = -(finalGyroY / sensitivity);

      exactMoveX += remainX;
      exactMoveY += remainY;

      int moveX = (int)exactMoveX;
      int moveY = (int)exactMoveY;

      if (millis() - clickFreezeTimer > clickFreezeDuration) {
        remainX = exactMoveX - moveX;
        remainY = exactMoveY - moveY;

        if (moveX != 0 || moveY != 0) {
          bleMouse.move(moveX, moveY);
          lastActivityTime = millis(); // Reset timer istirahat jika mouse bergeser
        }
      } else {
        remainX = 0;
        remainY = 0; 
      }

      if ((isScrollUp || isScrollDown) && (millis() - lastScrollTime >= scrollDelay)) {
        if (isScrollUp) bleMouse.move(0, 0, 1);
        if (isScrollDown) bleMouse.move(0, 0, -1);
        lastScrollTime = millis();
      }
    }

  } else {
    // --- JIKA BLUETOOTH TERPUTUS / PC MATI ---
    if (wasConnected) {
      if (disconnectTimer == 0) disconnectTimer = millis();
      
      // Lakukan restart pintar tanpa hilang kalibrasi (Berkat memori RTC)
      if (millis() - disconnectTimer > 3000) {
        delay(100);
        ESP.restart(); 
      }
    }
  }
  
  // Delay normal saat alat aktif beroperasi
  delay(10); 
}
