extern "C" {
#include <delay.h>
#include <math.h>
#include <FillPat.h>
#include <I2CEEPROM.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
}

#define MODE_0 0
#define MODE_1 2
#define MODE_2 1
#define MODE_3 3
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

extern int xchOledMax; // defined in OrbitOled.c
extern int ychOledMax; // defined in OrbitOled.c

char	chSwtCur;
char	chSwtPrev;
bool	fClearOled;

void DeviceInit();
char CheckSwitches();
void OrbitSetOled();
void mode0();
void mode1();
void mode2();
void mode3();

void setup()
{
  DeviceInit();
}

void loop()
{

  char bDemoState = 0;
  volatile uint32_t ui32Loop;

  bDemoState = CheckSwitches();
  for(ui32Loop = 0; ui32Loop < 200000; ui32Loop++)
  {
  }

  switch(bDemoState) {

  case MODE_0:   //step counter
    mode0();
    break;
  case MODE_1:
    mode1();
    break;
  case MODE_2:
    mode2();
    break;
  case MODE_3:
    mode3();
    break;
  default:
    mode0();
    break;
  }

}


void DeviceInit()
{
  /*
   * First, Set Up the Clock.
   * Main OSC		  -> SYSCTL_OSC_MAIN
   * Runs off 16MHz clock -> SYSCTL_XTAL_16MHZ
   * Use PLL		  -> SYSCTL_USE_PLL
   * Divide by 4	  -> SYSCTL_SYSDIV_4
   */
  SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ | SYSCTL_USE_PLL | SYSCTL_SYSDIV_4);

  /*
   * Enable and Power On All GPIO Ports
   */
  //SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB | SYSCTL_PERIPH_GPIOC |
  //						SYSCTL_PERIPH_GPIOD | SYSCTL_PERIPH_GPIOE | SYSCTL_PERIPH_GPIOF);

  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOA );
  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOB );
  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOC );
  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOD );
  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOE );
  SysCtlPeripheralEnable(	SYSCTL_PERIPH_GPIOF );
  /*
   * Pad Configure.. Setting as per the Button Pullups on
   * the Launch pad (active low).. changing to pulldowns for Orbit
   */
  GPIOPadConfigSet(SWTPort, SWT1 | SWT2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

  GPIOPadConfigSet(BTN1Port, BTN1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
  GPIOPadConfigSet(BTN2Port, BTN2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

  GPIOPadConfigSet(LED1Port, LED1, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);
  GPIOPadConfigSet(LED2Port, LED2, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);
  GPIOPadConfigSet(LED3Port, LED3, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);
  GPIOPadConfigSet(LED4Port, LED4, GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);

  /*
   * Initialize Switches as Input
   */
  GPIOPinTypeGPIOInput(SWTPort, SWT1 | SWT2);

  /*
   * Initialize Buttons as Input
   */
  GPIOPinTypeGPIOInput(BTN1Port, BTN1);
  GPIOPinTypeGPIOInput(BTN2Port, BTN2);

  /*
   * Initialize LEDs as Output
   */
  GPIOPinTypeGPIOOutput(LED1Port, LED1);
  GPIOPinTypeGPIOOutput(LED2Port, LED2);
  GPIOPinTypeGPIOOutput(LED3Port, LED3);
  GPIOPinTypeGPIOOutput(LED4Port, LED4);

  /*
   * Enable ADC Periph
   */
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

  GPIOPinTypeADC(AINPort, AIN);

 
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH0);
  ADCSequenceEnable(ADC0_BASE, 0);

  /*
   * Initialize the OLED
   */
  OrbitOledInit();

  /*
   * Reset flags
   */
  chSwtCur = 0;
  chSwtPrev = 0;
  fClearOled = true;

}

/* ------------------------------------------------------------ */
/***	CheckSwitches()
 **
 **	Parameters:
 **		none
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Return the state of the Switches
 */
char CheckSwitches() {

  long 	lSwt1;
  long 	lSwt2;

  chSwtPrev = chSwtCur;

  lSwt1 = GPIOPinRead(SWT1Port, SWT1);
  lSwt2 = GPIOPinRead(SWT2Port, SWT2);

  chSwtCur = (lSwt1 | lSwt2) >> 6;

  if(chSwtCur != chSwtPrev) {
    fClearOled = true;
  }

  return chSwtCur;

}

//Mode 0

void mode0() {
  int inputed = 0;
  int weight;
uint32_t	ulAIN0;
    long lBtn1;
    long lBtn2;
    char szAIN[6] = {0};
    char cMSB = 0x00;
    char cMIDB = 0x00;
    char cLSB = 0x00;


  while(CheckSwitches() == MODE_0) {
      if(fClearOled == true) {
        OrbitOledClear();
        OrbitOledMoveTo(0,0);
        OrbitOledSetCursor(0,0);
        fClearOled = false;
      }
    
    
      ADCProcessorTrigger(ADC0_BASE, 0);
    
      while(!ADCIntStatus(ADC0_BASE, 0, false));
    
      ADCSequenceDataGet(ADC0_BASE, 0, &ulAIN0);
    
      cMSB = (0xF00 & ulAIN0) >> 8;
      cMIDB = (0x0F0 & ulAIN0) >> 4;
      //cLSB = (0x00F & ulAIN0);
      
    
      weight=200*(cMSB*16*16+cMIDB*16)/(16*16*15+16*15);
      char weightDisplay[10];
OrbitOledSetCursor(0,0);
OrbitOledPutString("    STEP mode");
OrbitOledSetCursor(0,1);
OrbitOledPutString("Input ur Weight"); 

sprintf(weightDisplay,"%03d",weight);

OrbitOledSetCursor(0,2);

    
    

OrbitOledPutString(weightDisplay); 
OrbitOledSetCursor(3,2);
OrbitOledPutString("KG"); 
OrbitOledSetCursor(0,3);
OrbitOledPutString("            save");    
      lBtn1 = GPIOPinRead(BTN1Port, BTN1);
      if(lBtn1 == BTN1) {
    OrbitOledClear();
    OrbitOledSetCursor(11,4);
    OrbitOledPutString("Reset");
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
     break;
      }        
  }

  short dataZ;
  short dataX;
  short dataY;
  char printVal[10];
  char printCal[10];
  int count=0;
  

  char 	chPwrCtlReg = 0x2D;
  char 	chX0Addr = 0x32;
  char  chY0Addr = 0x34;
  char  chZ0Addr = 0x36;
  char 	rgchReadAccl[] = {0, 0, 0};
  char 	rgchWriteAccl[] = {0, 0, 0};
  char  rgchReadAccl2[] = {0, 0, 0};
  char rgchReadAccl3[] = {0, 0, 0};
     
 /*
     * Enable I2C Peripheral
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    /*
     * Set I2C GPIO pins
     */
    GPIOPinTypeI2C(I2CSDAPort, I2CSDA_PIN);
    GPIOPinTypeI2CSCL(I2CSCLPort, I2CSCL_PIN);
    GPIOPinConfigure(I2CSCL);
    GPIOPinConfigure(I2CSDA);

    /*
     * Setup I2C
     */
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);

    /* Initialize the Accelerometer
     *
     */
    GPIOPinTypeGPIOInput(ACCL_INT2Port, ACCL_INT2);

    rgchWriteAccl[0] = chPwrCtlReg;
    rgchWriteAccl[1] = 1 << 3;		// sets Accl in measurement mode
    I2CGenTransmit(rgchWriteAccl, 1, WRITE, ACCLADDR);

  
  /*
   * If applicable, reset OLED
   */
  if(fClearOled == true) {
    OrbitOledClear();
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
    fClearOled = false;
  }
   
    /* Initialize the Accelerometer
     *
     */
    GPIOPinTypeGPIOInput(ACCL_INT2Port, ACCL_INT2);

    rgchWriteAccl[0] = chPwrCtlReg;
    rgchWriteAccl[1] = 1 << 3;		// sets Accl in measurement mode
    I2CGenTransmit(rgchWriteAccl, 1, WRITE, ACCLADDR);
   
   int next =0;
   int steps = -1;
   int initial;
   int flag = 0;

     while(CheckSwitches() == MODE_0) {

    /*
     * Read the X data register
     */
    rgchReadAccl[0] = chX0Addr;
    rgchReadAccl2[0] = chY0Addr;
    rgchReadAccl3[0] = chZ0Addr;
    
    I2CGenTransmit(rgchReadAccl, 2, READ, ACCLADDR);
    I2CGenTransmit(rgchReadAccl2, 2, READ, ACCLADDR);
    I2CGenTransmit(rgchReadAccl3, 2, READ, ACCLADDR);
    
    dataX = (rgchReadAccl[2] << 8) | rgchReadAccl[1];
    dataY = (rgchReadAccl2[2] << 8) | rgchReadAccl2[1];
    dataZ = (rgchReadAccl3[2] << 8) | rgchReadAccl3[1];
    
    if (dataZ>dataX&&dataZ>dataY) initial=285;
    else if (dataX>dataY&&dataX>dataZ)initial =245;
    else initial=265;
    
    double calorie = weight*0.57/2200*steps;
    sprintf(printVal, "Steps is %02d",steps);
    sprintf(printCal, "Calories burned: %.2g0",calorie);
    
    next = sqrt(dataX*dataX+dataY*dataY+dataZ*dataZ);
    if(next - initial > 5 && flag==0) {
     steps++;
     flag=1;
     initial = next;
    }
    
    if (next-initial<-5) flag=0;
    
    OrbitOledSetCursor(0,0);
    OrbitOledPutString(printVal);
    OrbitOledSetCursor(0,1);
    OrbitOledPutString(printCal);
    OrbitOledUpdate();
//check if button is pressed
     lBtn1 = GPIOPinRead(BTN1Port, BTN1);
     if(lBtn1 == BTN1)
	steps=0;

     delay(100);
     count++;
     count =count%5;
     if(count==5){
     printVal[10] = {0};
     OrbitOledClear();
     }
     if (CheckSwitches()!= MODE_0) break;
  }

}

