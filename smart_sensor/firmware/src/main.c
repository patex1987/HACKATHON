#include "stm32l1xx.h"
#include "stm32l100c_discovery.h"
#include "xprintf.h"
			
#define FCLK_SLOW() { SPIx_CR1 = (SPIx_CR1 & ~0x38) | 0x18; }	/* Set SCLK = PCLK / 16 */
#define SPIx_CR1	SPI1->CR1
#define SPIx_SR		SPI1->SR
#define SPIx_DR		SPI1->DR
#define CS_HIGH()	GPIOA->ODR |= (1<<4)
#define CS_LOW()	GPIOA->ODR &= ~(1<<4)
#define RST_HIGH()	GPIOA->ODR |= (1<<2)
#define RST_LOW()	GPIOA->ODR &= ~(1<<2)

#define LED_HIGH()	GPIOC->ODR |= (1<<9)
#define LED_LOW()	GPIOC->ODR &= ~(1<<9)
#define LEDB_HIGH()	GPIOC->ODR |= (1<<8)
#define LEDB_LOW()	GPIOC->ODR &= ~(1<<8)

void delay (int a);
static uint8_t xchg_spi (uint8_t dat);
void usart_str (char * str);
uint8_t buffer_a_ready (void);
void tx_curr (void);
void tx_tran (void);

/* PA4 - CS
 * PA5 - SCK
 * PA6 - MISO
 * PA7 - MOSI
 * PA3 - DR
 * PA2 - NRST
 *
 */


#define		BUFF_SIZE	130
#define		TBUFF_SIZE	800

int16_t	buff_u_a[BUFF_SIZE],buff_i_a[BUFF_SIZE];
int16_t	buff_u_b[BUFF_SIZE],buff_i_b[BUFF_SIZE];
int16_t	buff_u_c[BUFF_SIZE],buff_i_c[BUFF_SIZE];
int16_t	buff_u_t[TBUFF_SIZE],buff_i_t[TBUFF_SIZE];
int16_t	buff_u_t_tmp[TBUFF_SIZE],buff_i_t_tmp[TBUFF_SIZE];
uint16_t	buff_a_ptr=0,buff_b_ptr=0,buff_t_ptr=0;
uint8_t buff_a_active=1,t_log_running=0;
uint16_t tran_num,tran_num_tmp;


#define	CBUF_SIZE	20
int16_t cbuf[CBUF_SIZE],min,max;
int32_t diff;
uint16_t cbuf_p=0;
volatile uint8_t rx_flag,rx_data;

uint16_t i;
int8_t tx_str[50];

