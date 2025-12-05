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
uint8_t minha_porta_local = 5000;
uint8_t snaux = 0;
uint8_t tmaux = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//funcoes enviadas para a serial
static void UWriteData(const char data);
static void PHYStatusCheck(void);
static void PrintPHYConf(void);
static void PrintServerCheck(void);



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
  __HAL_SPI_ENABLE(&hspi1);
  HAL_TIM_Base_Start_IT(&htim2);

  printf("My W5500 Application!\r\n");

   W5500Init();

   ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);//envia os dados do adapatador definidos anteriormente para inicialializacao

   PHYStatusCheck();//checa a presenca da conexao pelo cabo ethernet ate que haja uma conexao
   PrintPHYConf();//envia os dados da configuracao do adaptador

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

#if 0 //funcionando como client
  if(socket(sn, Sn_MR_TCP, 0, 0)==sn) //cria o socket como client
    {
  	  printf("\r\nSocket Created Successfully");
    }
    else
    {
  	  printf("\r\nCannot create socket");
  	  while(1);
    }
  PrintServerCheck();
#endif
#if 1

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	//Return value of the send() function is the amount of data sent
	switch(getSn_SR(sn)) // Lê o Status Register do SockeT
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
				// Força o IP aqui se necessário, ou trava
				break;
			}
			// 2. Chama o connect
			printf("Enviando SYN para %d.%d.%d.%d...\r\n", destination_ip[0], destination_ip[1], destination_ip[2], destination_ip[3]);
			int8_t ret = connect(sn, destination_ip, destination_port);
			// 3. Análise do Erro
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
			// DELAY CRUCIAL: Impede que o erro -4 aconteça em loop infinito
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
				// Sua lógica de envio/recebimento de dados entra aqui
				uint16_t tamanho = getSn_RX_RSR(sn); // Verifica dados recebidos
				if(tamanho > 0) {
					recv(sn, receive_buff, tamanho); // Lê os dados
					}
				snaux = 1;
				break;
			case SOCK_CLOSE_WAIT:
				// 4. O servidor mandou fechar a conexão (FIN).
				// Devemos desconectar e fechar o socket para reiniciar o ciclo.
				disconnect(sn);
				close(sn);
				snaux = 0;
				HAL_Delay(1000);
				break;
			default:
				// Tratamento de erros/timeouts
				// Se ficar travado em SYN_SENT (tentando conectar) por muito tempo, feche.
				break;
	}
