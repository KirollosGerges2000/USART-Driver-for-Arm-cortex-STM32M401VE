/*
******************************************************************************
File:     main.c
Info:     Generated by Atollic TrueSTUDIO(R) 9.3.0   2023-09-16

The MIT License (MIT)
Copyright (c) 2019 STMicroelectronics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

******************************************************************************
*/

/* Includes */
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

/* Private macro */
//AF PA9/PA10 pi masks
#define GPIO_AFRH_AFRH9             ((uint32_t) 0x000000F0)
#define GPIO_AFRH_AFRH9_AF7         ((uint32_t) 0x00000070)
#define GPIO_AFRH_AFRH10            ((uint32_t) 0x00000F00)
#define GPIO_AFRH_AFRH10_AF7        ((uint32_t) 0x00000700)


//Maximum USART reciption buffer length
#define MAX_BUFFER_LENGTH           ((uint32_t) 200u)

//USART1 states defination
typedef enum
{
	USART1_IDLE,
	USART1_WAIT_FOR_RESPONSE,
	USART1_ASK_FOR_NAME,
	USART1_WAIT_FOR_NAME,
	USART1_WAIT_FOR_COMMAND,
}USART1_StateType;


//USART1 IRQ status defination
typedef enum
{
	USART1_NO_IRQ,
	USART1_CHAR_RECIVED,
	USART1_PARITY_ERROR,
}USART1_IRQStatusType;

	//RETURN TYPE
typedef enum
{
STR_NOT_EQUAL,
STR_EQUAL
}strCmpReturnType;


/* Private variables */
/*
 *@Brief USART message to be transmitted.
 *

*/
static const char ask_for_name[]="What is your name ?";
static const char hello_world[]="Hello";
static const char parity_error[]="Parity Error";
static const char  hi[]="Hi,";
static const char ask_for_command[]="Please, send a command!!";
static const char ask_for_command_ex[]="Action[Turn on/Turn Off]";
static const char turn_on_green_led[]="turn_on_green_led";
static const char turn_on_red_led[]="turn_on_red_led";
static const char turn_off_green_led[]="turn_off_green_led";
static const char turn_off_red_led[]="turn_off_red_led";
static const  char done[]="Done";
static const  char wrong_command[]="wrong_command";
static const  char Button_on[]="Button activate";
static const char Button_off[]="Button disactivate";
//Decleration of states and statues cases
static USART1_IRQStatusType currentIRQStatus;
static USART1_StateType currentState;



//Global Declerations
static char RxChar=0;
//Global Rx Buffer decleration
static char RxBuffer[MAX_BUFFER_LENGTH];

//Global RxMessageLength decleration
static uint8_t RxMessageLength=0;

/* Private function prototypes */
void USART1_GPIO_Config(void);
void USART1_Init(void);
void USART1_Enable(void);
void USART1_process(void);
void USART1_IRQHandler(void);
void USART1_IRQ_Callback(void);
static void strTransmit(const char *str,uint8_t size);
const static char strReceive(const char *str,uint8_t size);
static strCmpReturnType strCmp (const char *str1 , const char *str2 ,const uint8_t size);
/* Private functions */

void USART1_GPIO_Config(void)
{
	// Enable clock A clock
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOAEN;

	// select alternative function
	GPIOA->MODER &=~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
	GPIOA->MODER |=(GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1);

	//select output type push_pull for Tx (PA(9)).
	GPIOA->OTYPER &=~(GPIO_OTYPER_OT_9);

	//select Output speed medium for Tx(PA9)
	GPIOA->OSPEEDR &=~( GPIO_OSPEEDER_OSPEEDR9);
	GPIOA->OSPEEDR |=( GPIO_OSPEEDER_OSPEEDR9_0);

	//Select Pull up
	GPIOA->PUPDR &=~(GPIO_PUPDR_PUPDR9  | GPIO_PUPDR_PUPDR10);
	GPIOA->PUPDR |=(GPIO_PUPDR_PUPDR9_0  | GPIO_PUPDR_PUPDR10_0);

	//Select AF7
	GPIOA->AFR[1]&=~(GPIO_AFRH_AFRH9 | GPIO_AFRH_AFRH10);
	GPIOA->AFR[1]|=(GPIO_AFRH_AFRH9_AF7 | GPIO_AFRH_AFRH10_AF7);

}


void USART1_Init(void)
{
  //Enable USART1 clock
	RCC->APB2ENR =RCC_APB2ENR_USART1EN;

	//select oversampling by 16 mode
	USART1->CR1&=~(USART_CR1_OVER8);

	//select on sample bit method
	USART1->CR3|=USART_CR3_ONEBIT;

//Select 1 start bit, 9 Data bits , n stop bits.
	USART1->CR1|=USART_CR1_M;

	//Select 1 stop Bit
	USART1->CR2&=~USART_CR2_STOP;

	//select parity control
	USART1->CR1|=USART_CR1_PCE ;

	//select odd parity
	USART1->CR1|=USART_CR1_PS ;

	/*
	 * set Baud rate 115200 Bps
	 * USARTDiv =Fck/(16*baud_rate)
	 * =9000000/(115200*16)=48.82
	 * fraction=16*0.82=13.12=13=0xD
	 * Mintesa=48=0x30
	 */
	USART1->BRR|=0x30D;
}

