#include <Arduino.h>
#include "Substring.h"

Substring::Substring(int start, int end) {
  this->total = end - start + 1;
  this->start = start;
  this->buffer = malloc(sizeof(char) * (this->total + 1));
  this->buffer[this->total] = '\0';
}

Substring::~Substring() {
  free(this->buffer);
}

char* Substring::extract(char* src) {
  strncpy(this->buffer, src + this->start, this->total);

  return this->buffer;
}
