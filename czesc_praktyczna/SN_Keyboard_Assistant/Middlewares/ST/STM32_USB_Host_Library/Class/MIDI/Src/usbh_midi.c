/**
  * @file    usbh_midi.c
  * @brief   USB Host MIDI Class driver (MIDI IN only, polling mode).
  * @author  Nikodem Szafran
  */
#include "usbh_midi.h"
#include <string.h>
#include <stdio.h>

/* Internal function prototypes */
static USBH_StatusTypeDef USBH_MIDI_Init(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_DeInit(USBH_HandleTypeDef *phost);

/* MIDI Class structure for USB host */
USBH_ClassTypeDef MIDI_Class = {
    "MIDI",
    USB_MIDI_CLASS_CODE,
    USBH_MIDI_Init,
    USBH_MIDI_DeInit,
    USBH_MIDI_ClassRequest,
    USBH_MIDI_Process,
    USBH_MIDI_SOFProcess
};

/* USBH_MIDI_Init: called upon device connection for MIDI class */
static USBH_StatusTypeDef USBH_MIDI_Init(USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_FAIL;
  MIDI_HandleTypeDef *MIDI_Handle;

  /* Allocate memory for MIDI class handle */
  MIDI_Handle = (MIDI_HandleTypeDef *)USBH_malloc(sizeof(MIDI_HandleTypeDef));
  if (MIDI_Handle == NULL)
  {
    printf("USBH_MIDI_Init: Failed to allocate MIDI class handle\r\n");
    return USBH_FAIL;
  }
  memset(MIDI_Handle, 0, sizeof(MIDI_HandleTypeDef));
  phost->pActiveClass->pData = (void *)MIDI_Handle;

  /* Find the MIDI Streaming interface (Audio class 0x01, subclass 0x03) */
  uint8_t interface = 0xFF;
  for (uint8_t idx = 0; idx < phost->device.CfgDesc.bNumInterfaces; idx++)
  {
    if ((phost->device.CfgDesc.Itf_Desc[idx].bInterfaceClass == USB_MIDI_CLASS_CODE) &&
        (phost->device.CfgDesc.Itf_Desc[idx].bInterfaceSubClass == USB_MIDI_SUBCLASS_STREAMING))
    {
      interface = idx;
      break;
    }
  }
  if (interface == 0xFF)
  {
    printf("USBH_MIDI_Init: No MIDI Streaming interface found\r\n");
    return USBH_FAIL;
  }
  printf("USBH_MIDI_Init: MIDI Streaming interface found at index %d\r\n", interface);

  /* Get the interface descriptor and parse its endpoints */
  USBH_InterfaceDescTypeDef *itf_desc = &phost->device.CfgDesc.Itf_Desc[interface];
  for (uint8_t ep_idx = 0; ep_idx < itf_desc->bNumEndpoints; ep_idx++)
  {
    USBH_EpDescTypeDef *ep_desc = &itf_desc->Ep_Desc[ep_idx];
    uint8_t ep_addr = ep_desc->bEndpointAddress;
    uint8_t ep_type = ep_desc->bmAttributes & 0x03U;  // lower 2 bits indicate type
    if ((ep_addr & 0x80U) && (ep_type == 0x02U))
    {
      /* MIDI IN endpoint (Bulk IN) */
      MIDI_Handle->InEp = ep_addr;
      MIDI_Handle->InEpSize = ep_desc->wMaxPacketSize;
      MIDI_Handle->InPipe = USBH_AllocPipe(phost, MIDI_Handle->InEp);
      USBH_OpenPipe(phost, MIDI_Handle->InPipe, MIDI_Handle->InEp,
                    phost->device.address, phost->device.speed,
                    USBH_EP_BULK, MIDI_Handle->InEpSize);
      USBH_LL_SetToggle(phost, MIDI_Handle->InPipe, 0);
      printf("USBH_MIDI_Init: Bulk IN endpoint 0x%02X (pipe %d) opened, max packet %d bytes\r\n",
             MIDI_Handle->InEp, MIDI_Handle->InPipe, MIDI_Handle->InEpSize);
    }
    else if (!(ep_addr & 0x80U) && (ep_type == 0x02U))
    {
      /* MIDI OUT endpoint (Bulk OUT) */
      MIDI_Handle->OutEp = ep_addr;
      MIDI_Handle->OutEpSize = ep_desc->wMaxPacketSize;
      MIDI_Handle->OutPipe = USBH_AllocPipe(phost, MIDI_Handle->OutEp);
      USBH_OpenPipe(phost, MIDI_Handle->OutPipe, MIDI_Handle->OutEp,
                    phost->device.address, phost->device.speed,
                    USBH_EP_BULK, MIDI_Handle->OutEpSize);
      USBH_LL_SetToggle(phost, MIDI_Handle->OutPipe, 0);
      printf("USBH_MIDI_Init: Bulk OUT endpoint 0x%02X (pipe %d) opened, max packet %d bytes\r\n",
             MIDI_Handle->OutEp, MIDI_Handle->OutPipe, MIDI_Handle->OutEpSize);
    }
  }

  /* Initialize FIFO and state */
  MIDI_Handle->EventFIFOHead = 0;
  MIDI_Handle->EventFIFOTail = 0;
  MIDI_Handle->state = MIDI_IDLE;
  printf("USBH_MIDI_Init: MIDI FIFO initialized (head=%u, tail=%u)\r\n",
         MIDI_Handle->EventFIFOHead, MIDI_Handle->EventFIFOTail);

  /* Indicate successful initialization */
  printf("USBH_MIDI_Init: MIDI class driver initialized successfully\r\n");
  status = USBH_OK;
  return status;
}

/* USBH_MIDI_DeInit: called on device disconnection for cleanup */
static USBH_StatusTypeDef USBH_MIDI_DeInit(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  if (MIDI_Handle != NULL)
  {
    /* Close and free IN pipe */
    if (MIDI_Handle->InPipe)
    {
      printf("USBH_MIDI_DeInit: Closing InPipe %d (EP 0x%02X)\r\n",
             MIDI_Handle->InPipe, MIDI_Handle->InEp);
      USBH_ClosePipe(phost, MIDI_Handle->InPipe);
      USBH_FreePipe(phost, MIDI_Handle->InPipe);
      MIDI_Handle->InPipe = 0;
    }
    /* Close and free OUT pipe */
    if (MIDI_Handle->OutPipe)
    {
      printf("USBH_MIDI_DeInit: Closing OutPipe %d (EP 0x%02X)\r\n",
             MIDI_Handle->OutPipe, MIDI_Handle->OutEp);
      USBH_ClosePipe(phost, MIDI_Handle->OutPipe);
      USBH_FreePipe(phost, MIDI_Handle->OutPipe);
      MIDI_Handle->OutPipe = 0;
    }
    /* Free MIDI class handle */
    USBH_free(MIDI_Handle);
    phost->pActiveClass->pData = NULL;
    printf("USBH_MIDI_DeInit: Freed MIDI class handle memory\r\n");
    printf("USBH_MIDI_DeInit: De-initialization complete\r\n");
  }
  return USBH_OK;
}

/* USBH_MIDI_ClassRequest: no class-specific requests needed for basic MIDI */
static USBH_StatusTypeDef USBH_MIDI_ClassRequest(USBH_HandleTypeDef *phost)
{
  /* No special control requests; simply return OK */
  return USBH_OK;
}

/* USBH_MIDI_Process: polling handler for incoming MIDI data */
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  USBH_StatusTypeDef status = USBH_OK;
  uint32_t urb_state;
  uint32_t length;

  if (MIDI_Handle == NULL)
  {
    return USBH_FAIL;  // Should not happen if class is active
  }

  switch (MIDI_Handle->state)
  {
    case MIDI_IDLE:
      /* Idle: start a new IN transfer */
      printf("USBH_MIDI_Process: State=MIDI_IDLE, initiating IN transfer\r\n");
      USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer,
                           MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
      MIDI_Handle->state = MIDI_TRANSFER;
      printf("USBH_MIDI_Process: State -> MIDI_TRANSFER (waiting for data)\r\n");
      status = USBH_BUSY;
      break;

    case MIDI_TRANSFER:
      /* Check the state of the last USB transfer */
      urb_state = USBH_LL_GetURBState(phost, MIDI_Handle->InPipe);
      if (urb_state == USBH_URB_DONE)
      {
        /* Data packet received */
        length = USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);
        printf("USBH_MIDI_Process: URB done, received %lu bytes\r\n", (unsigned long)length);
        if (length > 0 && length <= USBH_MIDI_MAX_PACKET_SIZE)
        {
          uint32_t i = 0;
          while (i < length && (length - i) >= 4)
          {
            uint16_t nextHead = (MIDI_Handle->EventFIFOHead + 4) % USBH_MIDI_EVENT_FIFO_SIZE;
            if (nextHead == MIDI_Handle->EventFIFOTail)
            {
              /* FIFO full – cannot add more events */
              break;  // drop remaining events
            }
            /* Copy one 4-byte MIDI event into FIFO */
            memcpy(&MIDI_Handle->EventFIFO[MIDI_Handle->EventFIFOHead],
                   &MIDI_Handle->RxBuffer[i], 4);
            MIDI_Handle->EventFIFOHead = nextHead;
            i += 4;
          }
          uint32_t eventsAdded = i / 4;
          if (eventsAdded > 0)
          {
            printf("USBH_MIDI_Process: Added %lu MIDI events to FIFO\r\n", (unsigned long)eventsAdded);
          }
          if (i < length)
          {
            printf("USBH_MIDI_Process: MIDI FIFO overflow, dropped %lu bytes\r\n",
                   (unsigned long)(length - i));
          }
        }
        else if (length > USBH_MIDI_MAX_PACKET_SIZE)
        {
          /* Received packet larger than our buffer (should not happen in FS) */
          printf("USBH_MIDI_Process: Packet size %lu exceeds max %d, ignoring\r\n",
                 (unsigned long)length, USBH_MIDI_MAX_PACKET_SIZE);
        }
        /* Submit a new read transfer immediately to continue polling */
        USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer,
                             MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
        /* Stay in MIDI_TRANSFER state */
        status = USBH_OK;
      }
      else if (urb_state == USBH_URB_STALL)
      {
        /* IN endpoint stalled – clear the stall and retry */
        printf("USBH_MIDI_Process: IN endpoint 0x%02X stalled, clearing halt condition\r\n",
               MIDI_Handle->InEp);
        USBH_ClrFeature(phost, MIDI_Handle->InEp);  // clear stall on the IN endpoint
        USBH_BulkReceiveData(phost, MIDI_Handle->RxBuffer,
                             MIDI_Handle->InEpSize, MIDI_Handle->InPipe);
        status = USBH_OK;
      }
      else if (urb_state == USBH_URB_ERROR)
      {
        /* USB transfer error */
        MIDI_Handle->state = MIDI_ERROR;
        printf("USBH_MIDI_Process: USB transfer error, state -> MIDI_ERROR\r\n");
        status = USBH_FAIL;
      }
      else
      {
        /* URB_BUSY or NOTREADY: no new data yet, keep waiting */
        status = USBH_BUSY;
        // (No log here to avoid excessive output on each poll cycle)
      }
      break;

    case MIDI_ERROR:
      /* Error state - no recovery implemented */
      printf("USBH_MIDI_Process: State=MIDI_ERROR, unrecoverable error\r\n");
      status = USBH_FAIL;
      break;

    default:
      status = USBH_FAIL;
      break;
  }

  return status;
}

/* USBH_MIDI_SOFProcess: not used (included for completeness) */
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost)
{
  /* No periodic SOF handling needed for this class */
  return USBH_OK;
}

/* Public API function to get a MIDI event from the FIFO */
USBH_StatusTypeDef USBH_MIDI_GetEvent(USBH_HandleTypeDef *phost, uint8_t *event_buf)
{
  MIDI_HandleTypeDef *MIDI_Handle = (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
  if (MIDI_Handle == NULL)
  {
    return USBH_FAIL;  // Class not initialized or device not connected
  }
  /* Check FIFO */
  if (MIDI_Handle->EventFIFOHead == MIDI_Handle->EventFIFOTail)
  {
    // FIFO empty
    return USBH_FAIL;
  }
  /* Copy one 4-byte event from FIFO to user buffer */
  for (uint8_t j = 0; j < 4; j++)
  {
    event_buf[j] = MIDI_Handle->EventFIFO[MIDI_Handle->EventFIFOTail + j];
  }
  /* Advance tail index by 4 (one event) */
  MIDI_Handle->EventFIFOTail = (MIDI_Handle->EventFIFOTail + 4) % USBH_MIDI_EVENT_FIFO_SIZE;
  return USBH_OK;
}
