/**
  ******************************************************************************
  * @file    usbh_midi.c
  * @author  Nikodem Szafran
  * @brief   USB Host MIDI Class driver (MIDI IN only, polling mode).
  * @details This file implements a minimal USB host class driver for MIDI devices.
  *          It supports receiving MIDI messages from a connected USB-MIDI device.
  *          The driver is designed for STM32Cube HAL USB Host stack.
  *          - Only MIDI IN is handled (device -> host).
  *          - No MIDI OUT or transmission from host is implemented.
  *          - Uses polling in USBH_Process (no RTOS required).
  *          - The device is assumed to be self-powered (does not require VBUS power).
  ******************************************************************************
  */

#include "usbh_midi.h"
#include <string.h>  /* For memset */

/* Internal function prototypes for class handler callbacks */
static USBH_StatusTypeDef USBH_MIDI_Init(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_DeInit(USBH_HandleTypeDef *phost);

/* MIDI Class structure definition for USB host */
USBH_ClassTypeDef MIDI_Class = {
    "MIDI",                  /* Name of the class (for debug purposes) */
    USB_MIDI_CLASS_CODE,     /* Class code (Audio class code for MIDI devices) */
    USBH_MIDI_Init,          /* Initialization callback */
    USBH_MIDI_DeInit,        /* De-initialization callback */
    USBH_MIDI_ClassRequest,  /* Class-specific requests (not used in this minimal driver) */
    USBH_MIDI_Process,       /* Data process callback (called in main host loop) */
    USBH_MIDI_SOFProcess     /* SOF (Start of Frame) callback, optional (not used here) */
};

/**
  * @brief  USBH_MIDI_Init
  *         Initializes the USB MIDI class on device connection.
  * @param  phost: pointer to the USB host handle
  * @retval USBH_StatusTypeDef: USBH_OK if successful, otherwise USBH_FAIL
  */
static USBH_StatusTypeDef USBH_MIDI_Init(USBH_HandleTypeDef *phost)
{
    USBH_StatusTypeDef status = USBH_FAIL;
    MIDI_HandleTypeDef *MIDI_Handle;

    /* Allocate memory for MIDI class handle */
    MIDI_Handle = (MIDI_HandleTypeDef *)USBH_malloc(sizeof(MIDI_HandleTypeDef));
    if (MIDI_Handle == NULL) {
        return USBH_FAIL;  /* Allocation failed */
    }
    /* Clear the allocated handle structure */
    memset(MIDI_Handle, 0, sizeof(MIDI_HandleTypeDef));

    /* Assign the class-specific handle to the host's active class data pointer */
    phost->pActiveClass->pData = (void *)MIDI_Handle;

    /* The device may have multiple interfaces (Audio Control and MIDI Streaming).
       We need to find the MIDI Streaming interface (Audio subclass 0x03). */
    uint8_t interface = 0xFF;
    for (uint8_t idx = 0; idx < phost->device.CfgDesc.bNumInterfaces; idx++) {
        /* Check for Audio class (0x01) and MIDI Streaming subclass (0x03) */
        if ((phost->device.CfgDesc.Itf_Desc[idx].bInterfaceClass == USB_MIDI_CLASS_CODE) &&
            (phost->device.CfgDesc.Itf_Desc[idx].bInterfaceSubClass == USB_MIDI_SUBCLASS_STREAMING)) {
            interface = idx;
            break;
        }
    }
    if (interface == 0xFF) {
        /* No MIDI Streaming interface found, return failure */
        return USBH_FAIL;
    }

    /* Get interface descriptor for the MIDI Streaming interface */
    USBH_InterfaceDescTypeDef *itf_desc = &phost->device.CfgDesc.Itf_Desc[interface];
    /* Parse endpoints for this interface */
    for (uint8_t ep_idx = 0; ep_idx < itf_desc->bNumEndpoints; ep_idx++) {
        USBH_EpDescTypeDef *ep_desc = &itf_desc->Ep_Desc[ep_idx];
        uint8_t ep_addr = ep_desc->bEndpointAddress;
        uint8_t ep_type = ep_desc->bmAttributes & 0x03U;  /* Mask to get EP transfer type (bits 1..0) */

        if ((ep_addr & 0x80U) && (ep_type == 0x02U || ep_type == 0x03U)) {
            /* IN endpoint found. MIDI uses Bulk (0x02) or Interrupt (0x03) for IN according to spec.
               Here we handle both as IN data source for MIDI messages. */
            MIDI_Handle->InEp = ep_addr;
            MIDI_Handle->InEpSize = ep_desc->wMaxPacketSize;
            /* Allocate a pipe for IN endpoint and open it */
            MIDI_Handle->InPipe = USBH_AllocPipe(phost, MIDI_Handle->InEp);
            USBH_OpenPipe(phost, MIDI_Handle->InPipe, MIDI_Handle->InEp,
                          phost->device.address, phost->device.speed,
                          ep_type == 0x02U ? USBH_EP_BULK : USBH_EP_INTR,
                          MIDI_Handle->InEpSize);
            USBH_LL_SetToggle(phost, MIDI_Handle->InPipe, 0);  /* Initialize data toggle for IN pipe to 0 */
        }
        else if (!(ep_addr & 0x80U) && (ep_type == 0x02U || ep_type == 0x03U)) {
            /* OUT endpoint found (host-to-device MIDI OUT, not used in this minimal driver) */
            MIDI_Handle->OutEp = ep_addr;
            MIDI_Handle->OutEpSize = ep_desc->wMaxPacketSize;
            /* Allocate and open pipe for OUT endpoint (opened for completeness, but we will not send data) */
            MIDI_Handle->OutPipe = USBH_AllocPipe(phost, MIDI_Handle->OutEp);
            USBH_OpenPipe(phost, MIDI_Handle->OutPipe, MIDI_Handle->OutEp,
                          phost->device.address, phost->device.speed,
                          ep_type == 0x02U ? USBH_EP_BULK : USBH_EP_INTR,
                          MIDI_Handle->OutEpSize);
            USBH_LL_SetToggle(phost, MIDI_Handle->OutPipe, 0); /* Initialize data toggle for OUT pipe to 0 */
        }
    }

    /* Initialize FIFO indices */
    MIDI_Handle->EventFIFOHead = 0;
    MIDI_Handle->EventFIFOTail = 0;
    MIDI_Handle->state = MIDI_IDLE;

    /* Indicate successful initialization */
    status = USBH_OK;
    return status;
}

/**
  * @brief  USBH_MIDI_DeInit
  *         De-initializes the USB MIDI class when the device is disconnected.
  * @param  phost: pointer to the USB host handle
  * @retval USBH_StatusTypeDef: USBH_OK (de-init always succeeds)
  */
static USBH_StatusTypeDef USBH_MIDI_DeInit(USBH_HandleTypeDef *phost)
{
    MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *) phost->pActiveClass->pData;

    if (MIDI_Handle != NULL) {
        /* Close open pipes */
        if (MIDI_Handle->InPipe) {
            USBH_ClosePipe(phost, MIDI_Handle->InPipe);
            USBH_FreePipe(phost, MIDI_Handle->InPipe);
            MIDI_Handle->InPipe = 0;
        }
        if (MIDI_Handle->OutPipe) {
            USBH_ClosePipe(phost, MIDI_Handle->OutPipe);
            USBH_FreePipe(phost, MIDI_Handle->OutPipe);
            MIDI_Handle->OutPipe = 0;
        }

        /* Free the MIDI class handle memory */
        USBH_free(MIDI_Handle);
        phost->pActiveClass->pData = NULL;
    }
    return USBH_OK;
}