int main(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->MODER &= ~   (1 << ((2*5)+0));
    GPIOA->MODER |=     (1 << ((2*5)+1));
    GPIOA->MODER &= ~   (1 << ((2*6)+0));
    GPIOA->MODER |=     (1 << ((2*6)+1));
    GPIOA->MODER &= ~   (1 << ((2*7)+0));
    GPIOA->MODER |=     (1 << ((2*7)+1));


    GPIOA->MODER |=     (1 << ((2*2)+0));
    GPIOA->MODER &= ~   (1 << ((2*2)+1));
    GPIOA->MODER |=     (1 << ((2*4)+0));
    GPIOA->MODER &= ~   (1 << ((2*4)+1));

    GPIOC->MODER |=     (1 << ((2*9)+0));
    GPIOC->MODER &= ~   (1 << ((2*9)+1));
    GPIOC->MODER |=     (1 << ((2*8)+0));
    GPIOC->MODER &= ~   (1 << ((2*8)+1));



    GPIOA->AFR[0] &= ~  (0x0F << (4*5));     //5
    GPIOA->AFR[0] |=    (0x05 << (4*5));
    GPIOA->AFR[0] &= ~  (0x0F << (4*6));     //5
    GPIOA->AFR[0] |=    (0x05 << (4*6));
    GPIOA->AFR[0] &= ~  (0x0F << (4*7));     //5
    GPIOA->AFR[0] |=    (0x05 << (4*7));



    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR5_0 | GPIO_OSPEEDER_OSPEEDR5_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_0 | GPIO_OSPEEDER_OSPEEDR6_1;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR7_0 | GPIO_OSPEEDER_OSPEEDR7_1;

    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4_0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR5_0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR6_0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR7_0;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    EXTI->IMR = (0x01<<3);
    EXTI->FTSR = (0x01<<3);

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    GPIOB->MODER &= ~   (1 << ((2*10)+0));
    GPIOB->MODER |=     (1 << ((2*10)+1));
    GPIOB->MODER &= ~   (1 << ((2*11)+0));
    GPIOB->MODER |=     (1 << ((2*11)+1));
    GPIOB->AFR[1] &= ~  (0x0F << (4*(10-8)));     //7
    GPIOB->AFR[1] |=    (0x07 << (4*(10-8)));
    GPIOB->AFR[1] &= ~  (0x0F << (4*(11-8)));     //7
    GPIOB->AFR[1] |=    (0x07 << (4*(11-8)));
    USART3->CR1 = USART3->CR1 | USART_CR1_UE ;
    USART3->BRR = (52*16)+0;
    USART3->CR1 = USART3->CR1 | USART_CR1_RE;
    USART3->CR1 = USART3->CR1 | USART_CR1_TE;
    USART3->CR1 = USART3->CR1 | USART_CR1_RXNEIE ;
    NVIC_EnableIRQ(USART3_IRQn);


    SPI1->CR1 = 0x0354;
	FCLK_SLOW();
	CS_HIGH();
	RST_LOW();
	delay(10000);
	RST_HIGH();
	delay(10000);
	CS_LOW();
	xchg_spi(0x00 | (0x0A<<1));
	xchg_spi(0xB0);
	CS_HIGH();
	delay(10000);

	CS_LOW();
	xchg_spi(0x00 | (0x08<<1));
	xchg_spi(0xA0);
	CS_HIGH();
	delay(10000);

	NVIC_EnableIRQ(EXTI3_IRQn);

	while (1)
	{
	if (rx_flag)
		{
		rx_flag = 0;
		if (rx_data=='c')
			tx_curr();
		if (rx_data=='t')
			tx_tran();
		if (rx_data=='i')
			{
			usart_str("version 1");
			}
		}
	}
}


void tx_tran (void)
{
	NVIC_DisableIRQ(EXTI3_IRQn);
	for (i=0;i<TBUFF_SIZE;i++)
	{
		buff_u_t_tmp[i] = buff_u_t[i];
		buff_i_t_tmp[i] = buff_i_t[i];
	}
	tran_num_tmp = tran_num;
	NVIC_EnableIRQ(EXTI3_IRQn);

	xsprintf(tx_str,"%6d,%6d,",TBUFF_SIZE,tran_num_tmp);
	usart_str(tx_str);

	for (i=0;i<TBUFF_SIZE;i++)
	{
		xsprintf(tx_str,"%6d,",buff_u_t_tmp[i]);
		usart_str(tx_str);
	}
	for (i=0;i<TBUFF_SIZE;i++)
	{
		xsprintf(tx_str,"%6d,",buff_i_t_tmp[i]);
		usart_str(tx_str);
	}

}

void tx_curr (void)
{
	xsprintf(tx_str,"%6d,",BUFF_SIZE);
	usart_str(tx_str);
	LEDB_HIGH();
	if (buffer_a_ready())
	{

		NVIC_DisableIRQ(EXTI3_IRQn);
		for (i=0;i<BUFF_SIZE;i++)
		{
			buff_u_c[i] = buff_u_a[i];
			buff_i_c[i] = buff_i_a[i];
		}
		NVIC_EnableIRQ(EXTI3_IRQn);
	}
	else
	{

		NVIC_DisableIRQ(EXTI3_IRQn);
		for (i=0;i<BUFF_SIZE;i++)
		{
			buff_u_c[i] = buff_u_b[i];
			buff_i_c[i] = buff_i_b[i];
		}
		NVIC_EnableIRQ(EXTI3_IRQn);
	}
	for (i=0;i<BUFF_SIZE;i++)
	{
		xsprintf(tx_str,"%6d,",buff_u_c[i]);
		usart_str(tx_str);
	}
	for (i=0;i<BUFF_SIZE;i++)
	{
		xsprintf(tx_str,"%6d,",buff_i_c[i]);
		usart_str(tx_str);
	}
	LEDB_LOW();
}


