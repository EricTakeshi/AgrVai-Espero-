#include "NonBlockFunctions.h"

void stimer_init(stimer* ptimer, uint32_t tlapse)
{
	ptimer->time_lapse = tlapse;
	ptimer->tick_initial = HAL_GetTick();
}

void stimer_reload(stimer* ptimer)
{
	ptimer->tick_initial = ptimer->tick_initial + ptimer->time_lapse;
}

void stimer_redefine(stimer* ptimer, uint32_t tlapse)
{
	ptimer->tick_initial = ptimer->tick_initial + ptimer->time_lapse;
	ptimer->time_lapse = tlapse;
}

typeTimerStatus stimer_is_off(stimer* ptimer)
{
	uint32_t Delta = HAL_GetTick() - ptimer->tick_initial;
	return (Delta >= ptimer->time_lapse);
}

void edge_detect_init(sbot* pbot,
	GPIO_TypeDef* GPIOx,
	uint16_t GPIO_Pin,
	typeEdge edge,
	uint32_t dbtimelapse)
{
	stimer_init(&(pbot->dbtimer), dbtimelapse);
	pbot->GPIOx = GPIOx;
	pbot->GPIO_Pin = GPIO_Pin;
	pbot->edge_detect_state = edge_detect;
	if (rise == edge)
	{
		pbot->ant = push;
		pbot->atu = push;
	}
	else
	{
		pbot->ant = release;
		pbot->atu = release;
	}
}
typeBoolStatus is_Rise_Edge(sbot* pbot)
{
	switch (pbot->edge_detect_state)
	{
	case edge_detect:
		pbot->ant = pbot->atu;
		pbot->atu = (typePushBottonState)HAL_GPIO_ReadPin(pbot->GPIOx, pbot->GPIO_Pin);
		if ((pbot->atu == push) && (pbot->ant == release))
		{
			pbot->edge_detect_state = filter_bounce;
			stimer_init(&(pbot->dbtimer), pbot->dbtimer.time_lapse);
		}
		break;
	case filter_bounce:
		if (true == stimer_is_off(&(pbot->dbtimer)))
		{
			pbot->atu = (typePushBottonState)HAL_GPIO_ReadPin(pbot->GPIOx, pbot->GPIO_Pin);
			pbot->edge_detect_state = edge_detect;
			return ((typeBoolStatus)pbot->atu);
		}		
		break;
	default:
		break;
	}
	return (false);
}

typeBoolStatus is_Fall_Edge(sbot* pbot)
{
	switch (pbot->edge_detect_state)
	{
	case edge_detect:
		pbot->ant = pbot->atu;
		pbot->atu = (typePushBottonState)HAL_GPIO_ReadPin(pbot->GPIOx, pbot->GPIO_Pin);
		if ((pbot->atu == release) && (pbot->ant == push))
		{
			pbot->edge_detect_state = filter_bounce;
			stimer_init(&(pbot->dbtimer), pbot->dbtimer.time_lapse);
		}
		break;
	case filter_bounce:
		if (true == stimer_is_off(&(pbot->dbtimer)))
		{
			pbot->atu = (typePushBottonState)HAL_GPIO_ReadPin(pbot->GPIOx, pbot->GPIO_Pin);
			pbot->edge_detect_state = edge_detect;
			return !((typeBoolStatus)pbot->atu);
		}		
		break;
	default:
		break;
	}
	return (false);
}

void blinker_init(sblinker* Blinker, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint32_t half_period)
{
	Blinker->GPIOx = GPIOx;
	Blinker->GPIO_Pin = GPIO_Pin;
	stimer_init(&(Blinker->blinktimer), half_period);
}

void blinker_function(sblinker* Blinker)
{
	if (true == stimer_is_off(&(Blinker->blinktimer)))
	{
		HAL_GPIO_TogglePin(Blinker->GPIOx, Blinker->GPIO_Pin);
		stimer_reload(&(Blinker->blinktimer));
	}
}

void pwm_init(spwm* PWM,
	GPIO_TypeDef* GPIOx,
	uint16_t GPIO_Pin, 
	uint16_t Nt,
	uint16_t Na)
{
	PWM->GPIOx = GPIOx;
	PWM->GPIO_Pin = GPIO_Pin;
	PWM->Nt = Nt;
	PWM->Na = Na;
	
	// Calcula Nb
	if (Nt > Na)
	{
		PWM->Nb = Nt - Na;
	}
	else
	{
		PWM->Nb = 0; // 100% PWM
	}
	
	// Inicializa a porta e o timer embutido
	if (PWM->Na > 0)
	{
		HAL_GPIO_WritePin(PWM->GPIOx, PWM->GPIO_Pin, GPIO_PIN_SET);
		stimer_init(&(PWM->pwmtimer), PWM->Na);
		PWM->alto = true;
	}
	else // 0% PWM
	{
		HAL_GPIO_WritePin(PWM->GPIOx, PWM->GPIO_Pin, GPIO_PIN_RESET);
		stimer_init(&(PWM->pwmtimer), PWM->Nb);
		PWM->alto = false;
	}
	
}

void pwm_function(spwm* PWM)
{
	if (true == stimer_is_off(&(PWM->pwmtimer)))
	{
		if (PWM->alto)
		{
			if (0 == PWM->Nb) // 100% PWM
			{
				stimer_redefine(&(PWM->pwmtimer), PWM->Na);
			}
			else
			{
				HAL_GPIO_TogglePin(PWM->GPIOx, PWM->GPIO_Pin);
				stimer_redefine(&(PWM->pwmtimer), PWM->Nb);
				PWM->alto = false;
			}
		}
		else
		{
			if (0 == PWM->Na) // 0% PWM
			{
				stimer_redefine(&(PWM->pwmtimer), PWM->Nb);
			}
			else
			{
				HAL_GPIO_TogglePin(PWM->GPIOx, PWM->GPIO_Pin);
				stimer_redefine(&(PWM->pwmtimer), PWM->Na);
				PWM->alto = true;
			}
		}
	}
}

void pwm_redefine(spwm* PWM, uint16_t Nt, uint16_t Na)
{
	PWM->Nt = Nt;
	PWM->Na = Na;
	
	// Calcula Nb
	if (Nt > Na)
	{
		PWM->Nb = Nt - Na;
	}
	else
	{
		PWM->Nb = 0; // 100% PWM
	}
}