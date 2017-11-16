/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

#include "ADXL362.h"

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>
#include <ti/drivers/dma/UDMACC26XX.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
// #include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "Board.h"

#define TASKSTACKSIZE   1024

/* SPI Board */

#define CHIP_SELECT             IOID_11

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Pin driver handle */
static PIN_Handle csPinHandle;
static PIN_State csPinState;

static SPI_Handle       spi;
static SPI_Params       spiParams;
static UInt             transferOK;
static SPI_Transaction  spiTransaction;

#define SPI_MSG_LENGTH    3
unsigned char masterRxBuffer[SPI_MSG_LENGTH] = {0,0,0};
unsigned char masterTxBuffer[SPI_MSG_LENGTH] = {0,0,0};

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/
/*! Writes data into a register. */
void ADXL362_WriteRegisterValue(  unsigned char registerValue,
                                  unsigned char registerAddress);

/*! Performs a burst read of a specified number of registers. */
void ADXL362_ReadRegisterValue(  unsigned char   registerAddress );

/*! Resets the device via SPI communication bus. */
void ADXL362_SoftwareReset(void);

/*! Places the device into standby/measure mode. */
void ADXL362_SetPowerMode(unsigned char standbyORmeasure);

void ADXL362_ReadXYZ(unsigned char* x, unsigned char* y, unsigned char* z);

