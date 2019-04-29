#include "Motor.h"
using Utils::print;
using Utils::LogLevel;

Motor::Motor() {
  throttle = 0;
  set_motor_state(false);
}

void Motor::enable_motors() {
  set_throttle(MOTOR_OFF);
  set_motor_state(true);
  print(LogLevel::LOG_DEBUG, "Motors Enabled\n");
}

void Motor::disable_motors() {
  set_throttle(MOTOR_OFF);
  set_motor_state(false);
  print(LogLevel::LOG_DEBUG, "Motors Disabled\n");
}

void Motor::set_motor_state(bool enable) {
  enabled = enable;
  #ifdef SIM
  SimulatorManager::sim.sim_motor_state(enable);
  #else
  SourceManager::CAN.set_motor_state(enable);
  #endif
}

bool Motor::is_enabled() {
  return enabled;
}

int16_t Motor::get_throttle() {
  return throttle;
}

void Motor::set_throttle(int16_t value) {
  if (enabled) {
    throttle = value;
    #ifdef SIM
    SimulatorManager::sim.sim_motor_set_throttle(value);
    #else
    SourceManager::CAN.set_motor_throttle(value);
    #endif
    
    print(LogLevel::LOG_DEBUG, "Setting motor throttle: %d\n", value);
  }
}

void Motor::set_relay_state(HV_Relay_Select relay, HV_Relay_State state) {
  #ifdef SIM
  SimulatorManager::sim.sim_relay_state(relay, state);
  #else
  SourceManager::CAN.set_relay_state(relay, state);
  #endif
}
