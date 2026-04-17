#include <LiquidCrystal_I2C.h>
#include <OneButton.h>
#include <DHT.h>
#include <HCSR04.h>
#include <AccelStepper.h>

#define LED_PIN 9
#define PHOTO_PIN A0
#define BTN_PIN 4

#define DHTPIN 7
#define DHTTYPE DHT11

#define TRIGGER_PIN 12
#define ECHO_PIN 11

#define LCD_ADDR 0x27

#define MOTOR_INTERFACE_TYPE 4
#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37

#define VANNE_FERMEE_POS 0
#define VANNE_OUVERTE_POS 2038

enum AppState {
  DEMARRAGE,
  DHT_STATE,
  LUM_DIST,
  CALIBRATION,
  OUVERTURE,
  FERMETURE,
  VANNE_OUVERTE,
  VANNE_FERMEE,
  ARRET_URGENCE
};

AppState currentState = DEMARRAGE;

bool premiereFermeture = true;

unsigned long currentTime = 0;
unsigned long bootStartTime = 0;
unsigned long lastLcdTime = 0;

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneButton button(BTN_PIN, true);
DHT dht(DHTPIN, DHTTYPE);
UltraSonicDistanceSensor hc(TRIGGER_PIN, ECHO_PIN);

AccelStepper stepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

int lum = 0;
int lumMin = 1023;
int lumMax = 0;
int lumPercent = 0;

float temperature = 0;
float humidity = 0;
float distanceCm = 0;

long vannePosition = VANNE_OUVERTE_POS;
int vannePercent = 100;

bool ledState = false;

void setup() {
  Serial.begin(9600);

  initPins();
  lcdInit();

  button.attachClick(onSimpleClick);
  button.attachDoubleClick(onDoubleClick);

  dht.begin();

  stepper.setMaxSpeed(500);
  stepper.setAcceleration(100);

  stepper.setCurrentPosition(VANNE_OUVERTE_POS);

  bootStartTime = millis();

  lcd.clear();
}

void initPins() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);
}

void lcdInit() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  currentTime = millis();
  button.tick();
  if (currentState != ARRET_URGENCE) {
    stepper.run();
  }

  if (currentState != CALIBRATION) {
    readLuminosite(currentTime);
  }

  readDHT(currentTime);
  readDistance(currentTime);
  calculVannePercent();
  serialDisplay(currentTime);

  automaticIrrigation();

  switch (currentState) {
    case DEMARRAGE: bootState(currentTime); break;

    case DHT_STATE:
      dhtState(currentTime);
      ledTask();
      break;

    case LUM_DIST:
      lumDistState(currentTime);
      ledTask();
      break;

    case CALIBRATION:
      calibration(currentTime);
      calibrationState(currentTime);
      ledTask();
      break;

    case OUVERTURE:
      ouvertureVanneTask();
      vanneStateDisplay(currentTime);
      ledBlinkTask(currentTime);
      break;

    case FERMETURE:
      fermetureVanneTask();
      vanneStateDisplay(currentTime);
      ledBlinkTask(currentTime);
      break;

    case VANNE_OUVERTE:
      vanneStateDisplay(currentTime);
      ledTask();
      break;

    case VANNE_FERMEE:
      vanneStateDisplay(currentTime);
      ledTask();
      break;

    case ARRET_URGENCE:
      vanneStateDisplay(currentTime);
      digitalWrite(LED_PIN, LOW);
      break;
  }
}

void onSimpleClick() {
  if (currentState == OUVERTURE || currentState == FERMETURE) {
    stepper.stop();
    currentState = ARRET_URGENCE;
    lastLcdTime = 0;
    return;
  }

  if (currentState == ARRET_URGENCE) {
    currentState = OUVERTURE;
    lastLcdTime = 0;
    stepper.moveTo(VANNE_OUVERTE_POS);
    return;
  }

  if (currentState == VANNE_OUVERTE || currentState == VANNE_FERMEE) {
    currentState = DHT_STATE;
    lastLcdTime = 0;
    return;
  }

  if (currentState == CALIBRATION) {
    currentState = DHT_STATE;
    lastLcdTime = 0;
    return;
  }

  if (currentState == DHT_STATE) {
    currentState = LUM_DIST;
    lastLcdTime = 0;
    return;
  }

  if (currentState == LUM_DIST) {
    if (vannePercent >= 100) {
      currentState = VANNE_OUVERTE;
    } 
    else if (vannePercent <= 0) {
      currentState = VANNE_FERMEE;
    } 
    else {
      currentState = ARRET_URGENCE;
    }
    lastLcdTime = 0;
    return;
  }
}

