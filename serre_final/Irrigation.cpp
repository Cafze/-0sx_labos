#include "Irrigation.h"

Irrigation::Irrigation(int ledPin, int pin1, int pin2, int pin3, int pin4)
  : _stepper(4, pin1, pin3, pin2, pin4) {
  _ledPin = ledPin;
}

void Irrigation::begin() {
  pinMode(_ledPin, OUTPUT);

  _stepper.setMaxSpeed(500);
  _stepper.setAcceleration(100);
  _stepper.setCurrentPosition(_openedPos);
}

int Irrigation::getPosition() {
  return _stepper.currentPosition();
}

void Irrigation::setClosedOpenedPos(int closed, int opened) {
  _closedPos = closed;
  _openedPos = opened;
  _stepper.setCurrentPosition(_openedPos);
}

int Irrigation::getPositionPct() {
  int pct = map(_stepper.currentPosition(), _closedPos, _openedPos, 0, 100);
  return pct;
}

int Irrigation::setDistance(int &dist) {
  _distance = &dist;
  return dist;
}

void Irrigation::setBtnClickFlag(bool &clickFlag) {
  _clickFlag = &clickFlag;
}

bool Irrigation::isMoving() {
  return _state == IrrigationState::OUVERTURE || _state == IrrigationState::FERMETURE;
}

int Irrigation::getCurrentState() {
  return (int)_state;
}

void Irrigation::ouvrir() {
  _state = IrrigationState::OUVERTURE;
  _stepper.moveTo(_openedPos);
}

void Irrigation::fermer() {
  _state = IrrigationState::FERMETURE;
  _stepper.moveTo(_closedPos);
}

void Irrigation::stopUrgence() {
  _stepper.stop();
  _state = IrrigationState::ARRET;
}

void Irrigation::update() {
  if (_state != IrrigationState::ARRET) {
    _stepper.run();
  }

  if (*_clickFlag == true) {
    *_clickFlag = false;

    if (_state == IrrigationState::OUVERTURE || _state == IrrigationState::FERMETURE) {
      stopUrgence();
      return;
    }

    if (_state == IrrigationState::ARRET) {
      ouvrir();
      return;
    }
  }

  if (_distance == nullptr) {
    return;
  }

  int dist = *_distance;
  int pct = getPositionPct();

  if (_premiereFermeture == true) {
    fermer();

    if (_stepper.distanceToGo() == 0) {
      _premiereFermeture = false;
      _state = IrrigationState::FERME;
    }

    ledBlinkTask();
    return;
  }

  if (_state == IrrigationState::OUVERTURE) {
    if (dist >= 25) {
      fermer();
      return;
    }

    if (_stepper.distanceToGo() == 0) {
      _state = IrrigationState::OUVERT;
    }

    ledBlinkTask();
    return;
  }

  if (_state == IrrigationState::FERMETURE) {
    if (dist > 0 && dist < 20) {
      ouvrir();
      return;
    }

    if (_stepper.distanceToGo() == 0) {
      _state = IrrigationState::FERME;
    }

    ledBlinkTask();
    return;
  }

  if (_state == IrrigationState::OUVERT || _state == IrrigationState::FERME) {
    if (dist > 0 && dist < 20 && pct < 100) {
      ouvrir();
      return;
    }

    if (dist >= 25 && pct > 0) {
      fermer();
      return;
    }
  }

  if (_state == IrrigationState::ARRET) {
    digitalWrite(_ledPin, LOW);
  }
}

void Irrigation::ledBlinkTask() {
  unsigned long currentTime = millis();

  if (currentTime - _lastBlinkTime >= _blinkDelay) {
    _lastBlinkTime = currentTime;

    _ledState = !_ledState;
    digitalWrite(_ledPin, _ledState);
  }
}