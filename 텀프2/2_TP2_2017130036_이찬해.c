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

#define NAVI_PUSH	0x03C0  //PI5 0000 0011 1100 0000 
#define NAVI_UP		0x03A0  //PI6 0000 0011 1010 0000 
#define NAVI_DOWN        0x0360  //PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	0x02E0  //PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0  //PI9 0000 0001 1110 0000 

void _GPIO_Init(void);                  
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
uint8_t  SW0_Flag, SW7_Flag, Joy_Push_Flag;

//�����Լ���
void BEEP(void);
void BEEP3(void);
void BEEP5(void);

//����Լ���
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

//�¸���ư ���� ��, ���α׷�������ϴ� �Լ�
void ProgramRestart(void);

//������ û���� ��ǥ ������
int red_stone_x=5;
int red_stone_y=5;
int blue_stone_x=5;
int blue_stone_y=5;
int count=0;

//�ش���ǥ�� ���� �ִ��� ���θ� Ȯ���ϴ� 2���� �迭
int check_stone[10][10]={0};
//������ û���� ���� ������ ����ϴ� ����
int red_score;
int blue_score;

int main(void)
{
  _GPIO_Init(); 	          // GPIO (LED,SW,Buzzer)�ʱ�ȭ
  _EXTI_Init();                  //EXTI �ʱ�ȭ
  LCD_Init();	                 // LCD ��� �ʱ�ȭ
  Fram_Init();                    // FRAM �ʱ�ȭ H/W �ʱ�ȭ
  Fram_Status_Config();          // FRAM �ʱ�ȭ S/W �ʱ�ȭ
  DelayMS(10);                  //0.01�� ���
  red_score=Fram_Read(300);     //Fram300�ּҿ� ������ִ� ������ ������ red_score�� �Է�
  blue_score=Fram_Read(301);    //Fram301�ּҿ� ������ִ� û���� ������ blue_score�� �Է�
  DisplayInitScreen();            // LCD �ʱ�ȭ
  while(1){
    EXTI->IMR  &= ~0x000020;         // EXTI5���ͷ�Ʈ mask (Interrupt Disable) ����, ����, û�� ���ô������� �� JoyStick (EXTI5)Ű �Է¹���
    if(SW0_Flag==1)                     //���� ����
    {
      EXTI->IMR  &= ~0x008000;         // EXTI15���ͷ�Ʈ mask (Interrupt Disable) ����, ���� ���ʿ��� SW7 û�� ���� Ű �Է� ����
      if(count%2==0)
      {
        LCD_SetBrushColor(RGB_RED);                  //�귯�� ��: RED
        while(1)        
        {
          if(KEY_Scan()==SW1)                            //SW1 �� ������ ���� ��
          {
            if(red_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
            {
              red_score=0;
              Fram_Write(300,0);                              //Fram 300�� �ּҿ� red_score����
            }//if(red_score==9)
            else                                              //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
            {
              red_score++;
              Fram_Write(300,red_score);                      //Fram 300�� �ּҿ� red_score����
            }
            BEEP5();                                          //���� 5ȸ �︮��
            DelayMS(5000);                                    //5�� ���
            ProgramRestart();                                 //���α׷� ����� �Լ�
            break;
          }//if(!(GPIOH->IDR & ~SW1))
          if(KEY_Scan()==SW6)                            //SW6 �� ������ û�� ��
          {
            if(blue_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
            {
              blue_score=0;
              Fram_Write(301,0);                               //Fram 300�� �ּҿ� blue_score����
            }//if(blue_score==9)
            else                                               //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
            {
              blue_score++;
              Fram_Write(301,blue_score);                     //Fram 300�� �ּҿ� blue_score����
            }//if(blue_score!=9)
            BEEP5();                                          //���� 5ȸ �︮��
            DelayMS(5000);                                    //5�� ���
            ProgramRestart();                                 //���α׷� ����� �Լ�
            break;
          }//if(!(GPIOH->IDR & ~SW2))
          EXTI->IMR |= 0x000020;         // EXTI5���ͷ�Ʈ mask (Interrupt Enable) ����, JoyStick �Է¸�忡�� (EXTI5)Ȱ��ȭ
          switch(JOY_Scan())                      //���� JoyStick ���ۿ���
          {
          case NAVI_UP:                                 //Joy Stick UP ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(red_stone_y>0 && red_stone_y<=9)           //���� �� y��ǥ�� 1~9�϶����� ����
              red_stone_y--;
            LCD_DisplayChar(9,4,red_stone_y+48);                //GLCD ���� y��ǥ�� ����
            break;
            
          case NAVI_DOWN:                                //Joy Stick DOWN ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(red_stone_y>=0 && red_stone_y<9)           //���� �� y��ǥ�� 0~8�϶����� ����
              red_stone_y++;
            LCD_DisplayChar(9,4,red_stone_y+48);                //GLCD ���� y��ǥ�� ����
            break;
            
          case NAVI_LEFT:                                //Joy Stick LEFT ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(red_stone_x>0 && red_stone_x<=9)           //���� �� x��ǥ�� 1~9�϶����� ����
              red_stone_x--;
            LCD_DisplayChar(9,2,red_stone_x+48);                 //GLCD ���� x��ǥ�� ����
            break;
            
          case NAVI_RIGHT:                               //Joy Stick RIGHT ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(red_stone_x>=0 && red_stone_x<9)           //���� �� x��ǥ�� 0~8�϶����� ����
              red_stone_x++;
            LCD_DisplayChar(9,2,red_stone_x+48);                 //GLCD ���� x��ǥ�� ����
            break;
          }//switch(JOY_Scan)
          if(Joy_Push_Flag==1)                          //EXTI5 �߻�, ���� ���� �õ�
          {
            if(check_stone[red_stone_x][red_stone_y]==1)         //�� �ڸ��� ���� ������ ������� �︮�� ��ǥ������ �ٽ��Ѵ�.
            {
              DelayMS(100);                     //1�� ���
              BEEP3();                          //����3���︮��
              Joy_Push_Flag=0;                  //JoyStick Push ���ൿ���� ������ �����·� ��ȯ
            }// if(check_stone[red_stone_x][red_stone_y]==1)
            else                                                  //�� �ڸ��� ���� ������ ����
            {
              BEEP();                                    //���� �︮��
              check_stone[red_stone_x][red_stone_y]=1;   //���� ��ǥ���� ����, �� ��ǥ�� ������ ����
              LCD_DrawFillRect(35+8*red_stone_x-3, 31+8*red_stone_y-3, 7, 7);    //�ش� ��ǥ���� ���� ����
              Joy_Push_Flag=0;                                                    //JoyStick Push ���ൿ���� ������ �����·� ��ȯ
              count++;
              EXTI->IMR  &= ~0x000020;         // EXTI5���ͷ�Ʈ mask (Interrupt Disable) ����, JoyStickPush�� ���� ��(EXTI5) ��Ȱ��ȭ
              break;
            }//else if(check_stone[red_stone_x][red_stone_y]!=1)
          }//if(Joy_Push_Flag==1)
        }//while(1) in SW0, ���� ������ ���̽�ƽ ���� �ݺ�
      }//if(count/2==0)
      SW0_Flag=0;
      EXTI->IMR  |= 0x008000;         // EXTI15���ͷ�Ʈ mask (Interrupt Enable) ����, ���� ���ʰ� ���� �� û�� ���� Ű(EXTI15) Ȱ��ȭ
    }//if(SW0_Flag==1)
    else if(SW7_Flag==1)                  //û�� ���̽�ƽ ���� ����
    {
      EXTI->IMR  &= ~0x000100;         // EXTI8���ͷ�Ʈ mask (Interrupt Disable) ����, û�� ���ʿ��� ���� ���� Ű(EXTI8) ��Ȱ��ȭ
      if(count%2==1)
      {
        LCD_SetBrushColor(RGB_BLUE);                  //�귯�� ��: BLUE
        while(1)
        {
          if(KEY_Scan()==SW1)                            //SW1 �� ������ ���� ��
          {
            if(red_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
            {
              red_score=0;
              Fram_Write(300,0);                              //Fram 300�� �ּҿ� red_score����
            }//if(red_score==9)
            else                                              //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
            {
              red_score++;
              Fram_Write(300,red_score);                      //Fram 300�� �ּҿ� red_score����
            }
            BEEP5();                                          //���� 5ȸ �︮��
            DelayMS(5000);                                    //5�� ���
            ProgramRestart();                                 //���α׷� ����� �Լ�
            break;
          }//if(!(GPIOH->IDR & ~SW1))
          if(KEY_Scan()==SW6)                            //SW6 �� ������ û�� ��
          {
            if(blue_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
            {
              blue_score=0;
              Fram_Write(301,0);                               //Fram 300�� �ּҿ� blue_score����
            }//if(blue_score==9)
            else                                               //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
            {
              blue_score++;
              Fram_Write(301,blue_score);                     //Fram 300�� �ּҿ� blue_score����
            }//if(blue_score!=9)
            BEEP5();                                          //���� 5ȸ �︮��
            DelayMS(5000);                                    //5�� ���
            ProgramRestart();                                 //���α׷� ����� �Լ�
            break;
          }//if(!(GPIOH->IDR & ~SW2))
          EXTI->IMR |= 0x000020;         // EXTI5���ͷ�Ʈ mask (Interrupt Enable) ����, JoyStickPush Ű(EXTI5) Ȱ��ȭ
          switch(JOY_Scan())                      //û�� JoyStick ���ۿ���
          {
          case NAVI_UP:                                 //Joy Stick UP ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(blue_stone_y>0 && blue_stone_y<=9)         //���� �� y��ǥ�� 1~9�� ������ ����
              blue_stone_y--;
            LCD_DisplayChar(9,16,blue_stone_y+48);                //GLCD û�� x��ǥ�� ����
            break;
            
          case NAVI_DOWN:                                //Joy Stick DOWN ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(blue_stone_y>=0 && blue_stone_y<9)         //���� �� y��ǥ�� 0~8�� ������ ����
              blue_stone_y++;
            LCD_DisplayChar(9,16,blue_stone_y+48);                //GLCD���� y��ǥ�� ����
            break;
            
          case NAVI_LEFT:                                //Joy Stick LEFT ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(blue_stone_x>0 && blue_stone_x<=9)         //���� �� x��ǥ�� 1~9�� ������ ����
              blue_stone_x--;
            LCD_DisplayChar(9,14,blue_stone_x+48);                 //GLCD���� X��ǥ�� ����
            break;
            
          case NAVI_RIGHT:                               //Joy Stick RIGHT ��ư ����
            BEEP();                                       //���� 1ȸ �︮��
            if(blue_stone_x>=0 && blue_stone_x<9)         //���� �� x��ǥ�� 0~8�� ������ ����
              blue_stone_x++;
            LCD_DisplayChar(9,14,blue_stone_x+48);                 //GLCD���� X��ǥ�� ����
            break;
          }//switch(JOY_Scan)
          if(Joy_Push_Flag==1)                          //EXTI5 �߻�, û�� ���� �õ�
          {
            if(check_stone[blue_stone_x][blue_stone_y]==1)         //�� �ڸ��� ���� ������ ������� �︮�� ��ǥ������ �ٽ��Ѵ�.
            {
              DelayMS(100);                     //1�� ���
              BEEP3();                          //����3���︮��
              Joy_Push_Flag=0;                    //JoyStick Push ���ൿ���� ������ �����·� ��ȯ
            }// if(check_stone[blue_stone_x][blue_stone_y]==1)
            else                                                  //�� �ڸ��� ���� ������ ����
            {
              BEEP();                                    //���� �︮��
              check_stone[blue_stone_x][blue_stone_y]=1;   //���� ��ǥ���� ����, �� ��ǥ�� ������ ����
              LCD_DrawFillRect(35+8*blue_stone_x-3, 31+8*blue_stone_y-3, 7, 7);    //�ش� ��ǥ���� û�� ����
              Joy_Push_Flag=0;                            //JoyStick Push ���ൿ���� ������ �����·� ��ȯ
              count++;
              EXTI->IMR  &= ~0x000020;         // EXTI5���ͷ�Ʈ mask (Interrupt Disable) ����, ���� �� JoyStickPush(EXTI5) ��Ȱ��ȭ
              break;
            }//else if(check_stone[blue_stone_x][blue_stone_y]!=1)
          }//if(Joy_Push_Flag==1)
        }//while(1) in SW7, û�� ���� ������ ���̽�ƽ ���� �ݺ�
      }//if(count/2!=0)
      SW7_Flag=0;
      EXTI->IMR  |= 0x000100;         // EXTI8���ͷ�Ʈ mask (Interrupt Enable) ����, û�� ���� ���� �� ���� ���� (EXTI8)Ȱ��ȭ
    }//if(SW7_Flag==1)
    if(KEY_Scan()==SW1)                            //SW1 �� ������ ���� ��
    {
      if(red_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
      {
        red_score=0;
        Fram_Write(300,0);                              //Fram 300�� �ּҿ� red_score����
      }//if(red_score==9)
      else                                              //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
      {
        red_score++;
        Fram_Write(300,red_score);                      //Fram 300�� �ּҿ� red_score����
      }
      BEEP5();                                          //���� 5ȸ �︮��
      DelayMS(5000);                                    //5�� ���
      ProgramRestart();                                 //���α׷� ����� �Լ�
    }//if(!(GPIOH->IDR & ~SW1))
    if(KEY_Scan()==SW6)                            //SW6 �� ������ û�� ��
    {
      if(blue_score==9)                                  //������ 9���̾��ٸ� 0���� ���ư�
      {
        blue_score=0;
        Fram_Write(301,0);                               //Fram 300�� �ּҿ� blue_score����
      }//if(blue_score==9)
      else                                               //������ 9���� �ƴ϶�� 1���� �߰��Ѵ�.
      {
        blue_score++;
        Fram_Write(301,blue_score);                     //Fram 300�� �ּҿ� blue_score����
      }//if(blue_score!=9)
      BEEP5();                                          //���� 5ȸ �︮��
      DelayMS(5000);                                    //5�� ���
      ProgramRestart();                                 //���α׷� ����� �Լ�
    }//if(!(GPIOH->IDR & ~SW2))
  }// while(1)
  
}//main(void)

//GLCD �ʱ�ȭ�� ����
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_YELLOW);		// ȭ�� Ŭ����
  LCD_SetFont(&Gulim8);		// ��Ʈ : ���� 8
  LCD_SetBackColor(RGB_YELLOW);	// ���ڹ��� : YELLOW
  
  LCD_SetTextColor(RGB_BLACK);	// ���ڻ� :BLACK
  
  ///// Title(MeCha OMOK) /////
  LCD_DisplayText(0,0,"MeCha OMOK(LCH)");
  
  ///// ������ �׸��� /////
  LCD_DisplayText(1,3,"0");
  LCD_DisplayText(1,9,"5");
  LCD_DisplayText(1,13,"9");
  LCD_DisplayText(5,3,"5");
  LCD_DisplayText(8,3,"9");
  LCD_SetPenColor(RGB_BLACK);           //�� ��: BLACK
  //������//
  for(int i=35; i<=107;i+=8)
  {
    LCD_DrawVerLine(i, 31, 72);
  }
  //������//
  for(int i=31;i<=103;i+=8)
  {
    LCD_DrawHorLine(35, i, 72);
  }
  //(5,5) ��ġ ǥ�� �簢��//
  LCD_SetBrushColor(RGB_BLACK);
  LCD_DrawFillRect(73, 69, 5, 5);
  
  ///// ��Ȳ�� /////
  LCD_DisplayText(9,9,"vs");
  LCD_SetTextColor(RGB_RED);           //�� ��: RED
  LCD_DisplayText(9,1,"( , )");                
  LCD_DisplayChar(9,2,red_stone_x+48);         //���� x��ǥ�� ���� ��ǥ �� ����
  LCD_DisplayChar(9,4,red_stone_y+48);         //���� y��ǥ�� ���� ��ǥ �� ����
  LCD_DisplayChar(9,8,red_score+48);                             //0�� fram
  LCD_SetTextColor(RGB_BLUE);         //�� ��: BLUE
  LCD_DisplayChar(9,11,blue_score+48);                          //0�� fram
  LCD_DisplayText(9,13,"( , )");                
  LCD_DisplayChar(9,14,blue_stone_x+48);       //û�� x��ǥ�� ���� ��ǥ �� ����
  LCD_DisplayChar(9,16,blue_stone_y+48);       //û�� y��ǥ�� ���� ��ǥ �� ����
}

//GPIO �ʱ⼳��
void _GPIO_Init(void)
{
  // LED (GPIO G) ����
  RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
  GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
  GPIOG->OTYPER	&= ~0x000000FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
  GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
  
  // SW (GPIO H) ���� 
  RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
  GPIOH->MODER 	&=  ~0xFFFF0000; // remove after class        //~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
  GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state
  
  // Buzzer (GPIO F) ���� 
  RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
  GPIOF->MODER 	|=  0x00040050;	// GPIOF 9 : Output mode (0b01)						
  GPIOF->OTYPER 	&= ~0x00000200;	// GPIOF 9 : Push-pull  	
  GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
  
  //Joy Stick SW(PORT I) ����
  RCC->AHB1ENR 	|= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
  GPIOI->MODER 	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
  GPIOI->PUPDR    &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}

//EXTI ���ͷ�Ʈ �ʱ⼳��
void _EXTI_Init(void)
{
  RCC->AHB1ENR 	|= 0x00000180;	// RCC_AHB1ENR GPIOH, GPIOI Enable
  RCC->APB2ENR 	|= 0x00004000;	// Enable System Configuration Controller Clock
  // |=(1<<14);
  GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)	
  GPIOI->MODER    &= ~0x000FFC00;
  SYSCFG->EXTICR[1] |= 0x0080; 	//EXTI5�� ���� �ҽ� �Է��� GPIOI�� ���� 
  SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8�� ���� �ҽ� �Է��� GPIOH�� ����
  SYSCFG->EXTICR[3] |= 0x7000;	//EXTI15�� ���� �ҽ� �Է��� GPIOH�� ����
  EXTI->FTSR |= 0x008120;		// EXTI8,10,15: Falling Trigger Enable 
  EXTI->IMR  |= 0x008120;         // EXTI8,10,15���ͷ�Ʈ mask (Interrupt Enable) ����
  
  NVIC->ISER[0] |= ( 1<<23 );  // Enable 'Global Interrupt EXTI8
  // Vector table Position ����
  NVIC->ISER[1] |= ( 1<<(40-32) );
}

/* EXTI10~15 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{		
  if(EXTI->PR & 0x8000) 			// EXTI15 Interrupt Pending(�߻�) ����?, û�� ����
  {
    EXTI->PR |= 0x8000; 		// Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
    if(count==0)
    {
      count++;
    }
    SW7_Flag = 1;			// SW7_Flag: EXTI15�� �߻��Ǿ����� �˸��� ���� ���� ����(main���� mission�� ���) 
    GPIOG->ODR |=0x80;
    GPIOG->ODR &=~0x01;
    LCD_DisplayText(9,0," ");
    LCD_SetTextColor(RGB_BLUE);           //�� ��: BLUE
    LCD_DisplayText(9,18,"*");
  }
}

/* EXTI5~9 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
  if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(�߻�) ����, ���� ����
  {
    EXTI->PR |= 0x0100; 		// Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
    SW0_Flag = 1;			// SW0_Flag: EXTI8�� �߻��Ǿ����� �˸��� ���� ���� ����(main���� mission�� ���
    GPIOG->ODR |=0x01;
    GPIOG->ODR &=~0x80;
    LCD_DisplayText(9,18," ");
    LCD_SetTextColor(RGB_RED);           //�� ��: RED
    LCD_DisplayText(9,0,"*");
    
  }
  else if(EXTI->PR & 0x0020)              //EXTI5 Interrupt Pending (�߻�) ����, ���� ��ư
  {
    EXTI->PR |= 0x0020; 		// Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
    Joy_Push_Flag = 1;	        // J-P_Flag: JOYSTICK PUSH�� �߻��Ǿ����� �˸��� ���� ���� ����(main���� mission�� ���) 
    
  }
}

/* JoyStick�� �ԷµǾ������� ���ο� � JoyStick�� �ԷµǾ������� ������ return�ϴ� �Լ�  */ 
uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{ 
  uint16_t key;
  key = GPIOI->IDR & 0x03E0;	// any key pressed ?
  if(key == 0x03E0)		// if no key, check key off
  {  	if(joy_flag == 0)
    return key;
  else
  {	DelayMS(10); 
  joy_flag = 0;
  return key;
  }
  }
  else				// if key input, check continuous key
  {	if(joy_flag != 0)	// if continuous key, treat as no key input
    return 0x03E0;
  else			// if new key,delay for debounce
  {	joy_flag = 1;
  DelayMS(10);
  return key;
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
//�����︮�� �Լ�
void BEEP(void)
{ 	
  GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
  DelayMS(30);		// Delay 30 ms
  GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}
//������ 3�� �︮�� �Լ�
void BEEP3(void)
{
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
}

//������ 5�� �︮�� �Լ�
void BEEP5(void)
{
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
}

//milli second ������ ����ϴ� �Լ�
void DelayMS(unsigned short wMS)
{
  register unsigned short i;
  for (i=0; i<wMS; i++)
    DelayUS(1000);         		// 1000us => 1ms
}

//micro second ������ ����ϴ� �Լ�
void DelayUS(unsigned short wUS)
{
  volatile int Dly = (int)wUS*17;
  for(; Dly; Dly--);
}

//���α׷� ������ϴ� �Լ�
void ProgramRestart(void)
{
  red_score=Fram_Read(300);             //Fram 300�� �ּҿ� ����Ǿ��ִ� ���� �¸� Ƚ���� red_score�� ����
  blue_score=Fram_Read(301);            //Fram 301�� �ּҿ� ����Ǿ��ִ� û�� �¸� Ƚ���� blue_score�� ����
  DisplayInitScreen();                  //�ʱ�ȭ�� ����
  for(int i=0; i<10;i++)
    for(int j=0; j<10;j++)
      check_stone[i][j]=0;              //��ǥ�� ���� �ִ��� ���θ� �����ϴ� �迭 �ʱ�ȭ
  GPIOG->ODR &=~0x01;                   //LED0 OFF
  GPIOG->ODR &=~0x80;                   //LED7 OFF
}