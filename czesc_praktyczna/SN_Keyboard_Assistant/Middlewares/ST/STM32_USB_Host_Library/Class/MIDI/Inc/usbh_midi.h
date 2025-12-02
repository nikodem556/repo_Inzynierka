/**
  ******************************************************************************
  * @file    usbh_midi.h
  * @author  Nikodem Szafran
  * @brief   USB Host MIDI Class driver header (MIDI IN only).
  * @details This file contains definitions for the USB host MIDI class driver.
  *          It defines the class-specific data structures and API functions.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBH_MIDI_H
#define __USBH_MIDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"   /* USBH core structures and definitions */

/* Class Codes and Subclass for Audio/MIDI (per USB specification) */
#define USB_MIDI_CLASS_CODE         0x01  /* USB class code for Audio devices (MIDI falls under Audio class) */
#define USB_MIDI_SUBCLASS_CONTROL   0x01  /* Audio Control subclass (not used in this driver) */
#define USB_MIDI_SUBCLASS_STREAMING 0x03  /* MIDI Streaming subclass code */
#define USB_MIDI_PROTOCOL_UNDEFINED 0x00  /* Protocol (usually 0 for Audio/MIDI) */

/* Endpoint maximum packet sizes (typical for Full Speed) */
#define USBH_MIDI_MAX_PACKET_SIZE   64    /* Max packet size for MIDI endpoints (Full Speed bulk endpoints typically 64 bytes) */

/* FIFO size for incoming MIDI events */
#define USBH_MIDI_EVENT_FIFO_SIZE   64    /* Buffer size in number of bytes for storing incoming MIDI events (must be multiple of 4) */

/* MIDI Class-specific state definitions */
typedef enum {
    MIDI_IDLE = 0,    /* Idle state (no transfer in progress, ready to receive) */
    MIDI_TRANSFER,    /* Transfer in progress (waiting for USB data) */
    MIDI_ERROR        /* Error state (e.g., stalled or other error) */
} MIDI_StateTypeDef;

/* USBH MIDI Class handle structure */
typedef struct {
    uint8_t  InPipe;                /* Pipe index for IN endpoint (device -> host MIDI data) */
    uint8_t  OutPipe;               /* Pipe index for OUT endpoint (host -> device MIDI data, not used in this minimal driver) */
    uint8_t  InEp;                  /* MIDI Streaming Data IN endpoint address */
    uint8_t  OutEp;                 /* MIDI Streaming Data OUT endpoint address (if present, not used here) */
    uint16_t InEpSize;              /* Maximum packet size for IN endpoint */
    uint16_t OutEpSize;             /* Maximum packet size for OUT endpoint (if present) */
    MIDI_StateTypeDef state;        /* Current state of MIDI class (idle or transferring) */
    uint8_t  RxBuffer[USBH_MIDI_MAX_PACKET_SIZE];       /* Buffer for one USB packet of incoming MIDI data */
    uint8_t  EventFIFO[USBH_MIDI_EVENT_FIFO_SIZE];      /* FIFO buffer to store incoming MIDI event packets (4 bytes each) */
    uint16_t EventFIFOHead;         /* FIFO head index (for writing new events) */
    uint16_t EventFIFOTail;         /* FIFO tail index (for reading events) */
} MIDI_HandleTypeDef;

/* External variable for the MIDI class driver */
extern USBH_ClassTypeDef MIDI_Class;
#define USBH_MIDI_CLASS    &MIDI_Class

/* Public API functions for the MIDI class driver */
USBH_StatusTypeDef USBH_MIDI_GetEvent(USBH_HandleTypeDef *phost, uint8_t *event_buf);
/*
   USBH_MIDI_GetEvent:
   - Copies one 4-byte MIDI event from the internal FIFO into event_buf.
   - Returns USBH_OK if an event was available and copied,
     USBH_FAIL if no event is available or if the device is not ready.
*/

/* Note: No explicit send function is provided in this minimal IN-only driver. */

#ifdef __cplusplus
}
#endif

#endif /* __USBH_MIDI_H */
