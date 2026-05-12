#pragma once

enum class Symbole { INACTIF, ACTIF, AVANCE, RECULE };
enum class ConvState { MANUEL, CONSTANT, ACTIF, INACTIF };

#include <Arduino.h>
#include <OneButton.h>
#include <U8g2lib.h>

class Convoyeur {
public:
  Convoyeur(byte motPin1, byte motPin2, byte manette, byte btnPin, byte aff_CLK, byte aff_DIN, byte aff_CS);

  bool estEnFonction();
  void update();
  int getVitesse();
  void setVitesseCommande(int vitesse);
  void begin();

private:
  byte _moteur_pin_1 = 0;
  byte _moteur_pin_2 = 0;
  byte _manette = 0;
  byte _btn_pin = 0;
  byte _aff_CLK = 0;
  byte _aff_DIN = 0;
  byte _aff_CS = 0;

  int _joystickDeadZone = 50;
  int _vitesse = 0;
  int _vitesseConstante = 0;

  OneButton _button;
  bool _buttonPressed = false;
  bool _longPress = false;

  ConvState _state = ConvState::INACTIF;

  U8G2_MAX7219_8X8_F_4W_SW_SPI _u8g2;

  static void buttonClick(void *context);
  static void longPress(void *context);

  void manuelState();
  void constantState();
  void actifState();
  void inactifState();

  int readJoystick();
  void setMoteurVitesse(int vitesse);
  void setAffichage(Symbole symbole);
};