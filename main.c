/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> //incluido para o uso do setbuf
#include <string.h> //utilizado para o setcompare, possibilitando a comparacao com uma palavra recebida possibilitando controlar algo
//w5500 related
#include "w5500_spi.h"
#include "wizchip_conf.h"
#include "socket.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    LD1G = 0,
    LD2B,
	LD3R,
	SAIR,
	TEMP,
	DESCONHECIDO
} ComandosID;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//Define o endereco do adaptador, substitua pelos desejados ao ser utilizado
wiz_NetInfo gWIZNETINFO = {
		.mac = { 0x80, 0x34, 0x28, 0x74, 0xA5, 0xCB },//MSB - LSB
		.ip ={ 10, 0, 0, 102}, //os 3 primeiros valores devem ser iguais ao da rede alocada, o ultimo valor sera o endereco dentro dessa rede, em uma rede caseira com poucos aparelhos conectados, recomenda-se o uso de um valor acima de 100
		.sn = { 255, 255, 255, 0}, //subnet mask, deve ser o mesmo da sua rede, geralmente e 255.255.255.0
		.gw ={ 10, 0, 0, 1}, //gateway padrao quando conectado a um modem e o seu valor sao os 3 valores identicos ao da rede, mudando apenas o ultimo o qual tende a ser 1
		.dns = { 8, 8, 8, 8 },
		.dhcp = NETINFO_STATIC };

//The TCP Port on which we will listen
//Definicoes para o modo de servidor
#define LISTEN_PORT 5000  //define a porta que vai ser utilizada para o cliente se conectar no servidor
#define RECEIVE_BUFF_SIZE 128 //tamanho maximo de caracteres a ser recebido* (perguntar pra ter certeza)

//defines do TMP100
#define TMP100_ADDR        0x92

#define TMP100_REG_TEMP    0x00
#define TMP100_REG_CONFIG  0x01
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//Definicoes para o modo Client
uint8_t destination_ip[]={10, 0, 0, 230}; //ip para onde deseja enviar os dados, no caso o computador ou servidor que ira receber as informacoes, deve ser na mesma rede definida nas configuracoes acima
uint16_t destination_port = 2222; //define a porta na qual sera procurada quando conectado a rede

uint8_t receive_buff[RECEIVE_BUFF_SIZE];//to receive data from client
uint8_t sn = 1;//define o socket a ser utilizado podendo ser de 0-7
int minha_porta_local = 5000;
uint8_t snaux = 0;
uint8_t tmaux = 0;
float temperatura;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//funcoes enviadas para a serial
static void UWriteData(const char data);
static void PHYStatusCheck(void);
static void PrintPHYConf(void);
void TMP100_Init(void);
float TMP100_ReadTemp(void);
void Socket_Status(void);
void Modo_Client_Sensor_Temp(void);
void Modo_Server(void);
void I2C_Scanner(void);
void OnOff_Led_RedeCMD(void);
void Modo_Client(void);
ComandosID InterpretarComando(char* buffer);



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  setbuf(stdout, NULL); //para enviar a linha assim que for escrita sem bufferizacao
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(100);
  __HAL_SPI_ENABLE(&hspi1);
  HAL_TIM_Base_Start_IT(&htim2);

  printf("Inicializando W5500!\r\n");

   W5500Init();
   TMP100_Init();
   //I2C_Scanner();

   ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);//envia os dados do adapatador definidos anteriormente para inicialializacao

   PHYStatusCheck();//checa a presenca da conexao pelo cabo ethernet ate que haja uma conexao
   PrintPHYConf();//envia os dados da configuracao do adaptador

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	//Return value of the send() function is the amount of data sent
	Modo_Client();
	//Modo_Server();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void UWriteData(const char data)
{
	while(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_TXE)==RESET);

	huart2.Instance->TDR=data;

}

int __io_putchar(int ch)
{
	UWriteData(ch);
	return ch;
}


void PHYStatusCheck(void)
{
	uint8_t tmp;

	do
	{
		printf("\r\nChecking Ethernet Cable Presence ...");
		ctlwizchip(CW_GET_PHYLINK, (void*) &tmp);

		if(tmp == PHY_LINK_OFF)
		{
			printf("NO Cable Connected!");
			HAL_Delay(1000);
		}
	}while(tmp == PHY_LINK_OFF);

	printf("Good! Cable got connected!");

}