void mode1() {
  int inputed = 0;
  int weight;
uint32_t	ulAIN0;
    long lBtn1;
    long lBtn2;
    char szAIN[6] = {0};
    char cMSB = 0x00;
    char cMIDB = 0x00;
    char cLSB = 0x00;


  while(CheckSwitches() == MODE_1) {
      if(fClearOled == true) {
        OrbitOledClear();
        OrbitOledMoveTo(0,0);
        OrbitOledSetCursor(0,0);
        fClearOled = false;
      }
    
    
      ADCProcessorTrigger(ADC0_BASE, 0);
    
      while(!ADCIntStatus(ADC0_BASE, 0, false));
    
      ADCSequenceDataGet(ADC0_BASE, 0, &ulAIN0);
    
      cMSB = (0xF00 & ulAIN0) >> 8;
      cMIDB = (0x0F0 & ulAIN0) >> 4;
      //cLSB = (0x00F & ulAIN0);
      
    
      weight=200*(cMSB*16*16+cMIDB*16)/(16*16*15+16*15);
      char weightDisplay[10];
OrbitOledSetCursor(0,0);
OrbitOledPutString("    STAIR mode");
OrbitOledSetCursor(0,1);
OrbitOledPutString("Input ur Weight"); 

sprintf(weightDisplay,"%03d",weight);

OrbitOledSetCursor(0,2);

    
    

OrbitOledPutString(weightDisplay); 
OrbitOledSetCursor(3,2);
OrbitOledPutString("KG"); 
OrbitOledSetCursor(0,3);
OrbitOledPutString("            save");    
      lBtn1 = GPIOPinRead(BTN1Port, BTN1);
      if(lBtn1 == BTN1) {
    OrbitOledClear();
    OrbitOledSetCursor(11,4);
    OrbitOledPutString("Reset");
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
     break;
      }        
  }

  short dataZ;
  short dataX;
  short dataY;
  char printVal[10];
  char printCal[10];
  int count=0;
  

  char 	chPwrCtlReg = 0x2D;
  char 	chX0Addr = 0x32;
  char  chY0Addr = 0x34;
  char  chZ0Addr = 0x36;
  char 	rgchReadAccl[] = {0, 0, 0};
  char 	rgchWriteAccl[] = {0, 0, 0};
  char  rgchReadAccl2[] = {0, 0, 0};
  char rgchReadAccl3[] = {0, 0, 0};
     
 /*
     * Enable I2C Peripheral
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    /*
     * Set I2C GPIO pins
     */
    GPIOPinTypeI2C(I2CSDAPort, I2CSDA_PIN);
    GPIOPinTypeI2CSCL(I2CSCLPort, I2CSCL_PIN);
    GPIOPinConfigure(I2CSCL);
    GPIOPinConfigure(I2CSDA);

    /*
     * Setup I2C
     */
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);

    /* Initialize the Accelerometer
     *
     */
    GPIOPinTypeGPIOInput(ACCL_INT2Port, ACCL_INT2);

    rgchWriteAccl[0] = chPwrCtlReg;
    rgchWriteAccl[1] = 1 << 3;		// sets Accl in measurement mode
    I2CGenTransmit(rgchWriteAccl, 1, WRITE, ACCLADDR);

  
  /*
   * If applicable, reset OLED
   */
  if(fClearOled == true) {
    OrbitOledClear();
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
    fClearOled = false;
  }
   
    /* Initialize the Accelerometer
     *
     */
    GPIOPinTypeGPIOInput(ACCL_INT2Port, ACCL_INT2);

    rgchWriteAccl[0] = chPwrCtlReg;
    rgchWriteAccl[1] = 1 << 3;		// sets Accl in measurement mode
    I2CGenTransmit(rgchWriteAccl, 1, WRITE, ACCLADDR);
   
   int next =0;
   int steps = -1;
   int initial;
   int flag = 0;

     while(CheckSwitches() == MODE_1) {

    /*
     * Read the X data register
     */
    rgchReadAccl[0] = chX0Addr;
    rgchReadAccl2[0] = chY0Addr;
    rgchReadAccl3[0] = chZ0Addr;
    
    I2CGenTransmit(rgchReadAccl, 2, READ, ACCLADDR);
    I2CGenTransmit(rgchReadAccl2, 2, READ, ACCLADDR);
    I2CGenTransmit(rgchReadAccl3, 2, READ, ACCLADDR);
    
    dataX = (rgchReadAccl[2] << 8) | rgchReadAccl[1];
    dataY = (rgchReadAccl2[2] << 8) | rgchReadAccl2[1];
    dataZ = (rgchReadAccl3[2] << 8) | rgchReadAccl3[1];
    
    if (dataZ>dataX&&dataZ>dataY) initial=285;
    else if (dataX>dataY&&dataX>dataZ)initial =245;
    else initial=265;
    
    double calorie = weight*0.25*steps*0.000239006;
    sprintf(printVal, "Stairs is %02d    Calories burned: %.2g0",steps,calorie);
      
    
    
    next = sqrt(dataX*dataX+dataY*dataY+dataZ*dataZ);
    if(next - initial > 30 && flag==0) {
     steps++;
     flag=1;
     initial = next;
    }
    
    if (next-initial<-1) flag=0;
    
    OrbitOledSetCursor(0,0);
    OrbitOledPutString(printVal);

 
    OrbitOledUpdate();
//check if button is pressed
     lBtn1 = GPIOPinRead(BTN1Port, BTN1);
     if(lBtn1 == BTN1)
	steps=0;

     delay(50);
     count++;
     count =count%7;
     if(count==7){
     printVal[10] = {0};
     OrbitOledClear();
     }
     if (CheckSwitches()!= MODE_1) break;
  }

}

