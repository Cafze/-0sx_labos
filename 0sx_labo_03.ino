#include <LCD_I2C.h>

LCD_I2C lcd(0x27, 16, 2);

String nom = "JUSTE";
String numeroEtudiant = "6369240";
String deuxDerniers = "40";

const int thermPin = A0;
const int joyXPin = A1;
const int joyYPin = A2;
const int boutonPin = 2;
const int ledPin = 8;

bool affichage = 0;
int posX = 0;
int posY = 0;
bool clim = false;

bool ancienBouton = HIGH;

unsigned long tempsLCD = 0;
unsigned long tempsSerialPrint = 0;
unsigned long tempsPosition = 0;


byte symbole[8] = {
  B10100, B10100, B11100, B00100, B00111, B00101, B00101, B00111
};

float lireTemperature(int valAnalog) {
  float R1 = 10000;
  float logR2, R2, temp, tempCelcius;
  float c1 = 1.129148e-03, c2 = 2.34125e-04, c3 = 8.76741e-08;
  R2 = R1 * (1023.0 / (float)valAnalog - 1.0);
  logR2 = log(R2);
  temp = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  tempCelcius = temp - 273.15;
  return tempCelcius;
}

int estClic(unsigned long ct) {
  static unsigned long lastTime = 0;
  static int lastState = HIGH;
  const int rate = 50;
  int clic = 0;

  if (ct - lastTime < rate) {
    return clic;
  }
  lastTime = ct;
  int state = digitalRead(boutonPin);
  if (state == LOW) {
    if (lastState == HIGH) {
      clic = 1;
    }
  }
  lastState = state;
  return clic;
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(boutonPin, INPUT_PULLUP);

  Serial.begin(115200);

  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, symbole);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(nom);

  lcd.setCursor(0, 1);
  lcd.write((byte)0);

  lcd.setCursor(14, 1);
  lcd.print(deuxDerniers);
}

void loop() {
  int delayStart = 3000;
  if (millis() < delayStart) {
    return;
  }
  int valTherm = analogRead(thermPin);
  float temperature = lireTemperature(valTherm);

  if (temperature > 25.0) {
    clim = true;
  } else if (temperature < 24.0) {
    clim = false;
  }

  digitalWrite(ledPin, clim ? HIGH : LOW);

  if (estClic(millis())) {
    affichage = !affichage;
  }

  int valX = analogRead(joyXPin);
  int valY = analogRead(joyYPin);
  int readDelay = 100;
  if (millis() - tempsPosition >= readDelay) {
    tempsPosition = millis();

    if (valX < 400) {
      posX--;
    } else if (valX > 600) {
      posX++;
    }

    if (valY < 400) {
      posY--;
    } else if (valY > 600) {
      posY++;
    }

    if (posX > 100) posX = 100;
    if (posX < -100) posX = -100;
    if (posY > 100) posY = 100;
    if (posY < -100) posY = -100;
  }

  int lcdDelay = 100;
  if (millis() - tempsLCD >= lcdDelay) {

    tempsLCD = millis();

    lcd.clear();

    if (affichage == 0) {

      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      lcd.print(temperature, 1);
      lcd.print("C");

      lcd.setCursor(0, 1);
      lcd.print("AC:");
      if (clim) {
        lcd.print("ON");
      } else {
        lcd.print("OFF");
      }
    } else {
      lcd.setCursor(0, 0);
      lcd.print("X:");
      lcd.print(posX);

      lcd.setCursor(0, 1);
      lcd.print("Y:");
      lcd.print(posY);
    }
  }

  int printDelay = 1000;
  if (millis() - tempsSerialPrint >= printDelay) {

    tempsSerialPrint = millis();

    int valX = map(valX, 0, 1023, -100, 100);
    int valY = map(valY, 0, 1023, -100, 100);
    Serial.print("etd:");
    Serial.print(numeroEtudiant);
    Serial.print(",x:");
    Serial.print(valX);
    Serial.print(",y:");
    Serial.print(valY);
    Serial.print(",sys:");
    Serial.println(clim ? 1 : 0);
  }
}