void onDoubleClick() {
  if (currentState != OUVERTURE && currentState != FERMETURE && currentState != ARRET_URGENCE) {
    currentState = CALIBRATION;
    lastLcdTime = 0;
  }
}

void bootState(unsigned long ct) {
  if (ct - lastLcdTime >= 250 || lastLcdTime == 0) {
    lastLcdTime = ct;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ben: 6369240");

    lcd.setCursor(0, 1);
    lcd.print("Vanne ouverte");
  }

  if (ct - bootStartTime >= 3000) {
    lancerPremiereFermeture();
  }
}

void lancerPremiereFermeture() {
  currentState = FERMETURE;
  stepper.moveTo(VANNE_FERMEE_POS);
}

void dhtState(unsigned long ct) {
  if (ct - lastLcdTime >= 500 || lastLcdTime == 0) {
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
  if (ct - lastLcdTime >= 500 || lastLcdTime == 0) {
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
  if (ct - lastLcdTime >= 500 || lastLcdTime == 0) {
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

void vanneStateDisplay(unsigned long ct) {
  if (ct - lastLcdTime >= 200 || lastLcdTime == 0) {
    lastLcdTime = ct;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Vanne : ");
    lcd.print(vannePercent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Etat : ");

    if (currentState == OUVERTURE) {
      lcd.print("Ouverture");
    } else if (currentState == FERMETURE) {
      lcd.print("Fermeture");
    } else if (currentState == VANNE_OUVERTE) {
      lcd.print("Ouverte");
    } else if (currentState == VANNE_FERMEE) {
      lcd.print("Fermee");
    } else if (currentState == ARRET_URGENCE) {
      lcd.print("Arret");
    }
  }
}

void readLuminosite(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= 1000) {
    lastTime = ct;

    lum = analogRead(PHOTO_PIN);

    if (lumMax > lumMin) {
      lumPercent = map(lum, lumMin, lumMax, 0, 100);
      lumPercent = constrain(lumPercent, 0, 100);
    }
  }
}

void readDHT(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= 5000) {
    lastTime = ct;

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity)) {
      humidity = 0.0;
    }

    if (isnan(temperature)) {
      temperature = 0.0;
    }
  }
}

void readDistance(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= 250) {
    lastTime = ct;

    distanceCm = hc.measureDistanceCm();

    if (distanceCm < 0) {
      distanceCm = 0;
    }
  }
}

void automaticIrrigation() {
  if (currentState == DEMARRAGE ||
      currentState == ARRET_URGENCE ||
      currentState == OUVERTURE ||
      currentState == FERMETURE) {
    return;
  }

  if (distanceCm > 0 && distanceCm < 20) {
    if (vannePercent < 100) {
      currentState = OUVERTURE;
      lastLcdTime = 0;
      stepper.moveTo(VANNE_OUVERTE_POS);
    }
    return;
  }

  if (distanceCm >= 25) {
    if (vannePercent > 0) {
      currentState = FERMETURE;
      lastLcdTime = 0;
      stepper.moveTo(VANNE_FERMEE_POS);
    }
    return;
  }
}

void ouvertureVanneTask() {
  if (distanceCm >= 25) {
    currentState = FERMETURE;
    stepper.moveTo(VANNE_FERMEE_POS);
    return;
  }

  if (stepper.distanceToGo() == 0) {
    currentState = VANNE_OUVERTE;
  }
}

void fermetureVanneTask() {

  if (premiereFermeture == false) {
    if (distanceCm > 0 && distanceCm < 20) {
      currentState = OUVERTURE;
      stepper.moveTo(VANNE_OUVERTE_POS);
      return;
    }
  }

  if (stepper.distanceToGo() == 0) {
    premiereFermeture = false;
    currentState = VANNE_FERMEE;
  }
}

void calculVannePercent() {
  vannePosition = stepper.currentPosition();

  vannePercent = map(vannePosition, VANNE_FERMEE_POS, VANNE_OUVERTE_POS, 0, 100);
}

void serialDisplay(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= 3000) {
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
    Serial.print(humidity, 1);

    Serial.print(",Van:");
    Serial.println(vannePercent);
  }
}

void ledTask() {
  if (lumPercent < 30) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void ledBlinkTask(unsigned long ct) {
  static unsigned long lastBlinkTime = 0;

  if (ct - lastBlinkTime >= 100) {
    lastBlinkTime = ct;

    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}

void calibration(unsigned long ct) {
  static unsigned long lastTime = 0;
  if (ct - lastTime >= 100) {
    lastTime = ct;
    lum = analogRead(PHOTO_PIN);

    if (lum < lumMin) {
      lumMin = lum;
    }
    if (lum > lumMax) {
      lumMax = lum;
    }
  }
}