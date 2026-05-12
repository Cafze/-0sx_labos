#include <LiquidCrystal_I2C.h>
#include <OneButton.h>
#include <DHT.h>
#include <HCSR04.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include "Convoyeur.h"
#include "Irrigation.h"

#define LED_PIN 9
#define PHOTO_PIN A0
#define BTN_PIN 4

#define CONV_MOT_1 5
#define CONV_MOT_2 6
#define CONV_JOY A1
#define CONV_BTN 8
#define CONV_DIN 34
#define CONV_CS 32
#define CONV_CLK 30

#define DHTPIN 7
#define DHTTYPE DHT11

#define TRIGGER_PIN 12
#define ECHO_PIN 11

#define LCD_ADDR 0x27

#define IN_1 31
#define IN_2 33
#define IN_3 35
#define IN_4 37

#define VANNE_FERMEE_POS 0
#define VANNE_OUVERTE_POS 2038

#define AT_BAUD_RATE 115200

const char ssid[] = "TechniquesInformatique-Etudiant";
const char pass[] = "shawi123";

#define MQTT_PORT 1883
#define MQTT_USER "etdshawi"
#define MQTT_PASS "shawi123"

const char mqttServer[] = "arduino.nb.shawinigan.info";
const char mqttClientId[] = "Mega_6369240";
const char mqttPublishTopic[] = "etd/4";
const char mqttSubscribeTopic[] = "etu_04/#";
const char mqttConvVitTopic[] = "etu_04/convVit";

enum LCDState {
  LCD_DEMARRAGE,
  LCD_CLIMAT,
  LCD_LUM_DISTANCE,
  LCD_IRRIGATION,
  LCD_CALIBRATION
};

LCDState lcdState = LCD_DEMARRAGE;

unsigned long currentTime = 0;
unsigned long bootStartTime = 0;
unsigned long lastLcdUpdate = 0;

const unsigned int lcdRefreshRate = 500;

int lum_treshold = 30;
int calibrationDelay = 100;
int serialDelay = 3000;
int distanceDelay = 250;
int luminositeDelay = 1000;
int dhtDelay = 5000;
int mqttPublishDelay = 3000;

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneButton button(BTN_PIN, true);
DHT dht(DHTPIN, DHTTYPE);
UltraSonicDistanceSensor hc(TRIGGER_PIN, ECHO_PIN);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Convoyeur convoyeur(CONV_MOT_1, CONV_MOT_2, CONV_JOY, CONV_BTN, CONV_CLK, CONV_DIN, CONV_CS);
Irrigation irrigation(LED_PIN, IN_1, IN_2, IN_3, IN_4);

int lum = 0;
int lumMin = 1023;
int lumMax = 0;
int lumPercent = 0;

float temperature = 0;
float humidity = 0;
float distanceCm = 0;
int distanceInt = 0;

int vannePercent = 100;
bool irrigationClickFlag = false;

char lastMqttPayload[17] = ""; //reverse la place 

void setup() {
  Serial.begin(115200);

  initPins();
  lcdInit();

  convoyeur.begin();

  irrigation.begin();
  irrigation.setClosedOpenedPos(VANNE_FERMEE_POS, VANNE_OUVERTE_POS);

  button.attachClick(onSimpleClick);
  button.attachDoubleClick(onDoubleClick);

  dht.begin();

  bootStartTime = millis();

  lcd.clear();

  wifiInit();

  mqttClient.setServer(mqttServer, MQTT_PORT);
  mqttClient.setCallback(mqttEvent);
  mqttClient.setBufferSize(256);  // stock les messages MQTT pour que ca coupe pas le JSON
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

  mqttTask(currentTime);

  button.tick();
  convoyeur.update();

  if (lcdState != LCD_CALIBRATION) {
    readLuminosite(currentTime);
  }

  readDHT(currentTime);
  readDistance(currentTime);

  if (lcdState != LCD_DEMARRAGE) {
    distanceInt = (int)distanceCm;
    irrigation.setDistance(distanceInt);
    irrigation.setBtnClickFlag(irrigationClickFlag);
    irrigation.update();
    vannePercent = irrigation.getPositionPct();
  }

  if (lcdState == LCD_CALIBRATION) {
    calibration(currentTime);
  }

  serialDisplay(currentTime);
  lcdTask(currentTime);

  if (!irrigation.isMoving()) {
    ledTask();
  }
}

void onSimpleClick() {
  if (lcdState != LCD_DEMARRAGE) {
    if (irrigation.isMoving() || irrigation.getCurrentState() == (int)IrrigationState::ARRET) {
      irrigationClickFlag = true;
      setLcdState(LCD_LUM_DISTANCE);
      return;
    }
  }

  if (lcdState == LCD_CALIBRATION) {
    setLcdState(LCD_CLIMAT);
    return;
  }

  nextLcdState();
}

void onDoubleClick() {
  if (lcdState != LCD_DEMARRAGE) {
    setLcdState(LCD_CALIBRATION);
  }
}

void setLcdState(LCDState newState) {
  lcdState = newState;
  lastLcdUpdate = 0;
  lcd.clear();
}

void nextLcdState() {
  switch (lcdState) {
    case LCD_DEMARRAGE:
      setLcdState(LCD_CLIMAT);
      break;

    case LCD_CLIMAT:
      setLcdState(LCD_LUM_DISTANCE);
      break;

    case LCD_LUM_DISTANCE:
      setLcdState(LCD_IRRIGATION);
      break;

    case LCD_IRRIGATION:
      setLcdState(LCD_CLIMAT);
      break;

    case LCD_CALIBRATION:
      setLcdState(LCD_CLIMAT);
      break;

    default:
      setLcdState(LCD_CLIMAT);
      break;
  }
}

