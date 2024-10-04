// Wokwi Custom Chip -- stepper-esc.chip.c
// Work-in-process demo at 
// https://wokwi.com/projects/410499111488041985
// This is intended to simulate an Electronic Speed Controller
// to translate paired INA & INB PWM signals into sequencing
// commutating quadrature signals for a stepper
//
// Inputs:
// IN1 -- PWM duty cycle signal for Forward
// IN2 -- PWM duty cycle signal for reverse polarity
// Vmot -- incompletely implemented
//
// Outputs 
// A-, A+, B+, B- -- quadrature stepper signals
// Vout -- an analog voltage representing fraction of 100%
//
// Controls: 
// Tau and Tau_exponent -- to set a smoothing time constant in seconds
// MaxSpeedRPM -- full speed PRM
// OverrideVoltage -- only for debugging and development
//
// Discord: https://discord.com/channels/787627282663211009/887330958875967520/1290688230223642646
// 
// SPDX-License-Identifier: MIT
// Copyright 2023 David Forrest forrbin/DaveX

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const bool VERBOSE = false;
const bool OVERRIDE_VOLTAGE_FOR_DEBUGGING = false;
uint32_t intervalUs = 100; // microseconds

static void chip_timer_eventUI(void *user_data);
//static void chip_timer_eventStep(void *user_data);

const uint8_t stepTable[][4] = {
  //A-,A+,B+,B-
  {1,0,1,0},
  {0,1,1,0},
  {0,1,0,1},
  {1,0,0,1}
};
 
typedef struct {
  pin_t pin_inA, pin_inB;
  pin_t pin_Ap, pin_Am, pin_Bp, pin_Bm;
  pin_t pin_Vmot, pin_Vout;
  uint32_t Tau_attr, Texp_attr;
  uint32_t MaxSpeed_attr;
  uint32_t OverrideVoltage_attr; // for debugging 
  float overrideVoltage; // for debugging
  uint64_t lastStepUs;
  float Tau,Texp;
  float Speed;
  uint32_t minStepInterval, stepInterval; 
  float inVoltage;
  float voltage, lastVoltage; //internal state 
  float tau, frac;
  uint8_t step;
  timer_t ui_timer, step_timer;

} chip_state_t;

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pin_inA = pin_init("INA", INPUT);
  chip->pin_inB = pin_init("INB", INPUT);
  chip->pin_Ap = pin_init("A+", OUTPUT);
  chip->pin_Am = pin_init("A-", OUTPUT);
  chip->pin_Bp = pin_init("B+", OUTPUT);
  chip->pin_Bm = pin_init("B-", OUTPUT);
  chip->pin_Vout = pin_init("Vout", ANALOG);
  chip->pin_Vmot = pin_init("VMOT", ANALOG);
  chip->Tau_attr = attr_init_float("Tau", 10.0);
  chip->Texp_attr = attr_init_float("Texp", -1.0);
  chip->MaxSpeed_attr = attr_init_float("MaxSpeed", 60);
  chip->lastStepUs = get_sim_nanos()/1000;
  chip->OverrideVoltage_attr = attr_init_float("OverrideVoltage", 0);

  const timer_config_t timer_config_ui = {
    .callback = chip_timer_eventUI,
    .user_data = chip,
  };
 // const timer_config_t timer_config_step = {
 //   .callback = chip_timer_eventStep,
 //   .user_data = chip,
 // };
  
  chip->ui_timer = timer_init(&timer_config_ui);
//  chip->step_timer = timer_init(&timer_config_step);
  timer_start(chip->ui_timer, intervalUs, true); // us
  chip->stepInterval = 1000000;
  //->voltage = 0;
 // timer_start(chip->step_timer, intervalUs, true); // us
 // printf("Hello from custom chip!\n");
}


void step(void *user_data, int val){
  chip_state_t *chip = (chip_state_t*)user_data;
  chip->step += val;
  while (chip->step > 3) chip->step -=4;
  while( chip->step < 0) chip->step += 4;

  pin_write(chip->pin_Ap, stepTable[chip->step][0]);
  pin_write(chip->pin_Am, stepTable[chip->step][1]);
  pin_write(chip->pin_Bp, stepTable[chip->step][2]);
  pin_write(chip->pin_Bm, stepTable[chip->step][3]);
}