void PrintPHYConf(void)
{
	wiz_PhyConf phyconf;

	ctlwizchip(CW_GET_PHYCONF, (void*) &phyconf);

	if(phyconf.by==PHY_CONFBY_HW)
	{
		printf("\n\rPHY Configured by Hardware Pins");
	}
	else
	{
		printf("\n\rPHY Configured by Registers");
	}

	if(phyconf.mode==PHY_MODE_AUTONEGO)
	{
		printf("\n\rAutonegotiation Enabled");
	}
	else
	{
		printf("\n\rAutonegotiation NOT Enabled");
	}

	if(phyconf.duplex==PHY_DUPLEX_FULL)
	{
		printf("\n\rDuplex Mode: Full");
	}
	else
	{
		printf("\n\rDuplex Mode: Half");
	}

	if(phyconf.speed==PHY_SPEED_10)
	{
		printf("\n\rSpeed: 10Mbps");
	}
	else
	{
		printf("\n\rSpeed: 100Mbps");
	}
}


void Socket_Status(void)
{
	switch(getSn_SR(sn)) // Lê o Status Register do Socket
		{
		    case SOCK_CLOSED:
			// 1. Se o socket está FECHADO, precisamos abri-lo primeiro.
			// Se a conexão anterior falhou, o W5500 volta para cá automaticamente.
			printf("Socket Fechado. Reabrindo...\r\n");
			// Incrementa a porta para o servidor não confundir com a conexão anterior
			minha_porta_local++;
			if (minha_porta_local > 60000) minha_porta_local = 5000; // Reseta se ficar muito alto
			// Usa a porta variável
			if(socket(sn, Sn_MR_TCP, minha_porta_local, 0) == sn)
			{
				printf("Socket Aberto na porta local: %d. Estado -> INIT\r\n", minha_porta_local);
			}
			else
			{
				printf("Falha ao criar socket.\r\n");
			}
			break;
			case SOCK_INIT:
				if(destination_ip[0] == 0)
				{
					printf("ERRO: IP Zerado. Arrumando...\r\n");
					break;
				}
				printf("Enviando SYN para %d.%d.%d.%d...\r\n", destination_ip[0], destination_ip[1], destination_ip[2], destination_ip[3]);
				int8_t ret = connect(sn, destination_ip, destination_port);
				if (ret == SOCK_OK)
				{
					printf("Comando aceito. Aguardando resposta do servidor...\r\n");
				}
				else if (ret == -13) { // SOCKERR_TIMEOUT
					printf("ERRO -13: TIMEOUT! O PC nao respondeu. Verifique Firewall e Cabo.\r\n");
					close(sn); // Fecha para tentar de novo
					}
				else if (ret == -4) { // SOCKERR_SOCKCLOSED
					printf("ERRO -4: Socket Fechado! O comando socket() anterior falhou ou foi lento.\r\n");
					close(sn); // Reinicia o ciclo
					}
				else {
					printf("ERRO desconhecido: %d\r\n", ret);
					close(sn);
				}
				HAL_Delay(1000);
				break;
				case SOCK_ESTABLISHED:
					// 3. Conexão feita! O servidor aceitou.
					// Verifique se tem dados chegando
					if(getSn_IR(sn) & Sn_IR_CON) {
						setSn_IR(sn, Sn_IR_CON); // Limpa flag de interrupção de conexão
						// Pode acender um LED indicando conexão aqui
						}
					if(snaux == 0)
					{
						printf("\r\nConexao Realizada");
					}
					snaux = 1;
					break;
				case SOCK_CLOSE_WAIT:
					// Devemos desconectar e fechar o socket para reiniciar o ciclo.
					disconnect(sn);
					close(sn);
					snaux = 0;
					HAL_Delay(1000);
					break;
				default:
					// Tratamento de erros/timeouts
					break;
		}
}

void Modo_Client_Sensor_Temp(void)
{
if(snaux == 1 && tmaux == 1)
	  {
		  // Leitura do Sensor
		  float temperatura = TMP100_ReadTemp();
		  printf("Temperatura Sala: %.2f C\r\n", temperatura);
		  char msg_temp[50];
		  sprintf(msg_temp, "Temp: %.2f C\r\n", temperatura);
		  send(sn, (uint8_t*)msg_temp, strlen(msg_temp));

		  if((send(sn, (uint8_t*)msg_temp, strlen(msg_temp)))<=SOCK_ERROR) //envia o a mensagem para o servidor, sendo o segundo componente a mensagem e o terceiro o numero de caracteres
		  {
			  printf("\r\nSending Failed!");//envia que a mensagem nao foi enviada para a serial caso o servidor tenha sido fechado, gerando um erro no socket pois fechou a conexao
			  while(1);
		  }
		  else
		  {
			  printf("\r\nSending Success!"); //apensa um retorno pela serial que foi executado com sucesso o envio da mensagem
		  }
		  tmaux = 0;
	  }
}

