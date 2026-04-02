#include <LiquidCrystal_I2C.h>
#include <OneButton.h>
#include <DHT.h>
#include <HCSR04.h>

#define LED_PIN 9
#define PHOTO_PIN A0
#define BTN_PIN 4
#define DHTPIN 7
#define DHTTYPE DHT11
#define TRIGGER_PIN 12
#define ECHO_PIN 11
#define LCD_ADDR 0x27

enum AppState {DEMARRAGE, DHT_STATE, LUM_DIST, CALIBRATION};

AppState currentState = DEMARRAGE;

unsigned long currentTime = 0;
unsigned long bootStartTime = 0;

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneButton button(BTN_PIN, true);
DHT dht(DHTPIN, DHTTYPE);
UltraSonicDistanceSensor hc(TRIGGER_PIN, ECHO_PIN);

int lum = 0;
int lumMin = 1023;
int lumMax = 0;
int lumPercent = 0;

float temperature = 0.0;
float humidity = 0.0;
float distanceCm = 0.0;

void setup() {
  Serial.begin(9600);

  initPins();
  lcdInit();
  buttonInit();
  dht.begin();

  bootStartTime = millis();
}

void initPins() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);
}

void lcdInit() {
  lcd.init();
  lcd.backlight();
}

void buttonInit() {
  button.attachClick(onSimpleClick);
  button.attachDoubleClick(onDoubleClick);
}

void loop() {
  currentTime = millis();

  button.tick();

  if (currentState != CALIBRATION) {
    readLuminosite(currentTime);
  }

  readDHT(currentTime);
  readDistance(currentTime);
  serialDisplay(currentTime);
  ledTask();

  if (currentState == CALIBRATION) {
    calibration(currentTime);
  }

  switch (currentState) {
    case DEMARRAGE: bootState(currentTime); break;
    case DHT_STATE: dhtState(currentTime); break;
    case LUM_DIST: lumDistState(currentTime); break;
    case CALIBRATION: calibrationState(currentTime); break;
  }
}


void onSimpleClick() {
  if (currentState == CALIBRATION) {
    currentState = DHT_STATE;
    return;
  }
  if (currentState == DHT_STATE) {
    currentState = LUM_DIST;
  } else if (currentState == LUM_DIST) {
    currentState = DHT_STATE;
  }
}

void onDoubleClick() {
    currentState = CALIBRATION;
}

void bootState(unsigned long ct) {
  static unsigned long lastLcdTime = 0;
  int exitTime = 3000;
  int lcdRate = 250;

  if (ct - lastLcdTime > lcdRate) {
    lastLcdTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ben: 6369240");
  }

  if (currentTime < exitTime) { 
    return;
  }

  currentState = DHT_STATE;
}

void dhtState(unsigned long ct) {
  static unsigned long lastLcdTime = 0;
  const unsigned long lcdRate = 250;

  if (ct - lastLcdTime >= lcdRate) {
    lastLcdTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp : ");
    lcd.print(temperature, 1);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Hum : ");
    lcd.print(humidity, 1);
    lcd.print(" %");
  }
}

void lumDistState(unsigned long ct) {
  static unsigned long lastLcdTime = 0;
  int lcdRate = 250;

  if (ct - lastLcdTime >= lcdRate) {
    lastLcdTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lum : ");
    lcd.print(lumPercent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Dist : ");
    lcd.print(distanceCm, 1);
    lcd.print(" cm");
  }
}

void calibrationState(unsigned long ct) {
  static unsigned long lastLcdTime = 0;
  int lcdRate = 250;

  if (ct - lastLcdTime >= lcdRate) {
    lastLcdTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cal Min:");
    lcd.print(lumMin);

    lcd.setCursor(0, 1);
    lcd.print("Cal Max:");
    lcd.print(lumMax);
  }
}

void readLuminosite(unsigned long ct) {
  static unsigned long lastTime = 0;
  int rate = 1000;

  if (ct - lastTime >= rate) {
    lastTime = ct;
    lum = analogRead(PHOTO_PIN);
    lumPercent = map(lum, lumMin, lumMax, 0, 100);
      if (lumPercent < 0) {
        lumPercent = 0;
      }
      if (lumPercent > 100) {
        lumPercent = 100;
      }
  }
}

void readDHT(unsigned long ct) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 5000;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

  }
}

void readDistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  int rate = 250;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    distanceCm = hc.measureDistanceCm();
    if (distanceCm < 0) {
      distanceCm = 0;
    }
  }
}

void serialDisplay(unsigned long ct) {
  static unsigned long lastTime = 0;
  int rate = 3000;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    Serial.print("Lum:");
    Serial.print(lumPercent);
    Serial.print(",Min:");
    Serial.print(lumMin);
    Serial.print(",Max:");
    Serial.print(lumMax);
    Serial.print(",Dist:");
    Serial.print(distanceCm, 1);
    Serial.print(",T:");
    Serial.print(temperature, 1);
    Serial.print(",H:");
    Serial.println(humidity, 1);
  }
}

void ledTask() {
  if (lumPercent < 30) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void calibration(unsigned long ct) {
  static unsigned long lastTime = 0;
  int rate = 100;

  if (ct - lastTime >= rate) {
    lastTime = ct;

    lum = analogRead(PHOTO_PIN);

    if (lum < lumMin) lumMin = lum;
    if (lum > lumMax) lumMax = lum;
  }
}
