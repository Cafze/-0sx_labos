const int potPin = A1;
const int buttonPin = 2;
const int led1 = 11;
const int led2 = 10;
const int led3 = 9;
const int led4 = 8;

const int readInterval = 20;
const int delayRebond = 30;
unsigned long previousTime = 0;

int potValue = 0;
int scaledValue = 0;
bool validated = false;
bool etatPrecedent = HIGH;    
unsigned long dernierChangement = 0;
bool etatBouton = HIGH;
int maxPotentiometre = 1023;
int maxEchelle = 20;
int maxJauge = 20;

void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - previousTime >= readInterval) {
    previousTime = currentTime;

    potValue = analogRead(potPin);
    scaledValue = map(potValue, 0, maxPotentiometre, 0, maxEchelle);
  }

  bool etatPresent = digitalRead(buttonPin);
  
  if (etatPresent != etatPrecedent) {
    dernierChangement = currentTime;
  }

  if (currentTime - dernierChangement > delayRebond) {
    if (etatPresent != etatBouton){
      etatBouton = etatPresent;
      if (etatBouton == LOW) {
        validated = true;
      }
    }
  }
  etatPrecedent = etatPresent;

  if (validated) {
    float percent = (scaledValue / (maxEchelle * 1.0)) * 100.0;
    digitalWrite(led1, percent >= 0   && percent < 25);
    digitalWrite(led2, percent >= 25  && percent < 50);
    digitalWrite(led3, percent >= 50  && percent < 75);
    digitalWrite(led4, percent >= 75  && percent <= 100);

    Serial.print("[");
    for (int i = 0; i < maxJauge; i++) {
      if (i < scaledValue) Serial.print("*");
      else Serial.print(".");
    }
    Serial.print("] ");
    Serial.print(percent);
    Serial.println("%");
  }
}