void mode2() {
   if(fClearOled == true) {
    OrbitOledClear();
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
    fClearOled = false;
  }

}

void mode3() {
   if(fClearOled == true) {
    OrbitOledClear();
    OrbitOledMoveTo(0,0);
    OrbitOledSetCursor(0,0);
    fClearOled = false;
  }
  OrbitOledPutString(" Calorie meter");
  OrbitOledSetCursor(0,2);
  OrbitOledPutString("Use switches to choose modes.");
  delay(1000);
  
  fClearOled=true;
}





/* ------------------------------------------------------------ */
/***	I2CGenTransmit
 **
 **	Parameters:
 **		pbData	-	Pointer to transmit buffer (read or write)
 **		cSize	-	Number of byte transactions to take place
 **
 **	Return Value:
 **		none
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Transmits data to a device via the I2C bus. Differs from
 **		I2C EEPROM Transmit in that the registers in the device it
 **		is addressing are addressed with a single byte. Lame, but..
 **		it works.
 **
 */
char I2CGenTransmit(char * pbData, int cSize, bool fRW, char bAddr) {

  int 		i;
  char * 		pbTemp;

  pbTemp = pbData;

  /*Start*/

  /*Send Address High Byte*/
  /* Send Write Block Cmd*/
  I2CMasterSlaveAddrSet(I2C0_BASE, bAddr, WRITE);
  I2CMasterDataPut(I2C0_BASE, *pbTemp);

  I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);

  DelayMs(1);

  /* Idle wait*/
  while(I2CGenIsNotIdle());

  /* Increment data pointer*/
  pbTemp++;

  /*Execute Read or Write*/

  if(fRW == READ) {

    /* Resend Start condition
	** Then send new control byte
	** then begin reading
	*/
    I2CMasterSlaveAddrSet(I2C0_BASE, bAddr, READ);

    while(I2CMasterBusy(I2C0_BASE));

    /* Begin Reading*/
    for(i = 0; i < cSize; i++) {

      if(cSize == i + 1 && cSize == 1) {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));
      }
      else if(cSize == i + 1 && cSize > 1) {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));
      }
      else if(i == 0) {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));

        /* Idle wait*/
        while(I2CGenIsNotIdle());
      }
      else {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));

        /* Idle wait */
        while(I2CGenIsNotIdle());
      }

      while(I2CMasterBusy(I2C0_BASE));

      /* Read Data */
      *pbTemp = (char)I2CMasterDataGet(I2C0_BASE);

      pbTemp++;

    }

  }
  else if(fRW == WRITE) {

    /*Loop data bytes */
    for(i = 0; i < cSize; i++) {
      /* Send Data */
      I2CMasterDataPut(I2C0_BASE, *pbTemp);

      while(I2CMasterBusy(I2C0_BASE));

      if(i == cSize - 1) {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));
      }
      else {
        I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);

        DelayMs(1);

        while(I2CMasterBusy(I2C0_BASE));

        /* Idle wait */
        while(I2CGenIsNotIdle());
      }

      pbTemp++;
    }

  }

  /*Stop*/

  return 0x00;

}

/* ------------------------------------------------------------ */
/***	I2CGenIsNotIdle()
 **
 **	Parameters:
 **		pbData	-	Pointer to transmit buffer (read or write)
 **		cSize	-	Number of byte transactions to take place
 **
 **	Return Value:
 **		TRUE is bus is not idle, FALSE if bus is idle
 **
 **	Errors:
 **		none
 **
 **	Description:
 **		Returns TRUE if the bus is not idle
 **
 */
bool I2CGenIsNotIdle() {

  return !I2CMasterBusBusy(I2C0_BASE);

}










