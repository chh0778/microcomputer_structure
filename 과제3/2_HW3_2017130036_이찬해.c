/*
HW3: Ŀ���ڵ��Ǹű�(GPIO & GLCD)
����: ����ũ����ǻ�ͱ���
��米��: ������ ������
2017130036 ������
������: 2021.05.08
*/
#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
uint16_t KEY_Scan(void);

void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void Black(void);
void Sugar(void);

uint8_t Flag=0; 	//�����¸� ��Ÿ���� Flag(0: Coin SW������, 1: Ŀ���������� SW ������)

int main(void)
{
    _GPIO_Init(); 		                // GPIO (LED, SW, Buzzer) �ʱ�ȭ
    LCD_Init();		                        // LCD ��� �ʱ�ȭ
    DelayMS(100);

    GPIOG->ODR &= ~0x00FF;	                // LED �ʱⰪ: LED0~7 Off 
    DisplayInitScreen();	                // LCD �ʱ�ȭ��

    while(1)
    {
      switch(KEY_Scan())
      {
        case 0x7F00:                            //SW7 ON
          if(Flag==0){                            //Coin SW �������϶� ����
              GPIOG->BSRRL = 0x80;                         //coin LED(LED7) ON
              BEEP();                                             //Buzzer ON
              LCD_DisplayText(1,5,"O");                    //Coin mark ON
              Flag=1;                             //Ŀ������ SW�����·� ����
          }
          break;
          
        case 0xFE00:                    //SW0 ON
          if(Flag==1)                       //Ŀ���������� SW�������϶� ����
          {
            Black();                        //Black coffee ���� ���� �Լ�
            Flag=0;                       //Coin SW �����·� ����
          }
          break;
        
        case 0xFD00:                   //SW1 ON
          if(Flag==1)                     //Ŀ���������� SW�������϶� ����
          {
            Sugar();                      //Sugar coffee ���� ���� �Լ�
            Flag=0;                        //Coin SW �����·� ����
          }
          break;
      }//switch
    }//while(1)
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) �ʱ� ����*/
void _GPIO_Init(void)
{
      	// LED (GPIO G) ����
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) ���� 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) ����     
    	RCC->AHB1ENR	|=  0x00000020                  ; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

/* GLCD �ʱ�ȭ�� ���� �Լ� */
void DisplayInitScreen(void)
{
        LCD_Clear(RGB_YELLOW);		                           // ȭ�� Ŭ����
	LCD_SetFont(&Gulim8);		                           // ��Ʈ : ���� 8
        LCD_SetBackColor(RGB_YELLOW);                       //���� ����: YELLOW
	LCD_SetTextColor(RGB_BLUE);	                           // ���ڻ� : BLUE
	LCD_DisplayText(0,0,"Coffee Vendor");                   // Title : Coffee Vendor
        
        LCD_SetTextColor(RGB_BLACK);	                   // ���ڻ� : BLACK
        LCD_DisplayText(1,0,"Coin: ");                               // Text: Coin
        
        LCD_SetTextColor(RGB_RED);                      	  //���ڻ� : RED
        LCD_DisplayText(1,5,"X");                                       //Text: X
}

/* Switch�� �ԷµǾ������� ���ο� � switch�� �ԷµǾ������� ������ return�ϴ� �Լ�  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
        		return key;
      		else
		{	DelayMS(10);
        		key_flag = 0;
        		return key;
        	}
    	}
  	else				// if key input, check continuous key
	{	if(key_flag != 0)	// if continuous key, treat as no key input
        		return 0xFF00;
      		else			// if new key,delay for debounce
		{	key_flag = 1;
			DelayMS(10);
 			return key;
        	}
	}
}

/* Buzzer: Beep for 30 ms */
void BEEP(void)			
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

/*milli second delay*/
void DelayMS(unsigned short wMS)
{
    register unsigned short i;
    for (i=0; i<wMS; i++)
        DelayUS(1000);         		// 1000us => 1ms
}