void USART1_Enable(void)
{
	//Enable USART
	USART1->CR1|=USART_CR1_UE;

	//Enable Transmiter interrupt
	USART1->CR1|=USART_CR1_TE;

	//Enable Reciver interrupt
	USART1->CR1|=USART_CR1_RE;

	//Enable reciption buffer not empty flag interrupt
	USART1->CR1|=USART_CR1_RXNEIE;

	//Enable Parity error interrupt
	USART1->CR1|=USART_CR1_PEIE ;
}



void USART1_process(void)
{
	//check error status
	switch(currentIRQStatus)
	{
	case USART1_PARITY_ERROR:
		//Transmit parity error
		strTransmit(parity_error,sizeof(parity_error));

		//Reset USART1 state
		currentState=USART1_IDLE;

		//Reset IRQ status
		currentIRQStatus =USART1_NO_IRQ;
		break;

	case USART1_CHAR_RECIVED:
			//Transmit parity error
//			strReceive();

			//Reset IRQ status
			currentIRQStatus =USART1_NO_IRQ;
			break;

	case USART1_NO_IRQ:
				break;

	default:
		break;
	}

	//Check current USART state
	switch(currentState)
	{
	case USART1_IDLE:
		//Transmit data
		strTransmit(hello_world,sizeof(hello_world));

		//Go to next state
		currentState= USART1_WAIT_FOR_RESPONSE;
				break;

	case USART1_WAIT_FOR_RESPONSE:
		// check if new messege transmit
		if(RxMessageLength!='\0')
		{
			//Reset message length
			RxMessageLength=0;

			//Go to next state
	currentState=USART1_ASK_FOR_NAME;
}
		else
		{
			//Nothing received Yet
		}
		break;

	case USART1_ASK_FOR_NAME:
		//Transmit data
		strTransmit(ask_for_name,sizeof(ask_for_name));

		//Go to the next state
		currentState=USART1_WAIT_FOR_NAME;
		break;

	case USART1_WAIT_FOR_NAME:
		//check if new message recived
		if(RxMessageLength!='\0')
		{
			//Transmit data
			strTransmit(hi,sizeof(hi));
			strTransmit(RxBuffer,RxMessageLength);
			strTransmit(ask_for_command,sizeof(ask_for_command));
			strTransmit(ask_for_command_ex,sizeof(ask_for_command_ex));

			//Reset message Length
			RxMessageLength=0;

			//Go to next state
			currentState=USART1_WAIT_FOR_COMMAND;

		}

		else
		{

			//For Nothing recived
		}
break;

	case USART1_WAIT_FOR_COMMAND:
//check for new messege received
		if(RxMessageLength!='\0')
		{
			//Reset message Length
						RxMessageLength=0;

						//String Compare result
strCmpReturnType isMatch_01=STR_NOT_EQUAL;
strCmpReturnType isMatch_02=STR_NOT_EQUAL;
strCmpReturnType isMatch_03=STR_NOT_EQUAL;
strCmpReturnType isMatch_04=STR_NOT_EQUAL;


//campare With turn on green le
isMatch_01=strCmp(turn_on_green_led,RxBuffer,sizeof(turn_on_green_led));
//check return status
if(isMatch_01==STR_EQUAL)
{
	//Turn On green LED

	//Transmit data
	strTransmit(done,sizeof(done));

}

else
{
	//campore with turn on red led command.

	isMatch_02=strCmp(turn_on_red_led,RxBuffer,sizeof(turn_on_red_led));
}

//check return type

if(isMatch_02==STR_EQUAL)
{
	//Turn on green led
	//Transmit data
		strTransmit(done,sizeof(done));
}
else if(isMatch_01==STR_NOT_EQUAL)
{
	isMatch_03=strCmp(turn_off_green_led,RxBuffer,sizeof(turn_off_green_led));
}

else
{
	//Do nothing
}


if(isMatch_03==STR_EQUAL)
{
	//Turn off green led
	//Transmit data
		strTransmit(done,sizeof(done));
}
else if(isMatch_01==STR_NOT_EQUAL && isMatch_02==STR_NOT_EQUAL)
{
	isMatch_04=strCmp(turn_off_red_led,RxBuffer,sizeof(turn_off_red_led));
}

else
{
	//Do nothing
}


if(isMatch_04==STR_EQUAL)
{
	//Turn off red led
	//Transmit data
		strTransmit(done,sizeof(done));
}
else if(isMatch_01==STR_NOT_EQUAL && isMatch_02==STR_NOT_EQUAL && isMatch_03==STR_NOT_EQUAL)
{
	//Transmit data
	strTransmit(wrong_command,sizeof(wrong_command));
}

else
{
	//Do nothing
}

		}

		else
		{

		}

break;

default:
	break;

	}
}
	void USART1_IRQHandler(void)
	{
		//Interrupt Handler
		USART1_IRQ_Callback();
	}


	void USART1_IRQ_Callback(void)
	{
		//call back process after sending message

		//check if parity error detected or not
		if((USART1->SR & USART_SR_PE) == USART_SR_PE)
		{
			//wait fir RXNE flag to be set
		while((USART1->SR & USART_SR_RXNE)!=USART_SR_RXNE);

		//Read data register
		USART1->DR;

		//set parity control
		currentIRQStatus= USART1_PARITY_ERROR;

		}

		else
		{
			// No parity error
		}

		//Check USART Reciver
		if((USART1->SR & USART_SR_RXNE) == USART_SR_RXNE )
				{
		//Read character
		RxChar=USART1->DR;

			//set IRQ statues
			currentIRQStatus=USART1_CHAR_RECIVED;
		}

		else
		{

//No new data Recived
		}


	}



	static void strTransmit(const char *str,uint8_t size)
	{

		//check null Pointers
	//	if(str!='\0')
		//{
//send all string charcters
			for(int idx=0;idx<size ;idx++)
			{
				//Check USART status register
				//wait for transimation buffer empty flag
				//while((USART1->SR & USART_SR_TXE));

//Write data into TX data register
			USART1->DR = str[idx];
			}




	//	else
	//	{
//NULL Pointers, do nothing
//		}
	}


	const static char strReceive(const char *str,uint8_t size)
	{

		for(int idx=0;idx<size ;idx++)
					{
						//Check USART status register
						//wait for transimation buffer empty flag
						//while((USART1->SR & USART_SR_TXE));

		//Write data into TX data register
					USART1->DR = str[idx];
					}

		return USART1->DR;
//Local string Buffer
		/*static char RxLocalBuffer[MAX_BUFFER_LENGTH];

		//current reception index
		static int RxIndex=0;

		//check if string data was recived before
		if(RxChar =='\0')
		{
			if(RxIndex!=0)
			{
				for(int idx=0;idx<RxIndex;idx++)
					{
				RxBuffer[idx]=RxLocalBuffer[idx];
			}
				//Add  terminating NULL at the end
				RxBuffer[RxIndex]=0;

				//Set message Length
				RxMessageLength=RxIndex+1;

//Reset Rxindex by 0
				RxIndex=0;

					}
			else
			{
				//String Buffer already empty
			}
		}

		else
		{

//character data was recived , check for buffer overflow
			if(MAX_BUFFER_LENGTH==RxIndex)
			{
				//Reset reciver buffer
				RxIndex=0;
			}
			else
			{
				// No thing for No overflow
			}

			//copy char data into string buffer
			RxLocalBuffer[RxIndex] =RxChar;

			//Increament current index for next char reciption
			RxIndex++;

		}*/
	}


