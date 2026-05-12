#include "Convoyeur.h"

Convoyeur::Convoyeur(byte motPin1, byte motPin2, byte manette, byte btnPin, byte aff_CLK, byte aff_DIN, byte aff_CS)
  : _moteur_pin_1(motPin1),
    _moteur_pin_2(motPin2),
    _manette(manette),
    _btn_pin(btnPin),
    _aff_CLK(aff_CLK),
    _aff_DIN(aff_DIN),
    _aff_CS(aff_CS),
    _button(btnPin, true),
    _u8g2(U8G2_R0, aff_CLK, aff_DIN, aff_CS, U8X8_PIN_NONE)
{
}

void Convoyeur::begin() {
  pinMode(_moteur_pin_1, OUTPUT);
  pinMode(_moteur_pin_2, OUTPUT);
  pinMode(_manette, INPUT);

  _button.attachClick(buttonClick, this);
  _button.attachLongPressStart(longPress, this);

  _u8g2.begin();

  setMoteurVitesse(0);
  setAffichage(Symbole::INACTIF);
}

bool Convoyeur::estEnFonction() {
  return _state != ConvState::INACTIF;
}

int Convoyeur::getVitesse() {
  return _vitesse;
}

void Convoyeur::setVitesseCommande(int vitesse) {
  if (!estEnFonction()) {
    return;
  }

  vitesse = constrain(vitesse, -100, 100);

  if (vitesse == 0) {
    _vitesse = 0;
    _vitesseConstante = 0;
    _state = ConvState::ACTIF;
    return;
  }

  _vitesse = vitesse;
  _vitesseConstante = vitesse;
  _state = ConvState::CONSTANT;
}

void Convoyeur::buttonClick(void *context) {
  Convoyeur *self = static_cast<Convoyeur*>(context);
  self->_buttonPressed = true;
}

void Convoyeur::longPress(void *context) {
  Convoyeur *self = static_cast<Convoyeur*>(context);
  self->_longPress = true;
}

void Convoyeur::update() {
  _button.tick();

  if (_longPress) {
    _longPress = false;

    if (_state == ConvState::INACTIF) {
      _state = ConvState::ACTIF;
    } else {
      _state = ConvState::INACTIF;
    }
  }

  switch (_state) {
    case ConvState::MANUEL:
      manuelState();
      break;

    case ConvState::CONSTANT:
      constantState();
      break;

    case ConvState::ACTIF:
      actifState();
      break;

    case ConvState::INACTIF:
      inactifState();
      break;
  }
}

int Convoyeur::readJoystick() {
  int valeur = analogRead(_manette);
  int vitesse = map(valeur, 0, 1023, -100, 100);

  if (abs(vitesse) < _joystickDeadZone) {
    vitesse = 0;
  }

  return vitesse;
}

void Convoyeur::manuelState() {
  _vitesse = readJoystick();

  if (_vitesse == 0) {
    _state = ConvState::ACTIF;
    return;
  }

  if (_buttonPressed) {
    _buttonPressed = false;
    _vitesseConstante = _vitesse;
    _state = ConvState::CONSTANT;
    return;
  }

  setMoteurVitesse(_vitesse);

  if (_vitesse > 0) {
    setAffichage(Symbole::AVANCE);
  } else {
    setAffichage(Symbole::RECULE);
  }
}

void Convoyeur::constantState() {
  _vitesse = _vitesseConstante;

  if (_buttonPressed) {
    _buttonPressed = false;
    _vitesse = 0;
    _vitesseConstante = 0;
    _state = ConvState::ACTIF;
    return;
  }

  setMoteurVitesse(_vitesse);

  if (_vitesse > 0) {
    setAffichage(Symbole::AVANCE);
  } else {
    setAffichage(Symbole::RECULE);
  }
}

void Convoyeur::actifState() {
  _vitesse = 0;
  setMoteurVitesse(0);
  setAffichage(Symbole::ACTIF);

  int vitesseJoystick = readJoystick();

  if (vitesseJoystick != 0) {
    _state = ConvState::MANUEL;
  }

  _buttonPressed = false;
}

void Convoyeur::inactifState() {
  _vitesse = 0;
  setMoteurVitesse(0);
  setAffichage(Symbole::INACTIF);
  _buttonPressed = false;
}

void Convoyeur::setMoteurVitesse(int vitesse) {

  int pwm = map(abs(vitesse), 0, 100, 0, 255);

  if (vitesse > 0) {
    analogWrite(_moteur_pin_1, pwm);
    analogWrite(_moteur_pin_2, 0);
  } else if (vitesse < 0) {
    analogWrite(_moteur_pin_1, 0);
    analogWrite(_moteur_pin_2, pwm);
  } else {
    analogWrite(_moteur_pin_1, 0);
    analogWrite(_moteur_pin_2, 0);
  }
}

void Convoyeur::setAffichage(Symbole symbole) {
  static Symbole dernierSymbole = Symbole::INACTIF;
  static bool premierAffichage = true;

  if (!premierAffichage && symbole == dernierSymbole) {
    return;
  }

  premierAffichage = false;
  dernierSymbole = symbole;

  _u8g2.clearBuffer();

  if (symbole == Symbole::INACTIF) {
    _u8g2.drawLine(0, 0, 7, 7);
    _u8g2.drawLine(7, 0, 0, 7);
  }

  else if (symbole == Symbole::ACTIF) {
    _u8g2.drawLine(2, 0, 2, 5);
    _u8g2.drawPixel(2, 7);

    _u8g2.drawLine(5, 0, 5, 5);
    _u8g2.drawPixel(5, 7);
  }

  else if (symbole == Symbole::AVANCE) {
    _u8g2.drawLine(1, 3, 6, 3);
    _u8g2.drawLine(1, 3, 3, 1);
    _u8g2.drawLine(1, 3, 3, 5);
  }

  else if (symbole == Symbole::RECULE) {
    _u8g2.drawLine(1, 3, 6, 3);
    _u8g2.drawLine(6, 3, 4, 1);
    _u8g2.drawLine(6, 3, 4, 5);
  }

  _u8g2.sendBuffer();
}