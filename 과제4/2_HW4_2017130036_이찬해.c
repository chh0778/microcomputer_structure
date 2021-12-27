/*
HW3: ����������
����: ����ũ����ǻ�ͱ���
��米��: ������ ������
2017130036 ������
������: 2021.05.18
*/
#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

#define SW0    0xFE00  //PH8
#define SW1    0xFD00  //PH9
#define SW2    0xFB00  //PH10
#define SW3    0xF700  //PH11
#define SW4    0xEF00  //PH8
#define SW5    0xDF00  //PH9
#define SW6    0xBF00  //PH10
#define SW7    0x7F00  //PH11

#define LED0  0x0001    //PG0
#define LED1  0x0002    //PG1
#define LED2  0x0004    //PG2
#define LED3  0x0008    //PG3
#define LED4  0x0010    //PG4
#define LED5  0x0020    //PG5
#define LED6  0x0040    //PG6
#define LED7  0x0080    //PG7

void _GPIO_Init(void);                  
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
void BEEP(void);
void BEEP3(void); 

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t  SW0_Flag, SW7_Flag;            //���ͷ�Ʈ(EXTI8, 15) �ߺ� Ű�Է� ������ ���� Flag

uint16_t cur_led=0;                              //���� led �ּ����� (�ʱⰪ: 0)