void ADXL362_fallDetection(void);

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config CSpinTable[] = {

    CHIP_SELECT | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/*
 *  ======== ADXL362 Fxn ========
 *
 */
Void ADXL362Fxn(UArg arg0, UArg arg1)
{
    unsigned char xdata;
    unsigned char ydata;
    unsigned char zdata;
        SPI_Params_init(&spiParams);
        spi = SPI_open(Board_SPI0, &spiParams);
        if (spi == NULL) {
                System_abort("Error initializing SPI\n");
            }
            else {
                System_printf("SPI initialized\n");
            }
        spiTransaction.count = SPI_MSG_LENGTH;
        spiTransaction.txBuf = (Ptr)masterTxBuffer;
        spiTransaction.rxBuf = (Ptr)masterRxBuffer;
        ADXL362_SoftwareReset();


        //sets free fall threshold to 600 mg.
        ADXL362_WriteRegisterValue(ADXL362_REG_THRESH_INACT_L, 0x96);

        //sets free fall time to 30 ms
        ADXL362_WriteRegisterValue(ADXL362_REG_TIME_INACT_L, 0x03);

        //enables absolute inactivity detection.
        ADXL362_WriteRegisterValue(ADXL362_REG_ACT_INACT_CTL, 0x04);

        //map the inactivity interrupt to INT1
        ADXL362_WriteRegisterValue(ADXL362_REG_INTMAP1, 0x20);

        //±8 g range, 100 Hz ODR
        ADXL362_WriteRegisterValue(ADXL362_REG_FILTER_CTL, 0x93);

        //begin measurement
        ADXL362_SetPowerMode(ADXL362_REG_PARTID);

        System_printf("setup Done\n");

        System_flush();



//        ADXL362_fallDetection();
//        ADXL362_SetPowerMode(ADXL362_REG_PARTID);
while(1){

        //Soft Reset

      ADXL362_ReadXYZ(xdata,ydata,zdata);
//    ADXL362_ReadRegisterValue(ADXL362_REG_XDATA_L);
//        System_printf("Done\n");

        System_flush();
}
        SPI_close(spi);


}

/*
 *  ======== main ========
 */
int main(void)
{
    //Board_initGeneral();

    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();
     //Board_initI2C();
     Board_initSPI();

     /* Open LED pins */
     csPinHandle = PIN_open(&csPinState, CSpinTable);
     if(!csPinHandle) {
         System_abort("Error initializing board LED pins\n");
     }

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)ADXL362Fxn, &taskParams, NULL);


    System_printf("Starting the example\nSystem provider is set to SysMin. "
                  "Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}



/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

void ADXL362_SoftwareReset(void){
    ADXL362_WriteRegisterValue(ADXL362_REG_SOFT_RESET, ADXL362_RESET_KEY);
}

void ADXL362_SetPowerMode(unsigned char standbyORmeasure){
    ADXL362_WriteRegisterValue(ADXL362_REG_POWER_CTL, standbyORmeasure);
}

void ADXL362_WriteRegisterValue(  unsigned char  registerValue,
                                  unsigned char   registerAddress)
{
    PIN_setOutputValue(csPinHandle, CHIP_SELECT, 0);
    masterTxBuffer[0] = ADXL362_WRITE_REG;
    masterTxBuffer[1] = registerValue;
    masterTxBuffer[2] = registerAddress;

    transferOK = SPI_transfer(spi, &spiTransaction);

    if(transferOK) {
        System_printf("Transfer Done\n");
    }
    else {
        System_printf("Unsuccessful master SPI transfer");
    }
    PIN_setOutputValue(csPinHandle, CHIP_SELECT, 1);

    Task_sleep(50*1000/Clock_tickPeriod); //sleep 10ms
}


void ADXL362_ReadRegisterValue(  unsigned char   registerAddress )
{

    PIN_setOutputValue(csPinHandle, IOID_11, 0);
    masterTxBuffer[0] = ADXL362_READ_REG;
    masterTxBuffer[1] = registerAddress;
    masterTxBuffer[2] = 0x00;

    transferOK = SPI_transfer(spi, &spiTransaction);

    if(transferOK) {
        System_printf("1111Master: %d\n", masterRxBuffer[2]);
    }
    else {
        System_printf("Unsuccessful master SPI transfer");
    }

    PIN_setOutputValue(csPinHandle, IOID_11, 1);
    Task_sleep(50*1000/Clock_tickPeriod); //sleep 50ms
}

void ADXL362_ReadXYZ(unsigned char* x, unsigned char* y, unsigned char* z)
{
    unsigned char masterRxBuffer[10] = {0,0,0,0,0,0,0,0,0,0};
    spiTransaction.count = 10;
    spiTransaction.rxBuf = (Ptr)masterRxBuffer;
    PIN_setOutputValue(csPinHandle, IOID_11, 0);

    ADXL362_ReadRegisterValue(ADXL362_REG_XDATA_L);

    int temp = 0.065 * ((masterRxBuffer[9]<<8)+masterRxBuffer[8]);
    // /1000(8/2)
//    *x = masterRxBuffer[2] ;
    System_printf("xvalue: %d", (masterRxBuffer[3]<<8)+masterRxBuffer[2]);
    System_printf("\tyvalue: %d", (masterRxBuffer[5]<<8)+masterRxBuffer[4]);
    System_printf("\tzvalue: %d", (masterRxBuffer[7]<<8)+masterRxBuffer[6]);
    System_printf("\ttemp: %d\n", temp);
    PIN_setOutputValue(csPinHandle, IOID_11, 1);
    Task_sleep(50*1000/Clock_tickPeriod); //sleep 50ms
}

void ADXL362_fallDetection(void){
    //sets free fall threshold to 600 mg.
    ADXL362_WriteRegisterValue(ADXL362_REG_THRESH_INACT_L, 0x96);

    //sets free fall time to 30 ms
    ADXL362_WriteRegisterValue(ADXL362_REG_TIME_INACT_L, 0x03);

    //enables absolute inactivity detection.
    ADXL362_WriteRegisterValue(ADXL362_REG_ACT_INACT_CTL, 0x04);

    //map the inactivity interrupt to INT1
    ADXL362_WriteRegisterValue(ADXL362_REG_INTMAP1, 0x20);

    //±8 g range, 100 Hz ODR
    ADXL362_WriteRegisterValue(ADXL362_REG_FILTER_CTL, 0x93);

    //begin measurement
    ADXL362_WriteRegisterValue(ADXL362_REG_POWER_CTL, 0x20);

    System_printf("Done\n");

    System_flush();
}

