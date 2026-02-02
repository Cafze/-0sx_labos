enum etatAppli {CLIGNOTEMENT, VARIATION, ALLUMER_ETEINT};

etatAppli etatActuel = CLIGNOTEMENT;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

void clignotement(){
  Serial.println("Clignotement - 6369240");
  for(int i = 0; i < 2; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  etatActuel = VARIATION;
}

void variation(){
  Serial.println("Variation - 6369240");
  for(int i = 0; i <= 255; i++){
    analogWrite(LED_BUILTIN, i);
    delay(8);
  }
  etatActuel = ALLUMER_ETEINT;
}

void allumerEteint(){
  Serial.println("Allume - 6369240");  
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  etatActuel = CLIGNOTEMENT;
}

void loop() {
  switch(etatActuel){
    case CLIGNOTEMENT:
      clignotement();
      break;
    case VARIATION:
      variation();
      break;
    case ALLUMER_ETEINT:
      allumerEteint();
      break;
  }
}