/**
  * @brief  USBH_MIDI_ClassRequest
  *         Handles MIDI class-specific requests (none needed for basic MIDI).
  * @param  phost: pointer to the USB host handle
  * @retval USBH_StatusTypeDef: USBH_OK (since no class requests are required)
  */
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
    /* No specific class requests (like SetInterface) are needed for MIDI in this minimal implementation.
       Simply return OK to proceed with enumeration. */
    return USBH_OK;
}

/**
  * @brief  USBH_MIDI_Process
  *         Polling process for receiving MIDI data from the device.
  * @param  phost: pointer to the USB host handle
  * @retval USBH_StatusTypeDef: USBH_OK or USBH_BUSY depending on processing
  */
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost)
{
    MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *) phost->pActiveClass->pData;
    USBH_StatusTypeDef status = USBH_OK;
    uint32_t urb_state;
    uint32_t length;

    if (MIDI_Handle == NULL) {
        return USBH_FAIL;
    }

    switch (MIDI_Handle->state) {
    case MIDI_IDLE:
        /* No ongoing transfer, initiate a new IN transfer to read MIDI data */
        USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer, MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
        MIDI_Handle->state = MIDI_TRANSFER;
        status = USBH_BUSY;
        break;

    case MIDI_TRANSFER:
        /* Check the state of the last USB transfer */
        urb_state = USBH_LL_GetURBState(phost, MIDI_Handle->InPipe);
        if (urb_state == USBH_URB_DONE) {
            /* Data packet received successfully */
            length = USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);
            if (length > 0 && length <= USBH_MIDI_MAX_PACKET_SIZE) {
                /* Copy received data (one or multiple MIDI events) into the FIFO */
                uint32_t i = 0;
                while (i < length && (length - i) >= 4) {
                    /* Ensure we have space for 4 bytes (one MIDI event) in the FIFO */
                    uint16_t nextHead = (MIDI_Handle->EventFIFOHead + 4) % USBH_MIDI_EVENT_FIFO_SIZE;
                    if (nextHead == MIDI_Handle->EventFIFOTail) {
                        /* FIFO is full, drop this event (overflow) */
                        // Optionally, one could overwrite oldest data by advancing tail.
                        // Here we simply break to avoid overflow.
                        break;
                    }
                    /* Copy 4 bytes of one MIDI event from RxBuffer into the FIFO */
                    memcpy(&MIDI_Handle->EventFIFO[MIDI_Handle->EventFIFOHead],
                           &MIDI_Handle->RxBuffer[i],
                           4);
                    /* Update head index to next position */
                    MIDI_Handle->EventFIFOHead = nextHead;
                    i += 4;  /* Move to the next 4-byte event in the received packet */
                }
            }
            /* Prepare to receive the next packet immediately */
            USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer, MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
            /* Stay in TRANSFER state (continuous polling) */
            status = USBH_OK;
        }
        else if (urb_state == USBH_URB_STALL) {
            /* If the device stalled the IN endpoint, clear the stall condition */
            USBH_ClrFeature(phost, MIDI_Handle->InEp);
            /* After clearing stall, re-submit a new read */
            USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer, MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
            status = USBH_OK;
        }
        else if (urb_state == USBH_URB_ERROR) {
            /* An error happened during transfer (e.g., device disconnected unexpectedly) */
            MIDI_Handle->state = MIDI_ERROR;
            status = USBH_FAIL;
        }
        else {
            /* Transfer not yet complete (USBH_URB_BUSY), keep waiting */
            status = USBH_BUSY;
        }
        break;

    case MIDI_ERROR:
        /* In case of error, try to reinitialize or just report fail */
        // For simplicity, we do not implement recovery here.
        status = USBH_FAIL;
        break;

    default:
        status = USBH_FAIL;
        break;
    }

    return status;
}

