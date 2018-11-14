/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xiicps.h"


int main()
{
    int Status;
    XIicPs_Config *Config;
    XIicPs Iic;
    u8 data;
    u8 v0[2];
    u8 v1[2];

    init_platform();

    print("Hello World\n\r");

    Config = XIicPs_LookupConfig(XPAR_XIICPS_0_DEVICE_ID);
    if (NULL == Config) {
        xil_printf("ERROR(%d):NULL\n\r", __LINE__);
        goto finally;
    }

    Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }

    Status = XIicPs_SelfTest(&Iic);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }

    XIicPs_SetSClk(&Iic, 100000);

    data = 0x04;
    Status = XIicPs_MasterSendPolled(&Iic, &data, 1, 0x75);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }

    data = 0x5a;
    Status = XIicPs_MasterRecvPolled(&Iic, &data, 1, 0x75);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }

    v0[0] = 0x30;
    v0[1] = 0x0A;
    Status = XIicPs_MasterSendPolled(&Iic, v0, 2, 0x3c);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }
    Status = XIicPs_MasterRecvPolled(&Iic, &v0[0], 1, 0x3c);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }

    v1[0] = 0x30;
    v1[1] = 0x0B;
    Status = XIicPs_MasterSendPolled(&Iic, v1, 2, 0x3c);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }
    Status = XIicPs_MasterRecvPolled(&Iic, &v1[0], 1, 0x3c);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR(%d):%d\n\r", __LINE__, Status);
        goto finally;
    }
   
    xil_printf("0x%x 0x%x\n\r", (u32)v0[0], (u32)v1[0]);

finally:
    cleanup_platform();
    return 0;
}
