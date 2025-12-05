#include "stm32f7xx_hal.h" //importante trocar para o codigo do stm o qual esta sendo utilizado, neste caso e o stm32f746ZG portanto a biblioteca utilizada foi essa
#include "wizchip_conf.h"
#include "stdio.h"
#include "gpio.h"

extern SPI_HandleTypeDef hspi1;

// Aqui complementamos funcoes as quais faltam para a operação do W5500 disponibilizado pela WizChip, pois sem essas funcoes o adaptador não funciona corretamente

uint8_t SPIReadWrite(uint8_t data)
{
	//wait till FIFO has a free slot
	while((hspi1.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE);

	*(__IO uint8_t*)&hspi1.Instance->DR=data;

	//Now wait till data arrives
	while((hspi1.Instance->SR & SPI_FLAG_RXNE)!=SPI_FLAG_RXNE);

	return (*(__IO uint8_t*)&hspi1.Instance->DR);
}

void  wizchip_select(void)
{
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET); //ativa o chip select, as portas estao como SPI_CS pois foi o nome dado para o pino na inicializacao
}

void  wizchip_deselect(void)
{
	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET); //desativa o chip select
}

uint8_t wizchip_read()
{
	uint8_t rb;
	rb=SPIReadWrite(0x00);
	return rb;
}

void  wizchip_write(uint8_t wb)
{
	SPIReadWrite(wb);
}

void wizchip_readburst(uint8_t* pBuf, uint16_t len)
{
	for(uint16_t i=0;i<len;i++)
	{
		*pBuf=SPIReadWrite(0x00);
		pBuf++;
	}
}

void  wizchip_writeburst(uint8_t* pBuf, uint16_t len)
{
	for(uint16_t i=0;i<len;i++)
	{
		SPIReadWrite(*pBuf);
		pBuf++;
	}
}


void W5500Init()
{
	uint8_t tmp;
	uint8_t memsize[2][8] = { { 2, 2, 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2, 2, 2 } }; //alocacao de memoria do adaptador

	HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);//CS high by default

	//Send a pulse on reset pin
	HAL_GPIO_WritePin(SPI_RST_GPIO_Port, SPI_RST_Pin, GPIO_PIN_RESET);
	tmp = 0xFF;
	while(tmp--);
	HAL_GPIO_WritePin(SPI_RST_GPIO_Port, SPI_RST_Pin, GPIO_PIN_SET);

	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
	reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst);

	/* WIZChip Initialize*/
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1) {
		printf("WIZCHIP Initialized Failed.\r\n");
		while (1);
	}
	printf("WIZCHIP Initialization Success.\r\n");
/*
	do {
		if (ctlwizchip(CW_GET_PHYLINK, (void*) &tmp) == -1)
			printf("Unknown PHY Link stauts.\r\n");
	} while (tmp == PHY_LINK_OFF);
*/
}