void lcdTask(unsigned long ct) {
  if (lcdState == LCD_DEMARRAGE && ct - bootStartTime >= 3000) {
    setLcdState(LCD_CLIMAT);
  }

  if (ct - lastLcdUpdate < lcdRefreshRate) {
    return;
  }

  lastLcdUpdate = ct;

  switch (lcdState) {
    case LCD_DEMARRAGE:
      lcdDemarrage();
      break;

    case LCD_CLIMAT:
      lcdClimat();
      break;

    case LCD_LUM_DISTANCE:
      lcdLumDistance();
      break;

    case LCD_IRRIGATION:
      lcdIrrigation();
      break;

    case LCD_CALIBRATION:
      lcdCalibration();
      break;
  }
}
int delayDemarrage = 250;

void lcdDemarrage() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ben: 6369240");

    lcd.setCursor(0, 1);
    lcd.print("Vanne ouverte");
}


void lcdClimat() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Hum : ");
  lcd.print(humidity, 1);
  lcd.print(" %");
}

void lcdLumDistance() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Lum : ");
  lcd.print(lumPercent);
  lcd.print(" %");

  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distanceCm, 1);
  lcd.print(" cm");
}

void lcdCalibration() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Cal Min: ");
  lcd.print(lumMin);

  lcd.setCursor(0, 1);
  lcd.print("Cal Max: ");
  lcd.print(lumMax);
}

void lcdIrrigation() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Vanne: ");
  lcd.print(irrigation.getPositionPct());
  lcd.print(" %");

  lcd.setCursor(0, 1);
  lcd.print("Etat: ");

  int state = irrigation.getCurrentState();

  if (state == (int)IrrigationState::OUVERTURE) {
    lcd.print("Ouverture");
  } else if (state == (int)IrrigationState::FERMETURE) {
    lcd.print("Fermeture");
  } else if (state == (int)IrrigationState::OUVERT) {
    lcd.print("Ouverte");
  } else if (state == (int)IrrigationState::FERME) {
    lcd.print("Fermee");
  } else if (state == (int)IrrigationState::ARRET) {
    lcd.print("Arret");
  }
}

void wifiInit() {
  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("La communication avec le module WiFi a echoue !");
    return;
  }

  Serial.print("Connexion WiFi a : ");
  Serial.println(ssid);

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.println("Echec WiFi, nouvel essai...");
    delay(2000);
  }

  Serial.println("WiFi connecte");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void mqttEvent(char* topic, byte* payload, unsigned int length) {
  unsigned int copyLength = length;

  memcpy(lastMqttPayload, payload, copyLength);
  lastMqttPayload[copyLength] = '\0';


  Serial.print("Message MQTT recu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(lastMqttPayload);


  if (strcmp(topic, mqttConvVitTopic) == 0) {
    int vitesse = atoi(lastMqttPayload);

    if (convoyeur.estEnFonction()) {
      convoyeur.setVitesseCommande(vitesse);
      Serial.print("Vitesse convoyeur appliquée : ");
      Serial.println(vitesse);
    } else {
      Serial.println("Commande ignorée : convoyeur inactif");
    }
  }
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.println("Connexion MQTT...");

    if (mqttClient.connect(mqttClientId, MQTT_USER, MQTT_PASS)) {
      Serial.println("MQTT connecte");
      mqttClient.subscribe("etu_04/#");
    } else {
      Serial.print("Erreur MQTT: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void mqttPublishTask(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (!mqttClient.connected() || ct - lastTime < mqttPublishDelay) {
    return;
  }

  lastTime = ct;
  char message[180];

  char tempStr[10];
  char humStr[10];
  dtostrf(temperature, 0, 1, tempStr);
  dtostrf(humidity, 0, 1, humStr);


  snprintf(message, sizeof(message) ,"{\"Benjamin\":\"6369240\",\"temp\":\"%s\",\"hum\":\"%s\",\"millis\":%lu,\"lum\":%d,\"irrState\":%d,\"irrPos\":%d,\"convVit\":%d}", tempStr, humStr, ct, lumPercent, irrigation.getCurrentState(), irrigation.getPositionPct(), convoyeur.getVitesse());

  if (mqttClient.publish(mqttPublishTopic, message)) {
    Serial.println("JSON envoyé");
  } else {
    Serial.println("Erreur publication MQTT");
  }
}

void mqttTask(unsigned long ct) {
  mqttConnect();

  if (mqttClient.connected()) {
    mqttClient.loop();
    mqttPublishTask(ct);
  }
}

void readLuminosite(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= luminositeDelay) {
    lastTime = ct;

    lum = analogRead(PHOTO_PIN);

    if (lumMax > lumMin) {
      lumPercent = map(lum, lumMin, lumMax, 0, 100);
    }
  }
}

void readDHT(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= dhtDelay) {
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

  if (ct - lastTime >= distanceDelay) {
    lastTime = ct;

    distanceCm = hc.measureDistanceCm();

    if (distanceCm < 0) {
      distanceCm = 0;
    }
  }
}

void serialDisplay(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= serialDelay) {
    lastTime = ct;

    Serial.print("Lum:");
    Serial.print(lumPercent);

    Serial.print(",Dist:");
    Serial.print(distanceCm, 1);

    Serial.print(",T:");
    Serial.print(temperature, 1);

    Serial.print(",H:");
    Serial.print(humidity, 1);

    Serial.print(",Van:");
    Serial.print(vannePercent);

    Serial.print(",Conv:");
    Serial.println(convoyeur.getVitesse());
  }
}

void ledTask() {
  if (lumPercent < lum_treshold) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void calibration(unsigned long ct) {
  static unsigned long lastTime = 0;

  if (ct - lastTime >= calibrationDelay) {
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