void OnOff_Led_RedeCMD(void)
{
	if(snaux == 1)
	{
		int len=recv(sn, receive_buff, RECEIVE_BUFF_SIZE);
		if(len > 0)
		{
			switch (InterpretarComando(((char*) receive_buff)))
			{
			case LD1G:
					HAL_GPIO_TogglePin(LD1G_GPIO_Port,LD1G_Pin);
			break;
			case LD2B:
					HAL_GPIO_TogglePin(LD2B_GPIO_Port,LD2B_Pin);
			break;
			case LD3R:
					HAL_GPIO_TogglePin(LD3R_GPIO_Port,LD3R_Pin);
			break;
			default:
				send(sn,(uint8_t*)"Funcao Desconhecida\r\n",21);
			break;
			}
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
    	tmaux = 1;
    }
}

void Modo_Client(void)
{
	Socket_Status();
	Modo_Client_Sensor_Temp();
	OnOff_Led_RedeCMD();
}

void Modo_Server(void)
{
	printf("\r\nInitializing server socket\r\n");
	//Parameters in order socket_id, protocol TCP or UDP, Port number, Flags=0
	//Return value is socket ID on success
	if(sn>7)//verifica se o valor do socket esta dentro dos valores existentes
	{
		//error
		printf("Valor do Socket acima do permitido!\r\n");
		while(1);//halt here
	}
	if(socket(sn,Sn_MR_TCP,LISTEN_PORT,0)!=sn)//definindo o socket como servidor
	{
		//error
		printf("Cannot create Socket!\r\n");
		while(1);//halt here
	}
	 //success
	printf("Socket Created Successfully ! \r\n");
	uint8_t socket_io_mode=SOCK_IO_BLOCK;
	ctlsocket(sn, CS_SET_IOMODE , &socket_io_mode);//set blocking IO mode
	printf("Start listening on port %d ! \r\n",LISTEN_PORT);
	printf("Waiting for a client connection. \r\n");
	//Make it a passive socket (i.e. listen for connection)
	if(listen(sn)!=SOCK_OK)//our socket id is 1 (w5500 have 8 sockets from 0-7)
	{
		//error
		printf("Cannot listen on port %d",LISTEN_PORT);
		while(1);
	}
	uint8_t sr=0x00;//socket status register
	do
	{
		sr=getSn_SR(sn);//read status reg (SR of socket 1)
	}while (sr!=0x17 && sr!=0x00);
	if(sr==0x00)
	{
		printf("Some error occurred on server socket. Please restart.\r\n");
		while(1);
	}
	if(sr==0x17)
	{
		//we come here only when a client has connected.
		//Now we can read data from the socket
		printf("A client connected!\r\n");
		printf("Waiting for Client Data ...!\r\n");
		while(1)
		{
			int len=recv(sn, receive_buff, RECEIVE_BUFF_SIZE); //cria uma variavel a qual recebe os dados do cliente
			if(len==SOCKERR_SOCKSTATUS) //verifica se o cliente perdeu a conexao com o servidor
			{
				//client has disconnected
				printf("Client has disconnected\r\n");
				printf("*** SESSION OVER ***\r\n\r\n");
				break;
			}
			receive_buff[len]='\0'; //recebe os dados ate o final do desejado, onde \0 indica o fim dos dados
			printf("Received %d bytes from client\r\n",len);
			printf("Data Received: %s", receive_buff);
			//Echo the data back encloused in a [] pair
			send(sn,(uint8_t*)"[",1);//starting sq bracket
			send(sn,receive_buff,len);// the data
			send(sn,(uint8_t*)"]\r\n",5);//closing sq bracket
			if(len > 0)
			{

			}
			printf("\r\nECHO sent back to client\r\n");


			  }//While loop (as long as client is connected)
		  }//if block, client connect success
}

void Comandos_Servidor(char* buff)
{
	switch (InterpretarComando(((char*) receive_buff)))
	{
	case LD1G:
		HAL_GPIO_TogglePin(LD1G_GPIO_Port,LD1G_Pin);
		if(HAL_GPIO_ReadPin(LD1G_GPIO_Port,LD1G_Pin) == GPIO_PIN_SET)
		{
			send(sn,(uint8_t*)"LED Verde Ligado\r\n",20);
		}
		else
		{
			send(sn,(uint8_t*)"LED Verde Desligado\r\n",23);
		}
	break;
	case LD2B:
		HAL_GPIO_TogglePin(LD2B_GPIO_Port,LD2B_Pin);
		if(HAL_GPIO_ReadPin(LD2B_GPIO_Port,LD2B_Pin) == GPIO_PIN_SET)
		{
			send(sn,(uint8_t*)"LED Azul Ligado\r\n",19);
		}
		else
		{
			send(sn,(uint8_t*)"LED Azul Desligado\r\n",22);
		}
	break;
	case LD3R:
		HAL_GPIO_TogglePin(LD3R_GPIO_Port,LD3R_Pin);
		if(HAL_GPIO_ReadPin(LD3R_GPIO_Port,LD3R_Pin) == GPIO_PIN_SET)
		{
			send(sn,(uint8_t*)"LED Vermelho Ligado\r\n",23);
		}
		else
		{
			send(sn,(uint8_t*)"LED Vermelho Desligado\r\n",26);
		}
	break;
	case TEMP:
		float temperatura = TMP100_ReadTemp();
		char msg_temp[50];
		sprintf(msg_temp, "Temp: %.2f C\r\n", temperatura);
		send(sn, (uint8_t*)msg_temp, strlen(msg_temp));
		break;
		case SAIR:
			send(sn,(uint8_t*)"Sendo Desconectado\r\n",23);
			disconnect(sn);
		break;
		default:
			send(sn,(uint8_t*)"Funcao Desconhecida\r\n",21);
		break;
	}
}

ComandosID InterpretarComando(char* buffer) {
    // Compara o buffer com as palavras conhecidas
	if (strncmp(buffer, "LD1G", 4) == 0) return LD1G;
	if (strncmp(buffer, "LD2B", 4) == 0) return LD2B;
	if (strncmp(buffer, "LD3R", 4) == 0) return LD3R;
	if (strncmp(buffer, "SAIR", 4) == 0) return SAIR;
	if (strncmp(buffer, "TEMP", 4) == 0) return TEMP;

    return DESCONHECIDO;
}

//funções do TMP100
void TMP100_Init(void) {
    uint8_t data[2];

    // Seleciona o registrador de Configuração
    data[0] = TMP100_REG_CONFIG;

    // Configura para 12-bit resolution (Bits R1/R0 = 11)
    // Valor 0x60 = 0110 0000 em binário
    data[1] = 0x60;

    // Envia o comando
    if (HAL_I2C_Master_Transmit(&hi2c2, TMP100_ADDR, data, 2, 100) != HAL_OK) {
        printf("Erro ao inicializar TMP100\r\n");
    } else {
        printf("TMP100 Configurado para 12-bits.\r\n");
    }
}

float TMP100_ReadTemp(void) {
    uint8_t buffer[2];
    int16_t raw_temp;

    // 1. Aponta para o registrador de temperatura
    buffer[0] = TMP100_REG_TEMP;
    HAL_I2C_Master_Transmit(&hi2c2, TMP100_ADDR, buffer, 1, 100);

    // 2. Lê 2 bytes de dados
    if (HAL_I2C_Master_Receive(&hi2c2, TMP100_ADDR, buffer, 2, 100) == HAL_OK) {

        // 3. Combina os bytes
        // O TMP100 retorna 12 bits alinhados à esquerda dentro de 16 bits
        // Ex: [Temp Alta] [Temp Baixa]
        raw_temp = (buffer[0] << 8) | buffer[1];

        // Desloca 4 bits para a direita (para alinhar os 12 bits)
        raw_temp = raw_temp >> 4;

        // Multiplica pela resolução de 12 bits (0.0625)
        return raw_temp * 0.0625f;
    }
    else {
        printf("Erro de leitura I2C\r\n");
        return -999.0f; // Valor de erro
    }
}

void I2C_Scanner(void) {
    printf("Iniciando Scanner I2C...\r\n");
    HAL_StatusTypeDef result;
    uint8_t i;
    int dispositivosEncontrados = 0;

    for (i = 1; i < 128; i++) {
        /*
         * O HAL requer o endereço deslocado para a esquerda (i << 1).
         * Tentamos conectar 2 vezes (trials) com timeout de 10ms
         */
        result = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(i << 1), 2, 10);

        if (result == HAL_OK) {
            printf("Dispositivo encontrado no endereco: 0x%02X (HAL: 0x%02X)\r\n", i, (i << 1));
            dispositivosEncontrados++;
        }
    }

    if (dispositivosEncontrados == 0) {
        printf("Nenhum dispositivo I2C encontrado. Verifique conexoes/pull-ups.\r\n");
    } else {
        printf("Scan concluido.\r\n");
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
