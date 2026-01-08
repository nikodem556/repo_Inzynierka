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

/*
 * This project implements a minimal USB Host class for MIDI Streaming (IN only).
 *
 * USB-MIDI transport uses 4-byte "USB-MIDI Event Packets":
 *   [0] Cable Number + Code Index Number (CIN)
 *   [1] MIDI status byte
 *   [2] MIDI data byte 1
 *   [3] MIDI data byte 2
 *
 * The driver stores received packets in an internal FIFO (byte buffer),
 * where events are appended and read in chunks of 4 bytes.
 */

/* Class Codes and Subclass for Audio/MIDI (per USB specification) */
#define USB_MIDI_CLASS_CODE         0x01  /* Audio device class (MIDI falls under Audio) */
#define USB_MIDI_SUBCLASS_CONTROL   0x01  /* Audio Control subclass (not used in this driver) */
#define USB_MIDI_SUBCLASS_STREAMING 0x03  /* MIDI Streaming subclass code */
#define USB_MIDI_PROTOCOL_UNDEFINED 0x00  /* Protocol (typically 0 for Audio/MIDI) */

/* Endpoint maximum packet sizes (typical for Full Speed) */
#define USBH_MIDI_MAX_PACKET_SIZE   64    /* Typical FS bulk max packet size: 64 bytes */

/*
 * FIFO size for incoming MIDI events (in BYTES).
 * IMPORTANT: must be a multiple of 4, because events are 4-byte packets.
 */
#define USBH_MIDI_EVENT_FIFO_SIZE   64

/* MIDI Class-specific state definitions */
typedef enum {
    MIDI_IDLE = 0,    /* Ready to start a new IN transfer */
    MIDI_TRANSFER,    /* IN transfer is active; waiting for URB completion */
    MIDI_ERROR        /* Error state (e.g., unrecoverable transfer error) */
} MIDI_StateTypeDef;

/**
 * @brief USBH MIDI Class runtime handle.
 *
 * Fields:
 * - InPipe/InEp/InEpSize: host pipe and endpoint for device->host data
 * - OutPipe/OutEp/OutEpSize: optional host->device endpoint (not used by this IN-only API)
 * - RxBuffer: one USB packet buffer used by USBH_BulkReceiveData()
 * - EventFIFO: queue storing 4-byte USB-MIDI event packets
 * - EventFIFOHead/Tail: circular buffer indices (in bytes, step = 4)
 */
typedef struct {
    uint8_t  InPipe;                /* Pipe index for IN endpoint (device -> host) */
    uint8_t  OutPipe;               /* Pipe index for OUT endpoint (host -> device), unused here */
    uint8_t  InEp;                  /* MIDI Streaming Data IN endpoint address */
    uint8_t  OutEp;                 /* MIDI Streaming Data OUT endpoint address (if present) */
    uint16_t InEpSize;              /* Maximum packet size for IN endpoint */
    uint16_t OutEpSize;             /* Maximum packet size for OUT endpoint */
    MIDI_StateTypeDef state;        /* Current class state */
    uint8_t  RxBuffer[USBH_MIDI_MAX_PACKET_SIZE];       /* Buffer for one incoming USB packet */
    uint8_t  EventFIFO[USBH_MIDI_EVENT_FIFO_SIZE];      /* FIFO of 4-byte USB-MIDI events */
    uint16_t EventFIFOHead;         /* FIFO write index (byte offset) */
    uint16_t EventFIFOTail;         /* FIFO read index (byte offset) */
} MIDI_HandleTypeDef;

/* External variable for the MIDI class driver */
extern USBH_ClassTypeDef MIDI_Class;
#define USBH_MIDI_CLASS    &MIDI_Class

/**
 * @brief Pop one 4-byte USB-MIDI event packet from the internal FIFO.
 *
 * @param phost     USBH host handle.
 * @param event_buf Output buffer (must be at least 4 bytes).
 *
 * @retval USBH_OK   if one event was available and copied
 * @retval USBH_FAIL if FIFO is empty or class is not ready
 */
USBH_StatusTypeDef USBH_MIDI_GetEvent(USBH_HandleTypeDef *phost, uint8_t *event_buf);

/* Note: No explicit send function is provided in this minimal IN-only driver. */

#ifdef __cplusplus
}
#endif

#endif /* __USBH_MIDI_H */
