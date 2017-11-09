#ifndef BRAKE_PRESSURE_H
#define BRAKE_PRESSURE_H
#include "../Data.h"

struct Brake_Pressure_Raw {
	uint16_t brake_pressure;
};

void brake_pressure_parse(uint8_t * buffer, Arbitrary_Data raw);
#endif