#endif
#if 1
	  if(snaux == 1 && tmaux == 1)
	  {
		  HAL_GPIO_TogglePin(LD1G_GPIO_Port,LD1G_Pin);
		  if(send(sn, "Hello World!\r\n", 16)<=SOCK_ERROR) //envia o a mensagem para o servidor, sendo o segundo componente a mensagem e o terceiro o numero de caracteres
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
#endif
#if 0 //Funcionando como Server
	printf("\r\n*****************SIMPLE TCP ECHO SERVER******************\r\n");

	while(1)
	{
	  printf("\r\nInitializing server socket\r\n");
	  //Parameters in order socket_id, protocol TCP or UDP, Port number, Flags=0
	  //Return value is socket ID on success
	  if(sckt>7)//verifica se o valor do socket esta dentro dos valores existentes
	  {
	    //error
	    printf("Valor do Socket acima do permitido!\r\n");
	    while(1);//halt here
	  }
	  if(socket(sckt,Sn_MR_TCP,LISTEN_PORT,0)!=sckt)//definindo o socket como servidor
	  {
		  //error
		  printf("Cannot create Socket!\r\n");
		  while(1);//halt here
	  }

	  //success
	  printf("Socket Created Successfully ! \r\n");
	  uint8_t socket_io_mode=SOCK_IO_BLOCK;
	  ctlsocket(sckt, CS_SET_IOMODE , &socket_io_mode);//set blocking IO mode
	  printf("Start listening on port %d ! \r\n",LISTEN_PORT);
	  printf("Waiting for a client connection. \r\n");
		  //Make it a passive socket (i.e. listen for connection)
	  if(listen(sckt)!=SOCK_OK)//our socket id is 1 (w5500 have 8 sockets from 0-7)
	  {
		  //error
		  printf("Cannot listen on port %d",LISTEN_PORT);
			  while(1);
		  }
		  uint8_t sr=0x00;//socket status register
		  do
	  {
		  sr=getSn_SR(sckt);//read status reg (SR of socket 1)
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
			  int len=recv(sckt, receive_buff, RECEIVE_BUFF_SIZE); //cria uma variavel a qual recebe os dados do cliente
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
			  send(sckt,(uint8_t*)"[",1);//starting sq bracket
			  send(sckt,receive_buff,len);// the data
			  send(sckt,(uint8_t*)"]\r\n",5);//closing sq bracket

			  printf("\r\nECHO sent back to client\r\n");

			  //Look for quit message and quit if received
			  if(strcmp((char*)receive_buff,"QUIT")==0) //compara se a palavra recebida e a mesma escrita aqui, caso seja disconecta o cliente do servidor, lembrando que ocorre a diferenciacao das letras maiusculas e minusculas
			  {
				  printf("Received QUIT command from client\r\n");
				  printf("Disconnecting ... \r\n");
				  printf("*** SESSION OVER ***\r\n\r\n");
				  disconnect(sckt);//disconnect from the clinet
				  break;//come out of while loop
			  }
			  if(strcmp((char*)receive_buff,"LD1G")==0)//compara para acender o led verde da placa
			  {
				  HAL_GPIO_TogglePin(LD1G_GPIO_Port,LD1G_Pin);

				  if(HAL_GPIO_ReadPin(LD1G_GPIO_Port,LD1G_Pin) == GPIO_PIN_SET)
				  {
					  send(sckt,(uint8_t*)"LED Verde Ligado\r\n",20);
				  }
				  else
				  {
					  send(sckt,(uint8_t*)"LED Verde Desligado\r\n",23);
				  }
			  }
			  if(strcmp((char*)receive_buff,"LD2B")==0)//compara para acender o led azul da placa
			  {
				  HAL_GPIO_TogglePin(LD2B_GPIO_Port,LD2B_Pin);
				  if(HAL_GPIO_ReadPin(LD2B_GPIO_Port,LD2B_Pin) == GPIO_PIN_SET)
				  {
					  send(sckt,(uint8_t*)"LED Azul Ligado\r\n",19);
				  }
				  else
				  {
					  send(sckt,(uint8_t*)"LED Azul Desligado\r\n",22);
				  }
			  }
			  if(strcmp((char*)receive_buff,"LD3R")==0) //compara para acender o led vermelho da placa
			  {
				  HAL_GPIO_TogglePin(LD3R_GPIO_Port,LD3R_Pin);
				  if(HAL_GPIO_ReadPin(LD3R_GPIO_Port,LD3R_Pin) == GPIO_PIN_SET)
				  {
					  send(sckt,(uint8_t*)"LED Vermelho Ligado\r\n",23);
				  }
				  else
				  {
				 	  send(sckt,(uint8_t*)"LED Vermelho Desligado\r\n",26);
				  }
			  }

		  }//While loop (as long as client is connected)
	  }//if block, client connect success
  }//while loop for next client wait
#endif
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

void PrintServerCheck(void)
{

	printf("\r\nConnecting to server: %d.%d.%d.%d @ TCP Port: %d",destination_ip[0],destination_ip[1],destination_ip[2],destination_ip[3],destination_port);

	if(connect(sn, destination_ip, destination_port)==SOCK_OK) //busca a conexao com o server
	{
		printf("\r\nConnected with server.");
	}
	else
	{
		//failed
		printf("\r\nCannot connect with server!");
		printf("\r\nAbra o servidor e reinicialize o microcontrolador\r\n");
		while(1); //deixa o micro em loop eterno fazendo nada

	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // Verifica se quem chamou foi realmente o TIM2 (caso tenha outros timers)
    if (htim->Instance == TIM2)
    {
        // AÇÃO 1: Piscar um LED para debug visual (opcional)
        tmaux = 1;
        // AÇÃO 3: Contador global (opcional)
        // segundos_totais++;
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