void delay (int a)
{
    volatile int i,j;
    for (i=0 ; i < a ; i++)
    {
        j++;
    }
}


static uint8_t xchg_spi (uint8_t dat)
{
	SPIx_DR = dat;
//	while (SPIx_SR & _BV(7)) ;
    while (((SPI1->SR)&0x001)==0);
	return (uint8_t)SPIx_DR;
}

uint8_t buffer_a_ready (void)
{
	uint8_t buff_a_active_copy;
	NVIC_DisableIRQ(EXTI3_IRQn);

	if (buff_a_active)
		{
		buff_a_active_copy = 0;
		}
	else
		{
		buff_a_active_copy = 1;
		}
	NVIC_EnableIRQ(EXTI3_IRQn);
	return buff_a_active_copy;
}



void EXTI3_IRQHandler (void)
{
	uint16_t d1,d2,u,i,j;
	EXTI->PR = EXTI->PR |((0x01<<3));
	CS_LOW();
	xchg_spi(0x01 | (0x00<<1));
	d1 = xchg_spi(0x00);
	d2 = xchg_spi(0x00);
	CS_HIGH();
	u = d1*256 + d2;
	CS_LOW();
	xchg_spi(0x01 | (0x03<<1));
	d1 = xchg_spi(0x00);
	d2 = xchg_spi(0x00);
	CS_HIGH();
	i = d1*256 + d2;

	cbuf[cbuf_p] = i;
	if (cbuf[cbuf_p])
	cbuf_p++;
	if (cbuf_p>CBUF_SIZE)
		{
		cbuf_p = 0;
		min = 32767;
		max = -32767;
		for (j=0;j<CBUF_SIZE;j++)
			{
			if (cbuf[j]<min) min = cbuf[j];
			if (cbuf[j]>max) max = cbuf[j];
			diff = max-min;
			}
		}
	if (diff>500)
		{
		if (t_log_running==0) t_log_running = 1;
		}
	else
		{
		if (t_log_running==2) t_log_running = 0;
		}
	if (t_log_running==1)
		{
		LED_HIGH();
		buff_u_t[buff_t_ptr] = u;
		buff_i_t[buff_t_ptr] = i;
		buff_t_ptr++;
		if (buff_t_ptr>TBUFF_SIZE)
			{
			t_log_running = 2;
			buff_t_ptr = 0;
			tran_num++;
			}
		}
	else
		{
		LED_LOW();
		}
	if (buff_a_active)
		{
		buff_u_a[buff_a_ptr] = u;
		buff_i_a[buff_a_ptr] = i;
		buff_a_ptr++;
		if (buff_a_ptr>=BUFF_SIZE)
			{
			buff_a_ptr = 0;
			buff_a_active = 0;
			}
		}
	else
		{
		buff_u_b[buff_b_ptr] = u;
		buff_i_b[buff_b_ptr] = i;
		buff_b_ptr++;
		if (buff_b_ptr>=BUFF_SIZE)
			{
			buff_b_ptr = 0;
			buff_a_active = 1;
			}
		}
}

void USART3_IRQHandler(void)
{
unsigned char b;
if ((USART3->SR)&(1<<5))
{
    b = USART3->DR;
    rx_flag = 1;
    rx_data = b;
}
}


void usart_str (char * str)
{
while (*str)
    {
      USART3->DR = *str++;
      while (((USART3->SR)&(1<<7))==0);
    }
}