/*micro second delay*/
void DelayUS(unsigned short wUS)
{
    volatile int Dly = (int)wUS*17;
    for(; Dly; Dly--);
}
/*coffee select action function*/
void Black(void){
      GPIOG->ODR |= 0x01;                         //Black coffee LED(LED0) ON
      LCD_SetTextColor(RGB_BLACK);                //���ڻ� : BLACK
      LCD_DisplayText(2,0,"Black-C: ");           //Ŀ�Ǹ� ǥ��
      BEEP();                                     //Buzzer ON
      DelayMS(1000);                              //1s delay
      GPIOG->ODR |= 0x08;                         //CUP LED (LED3) ON
      LCD_SetTextColor(RGB_RED);                  //���ڻ� : RED
      LCD_DisplayText(2,8,"U");                   //GLCD 'U'
      DelayMS(1000);                              //1s delay
      GPIOG->ODR &= ~0x08;                        //CUP LED (LED3) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'U' delete
      
      GPIOG->ODR |= 0x10;                         //CUP LED (LED4) ON
      LCD_DisplayText(2,8,"C");                   //GLCD 'C'
      DelayMS(2000);                              //2s delay
      GPIOG->ODR &= ~0x10;                        //CUP LED (LED4) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'C' delete
        
      GPIOG->ODR |= 0x40;                         //CUP LED (LED6) ON
      LCD_DisplayText(2,8,"W");                   //GLCD 'W'
      DelayMS(2000);                              //2s delay
      GPIOG->ODR &= ~0x40;                        //CUP LED (LED6) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'W' delete
      
      BEEP();                                     //Buzzer ON
      DelayMS(500);                               //0.5s delay
      BEEP();                                     //Buzzer ON
      
      DelayMS(1000);                              //1s delay
      LCD_DisplayText(2,0,"        ");            //'Black-C' delete
      GPIOG->ODR &= ~0x01;                        //Black coffee LED(LED0) OFF
      
      DelayMS(1000);                              //1s delay
      GPIOG->BSRRH = 0x80;                        //Coin LED(LED7) OFF
      LCD_DisplayText(1,5,"X");                   //Coin mark OFF
}

void Sugar(void){
      GPIOG->ODR |= 0x02;                         //Sugar coffee LED(LED1) ON
      LCD_SetTextColor(RGB_BLACK);                //���ڻ� : BLACK
      LCD_DisplayText(2,0,"Sugar-C: ");           //Ŀ�Ǹ� ǥ��
      BEEP();                                     //Buzzer ON
      DelayMS(1000);                              //1s delay
      GPIOG->ODR |= 0x08;                         //CUP LED (LED3) ON
      LCD_SetTextColor(RGB_RED);                  //���ڻ� : RED
      LCD_DisplayText(2,8,"U");                   //GLCD 'U'
      DelayMS(1000);                              //1s delay
      GPIOG->ODR &= ~0x08;                        //CUP LED (LED3) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'U' delete
     
      GPIOG->ODR |= 0x10;                         //CUP LED (LED4) ON
      LCD_DisplayText(2,8,"C");                   //GLCD 'C'
      DelayMS(2000);                              //2s delay
      GPIOG->ODR &= ~0x10;                        //CUP LED (LED4) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'C' delete
       
      GPIOG->ODR |= 0x20;                         //CUP LED (LED5) ON
      LCD_DisplayText(2,8,"S");                   //GLCD 'S'
      DelayMS(1000);                              //1s delay
      GPIOG->ODR &= ~0x20;                        //CUP LED (LED5) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'S' delete
      
      GPIOG->ODR |= 0x40;                         //CUP LED (LED6) ON
      LCD_DisplayText(2,8,"W");                   //GLCD 'W'
      DelayMS(2000);                              //2s delay
      GPIOG->ODR &= ~0x40;                        //CUP LED (LED6) OFF
      LCD_DisplayText(2,8," ");                   //GLCD 'W' delete
      
      BEEP();                                     //Buzzer ON
      DelayMS(500);                               //0.5s delay
      BEEP();                                     //Buzzer ON
      
      DelayMS(1000);                              //1s delay
      LCD_DisplayText(2,0,"        ");            //'Sugar-C' delete
      GPIOG->ODR &= ~0x02;                        //Sugar coffee LED(LED1) OFF
      
      DelayMS(1000);                              //1s delay
      GPIOG->BSRRH = 0x80;                        //Coin LED(LED7) OFF
      LCD_DisplayText(1,5,"X");                   //Coin mark OFF
}  
 