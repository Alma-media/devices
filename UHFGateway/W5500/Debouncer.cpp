#include <Arduino.h>
#include "Debouncer.h"

Debouncer::Debouncer(RX_CALLBACK_SIGNATURE, uint16_t interval = DEFAULT_DEBOUNCE_INTERVAL) {
  this->last_ts  = millis();
  this->callback = callback;
  this->interval = interval;
}

Debouncer::~Debouncer() {}

Debouncer& Debouncer::setCallback(RX_CALLBACK_SIGNATURE) {
    this->callback = callback;
    return *this;
}

Debouncer& Debouncer::setInterval(uint16_t interval) {
    this->interval = interval;
    return *this;
}

void Debouncer::click(const unsigned long id) {
  if (this->last_id != id) {
    return this->_click(id);
  }

  if ((millis() - this->last_ts) > this->interval) {
    return this->_click(id);
  }
}

void Debouncer::_click(const uint32_t id) {
  this->last_id = id;
  this->last_ts = millis();

  const unsigned int BUFFER_LENGTH = 16;
  static char str_id[BUFFER_LENGTH];
  sprintf(str_id, "%lu", id);

  if (callback) {
    return callback(str_id);
  }

  Serial.println("no callback");
}
