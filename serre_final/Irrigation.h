#pragma once

#include <Arduino.h>
#include <AccelStepper.h>

enum class IrrigationState {
  FERME,
  OUVERTURE,
  OUVERT,
  FERMETURE,
  ARRET
};

class Irrigation {
public:
  Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4);

  int getPosition();
  int getPositionPct();

  void setClosedOpenedPos(int closed, int opened);
  int setDistance(int &dist);
  void setBtnClickFlag(bool &clickFlag);

  bool isMoving();
  int getCurrentState();

  void begin();
  void update();

private:
  AccelStepper _stepper;

  int _ledPin;
  int _closedPos = 0;
  int _openedPos = 2038;

  int *_distance = nullptr;
  bool *_clickFlag = nullptr;

  bool _premiereFermeture = true;
  bool _ledState = false;

  unsigned long _lastBlinkTime = 0;
  int _blinkDelay = 100;

  IrrigationState _state = IrrigationState::OUVERT;

  void ouvrir();
  void fermer();
  void stopUrgence();
  void ledBlinkTask();
};