static strCmpReturnType strCmp (const char *str1 , const char *str2 ,const uint8_t size)
		{
	//campore result
 strCmpReturnType cmpStatus =STR_EQUAL;

//Check Null pointers
if( str1!='\0' && str2!='\0' )
{
	//start camporing
	for(int idx=0 ;idx<size;idx++)
	{
//when not equal set return status
if(str1[idx]!=str2[idx])
{
	cmpStatus= STR_NOT_EQUAL;
}

else
{

	//Do nothing
}

	}

}
else
{
//For Null pointer Do nothing
}

	return cmpStatus ;
		}

/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/
int main(void)
{


  /**
  *  IMPORTANT NOTE!
  *  The symbol VECT_TAB_SRAM needs to be defined when building the project
  *  if code has been located to RAM and interrupts are used. 
  *  Otherwise the interrupt table located in flash will be used.
  *  See also the <system_*.c> file and how the SystemInit() function updates 
  *  SCB->VTOR register.  
  *  E.g.  SCB->VTOR = 0x20000000;  
  */

  /* TODO - Add your application code here */
	 USART1_GPIO_Config();
	 USART1_Init();
	USART1_Enable();



	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);


		     GPIO_InitTypeDef GPIO_InitDef_C;
		     	     GPIO_InitDef_C.GPIO_Pin = GPIO_Pin_13;
		     	     GPIO_InitDef_C.GPIO_Mode = GPIO_Mode_OUT;
		     	     GPIO_InitDef_C.GPIO_OType = GPIO_OType_PP;
		     	     GPIO_InitDef_C.GPIO_PuPd = GPIO_PuPd_NOPULL;
		     	     GPIO_InitDef_C.GPIO_Speed = GPIO_Speed_2MHz;




	         GPIO_Init(GPIOC, &GPIO_InitDef_C);

	      	     GPIO_ResetBits(GPIOC,GPIO_Pin_13);

	      		strReceive(Button_off, sizeof(Button_off));

  /* Infinite loop */
  while (1)
  {


if(strReceive(Button_on, sizeof(Button_on)))
		{
	GPIO_ToggleBits(GPIOC, GPIO_Pin_13);
	while(strReceive(Button_on, sizeof(Button_on)));

		}
else
{
//For Nothing sent

}

  }
  return 0;
}