//void chip_timer_eventStep(void *user_data) {
 // chip_state_t *chip = (chip_state_t*)user_data;
//  static uint32_t count = 0;
  //if(chip->voltage > 0 ) step(chip,1);
  //if(chip->voltage < 0 ) step(chip,-1);
  //   if(VERBOSE){
  // ;//   printf("step:%i rpm\n", chip->step);
  //  }
//}

void chip_timer_eventUI(void *user_data) {
  static uint32_t count = 0;
  chip_state_t *chip = (chip_state_t*)user_data;
  uint64_t nowUs = get_sim_nanos()/1000;

  float Tau = attr_read_float(chip->Tau_attr);
  float Texp = attr_read_float(chip->Texp_attr);
  float SpeedMax = attr_read_float(chip->MaxSpeed_attr);
  chip->overrideVoltage = attr_read_float(chip->OverrideVoltage_attr);
  chip->inVoltage = pin_adc_read(chip->pin_Vmot); 
  int valA = pin_read(chip->pin_inA); 
  int valB = pin_read(chip->pin_inB); 


  if( Tau != chip->Tau
    || Texp != chip->Texp ){
     chip->Tau = Tau;
    chip->Texp = Texp;
    chip->tau = Tau*pow(10,Texp); 
    float t_tau = intervalUs*1.0e-6 / chip->tau;
    chip->frac = 1-exp(-t_tau);
    if(VERBOSE){
      printf("xTau:%f s", chip->tau);
      // printf(" Texp:%f ", chip->Texp);
      printf(" t/tau: %f",t_tau);
      printf(" exp(-t/tau): %f",chip->frac);
      printf(" %f %f V\n", chip->inVoltage, chip->voltage);
    }
  }
  //if(false && (isnan(chip->voltage) || chip->voltage > 5 || chip->voltage < 5)){
  //  chip->voltage=0;  // chip->inVoltage;
  //}  

  if(chip->tau > 1e-8){
    if(valA>valB){
      float delta = (5.0 - chip->voltage) * chip->frac;
      chip->voltage += delta;
     // chip->voltage+= (5.0*valA - chip->voltage) * chip->frac;
//printf("A: %f %f; ",delta, chip->voltage );
    } 
    else if (valA < valB){ // drive negative
      chip->voltage+= (-5.0*valB -chip->voltage) * chip->frac;
  //    step(chip, -1);
    } else {
      float delta = (0.0 - chip->voltage) * chip->frac;
      chip->voltage += delta;
//      printf("0 %f ",chip->voltage);
    }
    if(OVERRIDE_VOLTAGE_FOR_DEBUGGING){
       chip->voltage = chip->overrideVoltage; // for debugging
    }
    if( VERBOSE && count++ > 1000){
      printf("Tau:%f %f %f V\n", chip->tau,chip->inVoltage,chip->voltage);
      count = 0;
    }
    if(chip->voltage != chip->lastVoltage || SpeedMax != chip->Speed ){ // update step interval
      chip->lastVoltage = chip->voltage;
      chip->Speed = SpeedMax;

      // RPM-> us/step = 1000000us/sec * 60s/min * 1Rev/200step *1/(Rev/min) *1/(maxRPM * (voltage/5))
      float rpm = SpeedMax * fabs(chip->voltage)/5.0;
      if (rpm > 0.0001){
      chip->stepInterval = 1000000.0* 60/200 / rpm;
       } else { // too slow
        chip->stepInterval = 1000000*60/200 * 1000;
       }
    }
  } else {
   ;// chip->voltage = chip->inVoltage;
  }
  if(nowUs - chip->lastStepUs > chip->stepInterval){
    chip->lastStepUs = nowUs;
    if(chip->voltage > 0 ) step(chip,1);
    if(chip->voltage < 0 ) step(chip,-1);
  }
  pin_dac_write(chip->pin_Vout, fabs(chip->voltage));
}
