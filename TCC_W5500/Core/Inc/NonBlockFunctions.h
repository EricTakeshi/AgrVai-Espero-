#pragma once
#ifndef __NonBlockFunctions_h
#define __NonBlockFunctions_h

#include "gpio.h"

typedef enum
{
	false = 0,
	true
}typeBoolStatus;

typedef struct
{
	uint32_t tick_initial;
	uint32_t time_lapse;
}stimer;

typedef enum
{
	counting = 0,
	off
}typeTimerStatus;

typedef enum
{
	release = 0,
	push
}typePushBottonState;

typedef enum
{
	edge_detect   = 0,
	filter_bounce
}typeEdgeDetectState;

typedef enum
{
	fall = 0,
	rise
}typeEdge;

typedef struct
{
	typePushBottonState ant;
	typePushBottonState atu;
	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	stimer dbtimer;
	typeEdgeDetectState edge_detect_state;
}sbot;

typedef struct
{
	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	stimer blinktimer;
}sblinker;

typedef struct
{
	uint16_t Nt;
	uint16_t Na;
	uint16_t Nb;
	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	stimer pwmtimer;
	typeBoolStatus alto;
}spwm;


void stimer_init(stimer* ptimer, uint32_t tlapse);
void stimer_reload(stimer* ptimer);
void stimer_redefine(stimer* ptimer, uint32_t tlapse);
typeTimerStatus stimer_is_off(stimer* ptimer);
void edge_detect_init(sbot* pbot,
	GPIO_TypeDef* GPIOx,
	uint16_t GPIO_Pin,
	typeEdge edge,
	uint32_t dbtimelapse);
typeBoolStatus is_Rise_Edge(sbot* pbot);
typeBoolStatus is_Fall_Edge(sbot* pbot);

void blinker_init(sblinker* Blinker, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t half_period);
void blinker_function(sblinker* Blinker);

void pwm_init(spwm* PWM,
	GPIO_TypeDef* GPIOx,
	uint16_t GPIO_Pin, 
	uint16_t Nt,
	uint16_t Na);
void pwm_function(spwm* PWM);
void pwm_redefine(spwm* PWM, uint16_t Nt, uint16_t Na);

#endif
