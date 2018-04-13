/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "LPC17xx.h"                    // Device header
#include "RTE_Components.h"             // Component selection
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "Board_LED.h"                  // ::Board Support:LED
#include "Driver_CAN.h"                 // ::CMSIS Driver:CAN
#include "Driver_ETH_MAC.h"             // ::CMSIS Driver:Ethernet MAC
#include "Driver_ETH_PHY.h"             // ::CMSIS Driver:Ethernet PHY
#include "Driver_SPI.h"                 // ::CMSIS Driver:SPI
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "GPDMA_LPC17xx.h"              // Keil::Device:GPDMA
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "RTE_Device.h"                 // Keil::Device:Startup
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
#include "stdio.h"

extern GLCD_FONT GLCD_Font_16x24;
extern GLCD_FONT GLCD_Font_6x8;
extern   ARM_DRIVER_CAN         Driver_CAN1;

int32_t udp_sock; 
int trameth;
int trame;

osThreadId id_CANthreadR;
void CANthreadR (void const *argument);       
osThreadDef(CANthreadR,osPriorityNormal,1,0);

osThreadId id_CANthreadT;
void CANthreadT (void const *argument);       
osThreadDef(CANthreadT,osPriorityNormal,1,0);

osThreadId id_eththreadR;
void eththreadR (void const *argument);       
osThreadDef(eththreadR,osPriorityNormal,1,0);

osThreadId id_eththreadT;
void eththreadT (void const *argument);       
osThreadDef(eththreadT,osPriorityNormal,1,0);


void send_udp_data_2 (char *data, char nbr_data) {
 
  if (udp_sock > 0) {
		
    NET_ADDR addr = { NET_ADDR_IP4, 2000, 192, 168, 0, 2 };
 
    uint8_t *sendbuf;
 
    sendbuf = netUDP_GetBuffer (nbr_data);
    *sendbuf = *data;
		*(sendbuf+1) = *(data+1);
    netUDP_Send (udp_sock, &addr, sendbuf, nbr_data);
    
  }
}
void send_udp_data_3 (char *data, char nbr_data) {
 
  if (udp_sock > 0) {
		
    NET_ADDR addr = { NET_ADDR_IP4, 2000, 192, 168, 0, 8 };
 
    uint8_t *sendbuf;
 
    sendbuf = netUDP_GetBuffer (nbr_data);
    *sendbuf = *data;
		*(sendbuf+1) = *(data+1);
    netUDP_Send (udp_sock, &addr, sendbuf, nbr_data);
	}
}




uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, const uint8_t *buf, uint32_t len) {
	char ID[10];
	trameth = buf[0];
	osSignalSet(id_eththreadR,0x001);
	sprintf(ID,"%02X", trameth);
	GLCD_DrawString(10,10,ID);
 	LED_On(4);
  return (0);	
}

 void myCAN1_callback(uint32_t obj_idx, uint32_t event){
      switch (event)
    {
    case ARM_CAN_EVENT_RECEIVE:
        /*  Message was received successfully by the obj_idx object. */
       osSignalSet(id_CANthreadR, 0x001);
        break;
    }
}

void InitCan1 (void){
	Driver_CAN1.Initialize(NULL,myCAN1_callback);
	Driver_CAN1.PowerControl(ARM_POWER_FULL);
	
	Driver_CAN1.SetMode(ARM_CAN_MODE_INITIALIZATION);
	Driver_CAN1.SetBitrate( ARM_CAN_BITRATE_NOMINAL,
													125000,
													ARM_CAN_BIT_PROP_SEG(5U)   |         // Set propagation segment to 5 time quanta
                          ARM_CAN_BIT_PHASE_SEG1(1U) |         // Set phase segment 1 to 1 time quantum (sample point at 87.5% of bit time)
                          ARM_CAN_BIT_PHASE_SEG2(1U) |         // Set phase segment 2 to 1 time quantum (total bit is 8 time quanta long)
                          ARM_CAN_BIT_SJW(1U));                // Resynchronization jump width is same as phase segment 2
                          
	// Mettre ici les filtres ID de réception sur objet 0
	Driver_CAN1.ObjectSetFilter(0,ARM_CAN_FILTER_ID_EXACT_ADD,ARM_CAN_STANDARD_ID(0x5F8),0);
	Driver_CAN1.ObjectConfigure(1,ARM_CAN_OBJ_TX);	
	Driver_CAN1.ObjectConfigure(0,ARM_CAN_OBJ_RX);				// Objet 0 du CAN1 pour réception
	
	Driver_CAN1.SetMode(ARM_CAN_MODE_NORMAL);					// fin init
}








void eththreadR (void const *argument){
		LED_On(0);
		while(1){
			osSignalWait(0x001,osWaitForever);
			osSignalSet(id_CANthreadT,0x001);
	}
}


void CANthreadT (void const *argument){
		uint8_t envoi_buf[1];
		ARM_CAN_MSG_INFO   tx_msg_info;
		tx_msg_info.id = ARM_CAN_STANDARD_ID(0x5F8);
		tx_msg_info.rtr = 0;
		LED_On(1);
		while(1){
			osSignalWait(0x001,osWaitForever);
			Driver_CAN1.MessageSend(1,&tx_msg_info,envoi_buf,1);
	}
}


void CANthreadR (void const *argument){
		uint8_t data_buf[1];
		char textr[20];
		ARM_CAN_MSG_INFO   rx_msg_info;
		LED_On(2);
		while(1){
			osSignalWait(0x001,osWaitForever);
			Driver_CAN1.MessageRead(0,&rx_msg_info,data_buf,1);
			trame = data_buf[0];
			sprintf(textr,"RECEIVE %02X",trame);
			GLCD_DrawString(10,100,textr);
			osSignalSet(id_eththreadT,0x001);
	}
}


void eththreadT (void const *argument){
	char youyouJ = 0xBB;
	char youyouM = 0xBA;
	char youyouCC = 0xCC;
	char txt[30];
	char envoi[2];
	LED_On(3);
		while(1){
	envoi[0] = 0;
	envoi[1] = 0;
		osSignalWait(0x001,osWaitForever);
	envoi[0] = trameth;
	envoi[1] = trame;
	sprintf(txt,"ENVOI %02X,%02x",trameth,trame);
		if (trameth == youyouJ){
		send_udp_data_2(envoi,2);
		GLCD_DrawString(10,150,txt);
		}
		else if (trameth == youyouM){
		send_udp_data_3(envoi,2);
		GLCD_DrawString(10,150,txt);
		}
			else if (trameth == youyouCC){
		send_udp_data_3(envoi,2);
		GLCD_DrawString(10,150,txt);
			}
			
		}
		}


int main (void) {
  osKernelInitialize ();
	InitCan1();
	GLCD_Initialize();
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_6x8);
	LED_Initialize();		
	netInitialize();
	LED_On(5);
	
	udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
  netUDP_Open (udp_sock, 2000);}

		id_eththreadR = osThreadCreate (osThread(eththreadR), NULL);
		id_eththreadT = osThreadCreate (osThread(eththreadT), NULL);
		id_CANthreadR = osThreadCreate (osThread(CANthreadR), NULL);
		id_CANthreadT = osThreadCreate (osThread(CANthreadT), NULL);

  osKernelStart ();                         // start thread execution 
	osDelay(osWaitForever);
}