int main(void)
{
	_GPIO_Init(); 	                 // GPIO (LED,SW,Buzzer)�ʱ�ȭ
        _EXTI_Init();                        //EXTI �ʱ�ȭ
	LCD_Init();	                 // LCD ��� �ʱ�ȭ
	DelayMS(10);

	Fram_Init();                           // FRAM �ʱ�ȭ H/W �ʱ�ȭ
	Fram_Status_Config();          // FRAM �ʱ�ȭ S/W �ʱ�ȭ
 
	DisplayInitScreen();            // LCD �ʱ�ȭ��  
        
//FRAM�̿��ؼ� ���� �� ������ ���� LED�� GLCD�� ǥ���ؾ��Ѵ�.
       switch(Fram_Read(36))
       {
       case 0:                                          //Cur_FL: 0
          LCD_DisplayText(1,8,"0");     
          cur_led=LED0;                       //cur_led�� LED0  �� ����
          break;
       case 1:                                          //Cur_FL: 1
         LCD_DisplayText(1,8,"1");
         cur_led=LED1;                          //cur_led�� LED1  �� ����
          break;
       case 2:                                          //Cur_FL: 2
         LCD_DisplayText(1,8,"2");
         cur_led=LED2;                          //cur_led�� LED2  �� ����
          break;
       case 3:                                           //Cur_FL: 3
         LCD_DisplayText(1,8,"3");
         cur_led=LED3;                          //cur_led�� LED3  �� ����
          break;
       case 4:                                          //Cur_FL: 4
         LCD_DisplayText(1,8,"4");
         cur_led=LED4;                          //cur_led�� LED4  �� ����
          break;
       case 5:                                          //Cur_FL: 5
         LCD_DisplayText(1,8,"5");
         cur_led=LED5;                          //cur_led�� LED5  �� ����
          break;
       case 6:                                          //Cur_FL: 6
         LCD_DisplayText(1,8,"6");
         cur_led=LED6;                          //cur_led�� LED6  �� ����
          break;
       case 7:                                          //Cur_FL: 7
         LCD_DisplayText(1,8,"7");
         cur_led=LED7;                          //cur_led�� LED7  �� ����
         break;
       default:                                         //���� �� ���� �̿��� ��
         LCD_DisplayText(1,8,"0");
         cur_led=LED0;                          //cur_led�� LED0  �� ����
       }
       
      	GPIOG->ODR &= ~0x00FF;	         // LED �ʱⰪ: LED0~7 Off
       GPIOG->ODR |= cur_led;                 //Fram ���� �о�� �������� LED  on
          
	while(1)
	{
            if(SW0_Flag)	        // ���ͷ�Ʈ (EXTI8) �߻�
            {
                  if(cur_led!=LED0)                            //��ǥ���� �������� �ٸ� ���
                  {
                      BEEP();                                           //���� on
                      LCD_DisplayText(2,8,"0");                       //��ǥ�� 0��       
                      while(cur_led!=LED0)                                      //��ǥ���� �������� �������� whiile��Ż��
                      {
                        DelayMS(500);                                 //��� 0.5��
                          GPIOG->ODR &=~cur_led;              //���� �� LED off
                          
                          if(cur_led >LED0)                         //��ǥ���� ���������� ���� ���
                              cur_led>>=1;                                  //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                          else                                            //��ǥ���� ���������� ���� ���
                              cur_led<<=1;                                      //���� led�ּҷ� ����Ʈ �̵�
                          GPIOG->ODR |=cur_led;               //������ LED on
                      }
                      BEEP3();                                          //����3ȸ  �ݺ�
                      LCD_DisplayText(1,8,"0");                 //GLCD ������ ǥ��
                      LCD_DisplayText(2,8,"-");                    //sw������
                      Fram_Write(36,0);                                 //FRAM36������ 0����
                   }
                  SW0_Flag = 0;
            }
            if(SW7_Flag)                  //���ͷ�Ʈ (EXTI15) �߻�
            {
                if(cur_led!=LED7)                                     //��ǥ���� �������� �ٸ� ���  
                {
                    BEEP();                                                     //���� on
                    LCD_DisplayText(2,8,"7");                           //GLCD ��ǥ�� 7��
                    while(cur_led!=LED7)                                        //��ǥ���� �������� �������� whiile��Ż��
                    {
                      DelayMS(500);                                         //��� 0.5��
                          GPIOG->ODR &=~cur_led;               //���� �� LED off
                          
                          if(cur_led >LED7)                         //��ǥ���� ���������� ���� ���
                          {
                              cur_led>>=1;                                      //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                          }
                          else                                            //��ǥ���� ���������� ���� ���
                          {
                              cur_led<<=1;                                      //����led�ּҷ� ����Ʈ �̵�
                          }
                          GPIOG->ODR |=cur_led;                //������ LED on
                    }
                    BEEP3();                                            //����3ȸ �ݺ�
                    LCD_DisplayText(1,8,"7");                           //GLCD ������ ǥ��
                    LCD_DisplayText(2,8,"-");                           //sw������
                    Fram_Write(36,7);                                   //FRAM36������ 7����
                } 
                SW7_Flag = 0;
            }
		switch(KEY_Scan())	// �Էµ� Switch ���� �з�
		{
        		case SW1 : 	                //SW1 �Է�
                           if(cur_led==LED1)                            //��ǥ���� �������� ���� ���
                           {
                              break;
                           }
                           else                                                 //��ǥ���� �������� �ٸ� ���
                          {
                              BEEP();                                           //���� on
                              LCD_DisplayText(2,8,"1");                         //GLCD ��ǥ�� 1��
                              while(cur_led!=LED1)                              //��ǥ���� �������� �������� whiile��Ż��                         
                              {
                                DelayMS(500);                                 //��� 0.5��
                                  GPIOG->ODR &=~cur_led;               //���� �� LED off
                                  
                                  if(cur_led >LED1)                         //��ǥ���� ���������� ���� ���
                                  {
                                      cur_led>>=1;                           //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                                  }
                                  else                                             //��ǥ���� ���������� ���� ���
                                  {
                                      cur_led <<=1;                             //����led�ּҷ� ����Ʈ �̵�
                                  }

                                  GPIOG->ODR |=cur_led;                //������ LED on
                               }
                             BEEP3();                                               //����3ȸ �ݺ�
                              LCD_DisplayText(1,8,"1");                         //GLCD ������ ǥ��
                              LCD_DisplayText(2,8,"-");                        //sw������
                              Fram_Write(36,1);                                   //FRAM36������ 1����
                              break;
                          }
                          case SW2:                         //SW2 �Է�         
                          if(cur_led==LED2)                                     //��ǥ���� �������� ���� ���
                          {
                                break;
                          }
                          else                                                 //��ǥ���� �������� �ٸ� ���
                          {
                            BEEP();                                             //���� on
                            LCD_DisplayText(2,8,"2");                   //GLCD ��ǥ�� 2��
                            while(cur_led!=LED2)                                //��ǥ���� �������� �������� whiile��Ż��
                            {
                              DelayMS(500);                                   //��� 0.5��
                                GPIOG->ODR &=~cur_led;               //���� �� LED off
                                
                                if(cur_led >LED2)                         //��ǥ���� ���������� ���� ���
                                {
                                    cur_led>>=1;                                //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                                }
                                else                                             //��ǥ���� ���������� ���� ���{
                                {
                                    cur_led<<=1;                                    //����led�ּҷ� ����Ʈ �̵�
                                }
                                GPIOG->ODR |=cur_led;                //������ LED on
                            }
                            BEEP3();                                                //����3ȸ �ݺ�
                            LCD_DisplayText(1,8,"2");                            //GLCD ������ ǥ��
                            LCD_DisplayText(2,8,"-");                            //sw������
                            Fram_Write(36,2);                                   //FRAM36������ 2����
                            break;
                          }
        		case SW3 : 	                //SW3 �Է�
                          if(cur_led==LED3)                                  //��ǥ���� �������� ���� ���
                          {
                              break;
                          }
                          else                                                 //��ǥ���� �������� �ٸ� ���
                          {
                            BEEP();
                            LCD_DisplayText(2,8,"3");                   //GLCD ��ǥ�� 3��
                            while(cur_led!=LED3)                                //��ǥ���� �������� �������� whiile��Ż��
                            {
                                DelayMS(500);                                     //��� 0.5��
                                GPIOG->ODR &=~cur_led;               //���� �� LED off
                                
                                if(cur_led >LED3)                         //��ǥ���� ���������� ���� ���
                                {
                                    cur_led>>=1;                                //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                                }
                                else                                             //��ǥ���� ���������� ���� ���
                                {
                                    cur_led<<=1;                                    //����led�ּҷ� ����Ʈ �̵�
                                }
                                GPIOG->ODR |=cur_led;                //������ LED on
                            }
                           BEEP3();                                                 //����3ȸ �ݺ�   
                            LCD_DisplayText(1,8,"3");                            //GLCD ������ ǥ��
                            LCD_DisplayText(2,8,"-");                            //sw������
                            Fram_Write(36,3);                                               //FRAM36������ 3����
                            break;
                          }
                        case SW4: 	               //SW4 �Է�
                         if(cur_led==LED4)                                  //��ǥ���� �������� ���� ���
                         {
                            break;
                         }
                          else                                                //��ǥ���� �������� �ٸ� ���
                          {
                            BEEP();
                            LCD_DisplayText(2,8,"4");                   //GLCD ��ǥ�� 4��
                            while(cur_led!=LED4)                                //��ǥ���� �������� �������� whiile��Ż��
                            {
                                DelayMS(500);                                     //��� 0.5��
                                GPIOG->ODR &=~cur_led;               //���� �� LED off
                                
                                if(cur_led >LED4)                         //��ǥ���� ���������� ���� ���
                                {
                                    cur_led>>=1;                                //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                                }
                                else                                             //��ǥ���� ���������� ���� ���
                                {
                                    cur_led<<=1;                                    //����led�ּҷ� ����Ʈ �̵�
                                }
                                GPIOG->ODR |=cur_led;                //������ LED on
                            }
                            BEEP3();                                                //����3ȸ �ݺ�
                            LCD_DisplayText(1,8,"4");                            //GLCD ������ ǥ��
                            LCD_DisplayText(2,8,"-");                            //sw������
                            Fram_Write(36,4);                                       //FRAM36������ 4����
                            break;
                          }
                        case SW5: 	               //SW5 �Է�
                           if(cur_led==LED5)                                  //��ǥ���� �������� ���� ���
                           {
                              break;
                           }
                          else                                                 //��ǥ���� �������� �ٸ� ���
                            {
                            BEEP();
                            LCD_DisplayText(2,8,"5");                           //GLCD ��ǥ�� 5��
                            while(cur_led!=LED5)                                //��ǥ���� �������� �������� whiile��Ż��
                            {
                                DelayMS(500);                                     //��� 0.5��
                                GPIOG->ODR &=~cur_led;               //���� �� LED off
                                
                                if(cur_led >LED5)                         //��ǥ���� ���������� ���� ���
                                {
                                    cur_led>>=1;                                        //�Ʒ��� led�ּҷ� ����Ʈ �̵�        
                                }
                                else                                             //��ǥ���� ���������� ���� ���
                                {
                                    cur_led<<=1;                                            //����led�ּҷ� ����Ʈ �̵�
                                }
                                GPIOG->ODR |=cur_led;                //������ LED on
                            }
                            BEEP3();                                                //����3ȸ �ݺ�
                            LCD_DisplayText(1,8,"5");                            //GLCD ������ ǥ��
                            LCD_DisplayText(2,8,"-");                            //sw������
                         Fram_Write(36,5);                                          //FRAM36������ 5����
                            break;
                          }
                        case SW6: 	                //SW6 �Է�
                           if(cur_led==LED6)                                      //��ǥ���� �������� ���� ���
                           {
                              break;
                           }
                           else                                                //��ǥ���� �������� �ٸ� ���
                           {
                              BEEP();
                              LCD_DisplayText(2,8,"6");                         //GLCD ��ǥ�� 6��
                              while(cur_led!=LED6)                                      //��ǥ���� �������� �������� whiile��Ż��
                              {
                                  DelayMS(500);                                   //��� 0.5��
                                  GPIOG->ODR &=~cur_led;               //���� �� LED off
                                
                                  if(cur_led >LED6)                         //��ǥ���� ���������� ���� ���
                                  {
                                    cur_led>>=1;                                //�Ʒ��� led�ּҷ� ����Ʈ �̵�
                                  }
                                  else                                             //��ǥ���� ���������� ���� ���
                                  {
                                    cur_led<<=1;
                                  }
                                  GPIOG->ODR |=cur_led;                //������ LED on
                               }
                               BEEP3();                                                     //����3ȸ �ݺ�
                               LCD_DisplayText(1,8,"6");                                 //GLCD ������ ǥ��
                              LCD_DisplayText(2,8,"-");                          //sw������
                              Fram_Write(36,6);                                     //FRAM36������ 6����
                               break;
                          }
                  }  // switch(KEY_Scan())
    	}  // while(1)
}

/* GLCD �ʱ�ȭ�� ���� */
void DisplayInitScreen(void)
{
        LCD_Clear(RGB_YELLOW);		// ȭ�� Ŭ����
        LCD_SetFont(&Gulim8);		// ��Ʈ : ���� 8
        LCD_SetBackColor(RGB_YELLOW);	// ���ڹ��� : YELLOW
        LCD_SetTextColor(RGB_BLUE);	// ���ڻ� :BLUE
        LCD_DisplayText(0,0,"MC Elevator (LCH)");  // Title
        LCD_SetTextColor(RGB_BLACK);    // ���ڻ� :BLACK
        LCD_DisplayText(1,0,"Cur FL: ");  // subtitle
        LCD_DisplayText(2,0,"Des FL: ");  // subtitle
        LCD_SetTextColor(RGB_RED);      //���ڻ�: RED
        LCD_DisplayText(1,8,"0");
        LCD_DisplayText(2,8,"-");
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joy stick)) �ʱ� ����	*/
void _GPIO_Init(void)
{
        // LED (GPIO G) ����
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) ���� 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) ���� 
    	RCC->AHB1ENR    |=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER    |=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI15(GPIOH.15, SW7)) �ʱ� ����  */
void _EXTI_Init(void)
{
    	RCC->AHB1ENR 	|= 0x00000080  ;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x00004000  ;	// Enable System Configuration Controller Clock
                                          //|=(1<<14);
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8,PIN15 Input mode (reset state)			 
	
	SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8�� ���� �ҽ� �Է��� GPIOH�� ����
					// EXTI8 <- PH8, 
					// EXTICR3(EXTICR[2])�� �̿� 
					// reset value: 0x0000
       SYSCFG->EXTICR[3] |= 0x7000; 	// EXTI15�� ���� �ҽ� �Է��� GPIOH�� ����	
                                        //EXTI15 <- PH15.
                                        //EXTICR4(EXTICR[3])�� �̿�
                                        //reset value: 0x0000
	EXTI->FTSR |= 0x000100;		        // EXTI8: Falling Trigger Enable
        EXTI->FTSR |= 0x008000;		        // EXTI15: Falling Trigger Enable
       
    	EXTI->IMR  |= 0x000100;  		// EXTI8 ���ͷ�Ʈ mask (Interrupt Enable) ����
	EXTI->IMR  |= 0x008000;  		// EXTI15 ���ͷ�Ʈ mask (Interrupt Enable) ����	
        
	NVIC->ISER[0] |= (1<<23 );  // Enable 'Global Interrupt EXTI8'
	NVIC->ISER[1] |= (1<<(40-32) );  // Enable 'Global Interrupt EXTI15'		
        // Vector table Position ����
}

/* EXTI5~9 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
    if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(�߻�) ����?
    {
      EXTI->PR |= 0x0100;                     // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
      if(SW0_Flag ==0)
      {
        SW0_Flag = 1;			// SW0_Flag: EXTI8�� �߻��Ǿ����� �˸��� ���� ���� ����(main���� mission�� ���) 
      }
    }
}

/*EXTI15~10 ���ͷ�Ʈ �ڵ鷯(ISR: interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
    if(EXTI-> PR & 0x8000)                              // EXTI15 Interrupt Pending(�߻�) ����?
    {
      EXTI->PR |= 0x8000;                               // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
      if(SW7_Flag ==0)
      { 
         SW7_Flag = 1;			// SW7_Flag: EXTI15�� �߻��Ǿ����� �˸��� ���� ���� ����(main���� mission�� ���) 
      }
    }
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
void BEEP3(void)
{
    BEEP();                                           //���� on
    DelayMS(100);                                    //���
    BEEP();                                           //���� on
    DelayMS(100);                                    //���
    BEEP();                                           //���� on
    DelayMS(100);                                     //���
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);         		// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
    	for(; Dly; Dly--);
}