/**
  * @brief  USBH_MIDI_SOFProcess
  *         Start-of-Frame process callback (not used in this driver, provided for completeness).
  * @param  phost: pointer to the USB host handle
  * @retval USBH_StatusTypeDef: always USBH_OK in this implementation
  */
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)
{
    /* No periodic SOF processing is needed for this polling driver */
    return USBH_OK;
}

/**
  * @brief  USBH_MIDI_GetEvent
  *         Retrieves one MIDI event (4 bytes) from the internal FIFO (if available).
  * @param  phost: pointer to the USB host handle
  * @param  event_buf: pointer to a buffer (at least 4 bytes) to store the MIDI event
  * @retval USBH_StatusTypeDef: USBH_OK if an event was retrieved, USBH_FAIL if no event is available
  */
USBH_StatusTypeDef USBH_MIDI_GetEvent(USBH_HandleTypeDef *phost, uint8_t *event_buf)
{
    MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *) phost->pActiveClass->pData;
    if (MIDI_Handle == NULL) {
        return USBH_FAIL;  /* Class not initialized or device not connected */
    }
    /* Check if FIFO has data */
    if (MIDI_Handle->EventFIFOHead == MIDI_Handle->EventFIFOTail) {
        /* FIFO is empty, no MIDI event available */
        return USBH_FAIL;
    }

    /* Copy one 4-byte event from FIFO to user buffer */
    for (uint8_t j = 0; j < 4; j++) {
        event_buf[j] = MIDI_Handle->EventFIFO[MIDI_Handle->EventFIFOTail + j];
    }
    /* Advance the tail index by 4 bytes (one event) */
    MIDI_Handle->EventFIFOTail = (MIDI_Handle->EventFIFOTail + 4) % USBH_MIDI_EVENT_FIFO_SIZE;

    return USBH_OK;
}
