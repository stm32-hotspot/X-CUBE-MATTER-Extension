/*******************************************************************************
  * @file    stm32_trace_mng.c
  * @author  MCD Application Team
  * @brief   This file contains the trace manager utility functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "utilities_conf.h"
#include "stm32_trace_mng.h"

/**
  * @addtogroup TRACE_MNG
  * @{
  */

/* Private defines -----------------------------------------------------------*/
/**
  * @defgroup TRACE_MNG_Private_defines TRACE_MNG Privates defines
  *  @{
  */

/**
  *  @brief  default definition of the max number of application channel managed
  */
#ifndef UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL
#define UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL 254
#elif UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL > 254
#error "the maximum number of application channel is 254"
#endif /* UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL */

/**
  *  @brief  Number of channel including channel 0
  */
#define TRACE_MNG_MAXCHANNEL (UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL + 1)


/**
  *  @brief  maximum data length of a command
  */
#define MAX_COMMAND_LENGTH       10

/**
  *  @brief  the control channel id
  */
#define CHANNEL_CONTROL          0

/**
  *  @brief  FOF identifier used to start/end a frame
  */
#define FOF            0x7Eu

/**
  *  @brief  ESC identifier used to escape a frame value
  */
#define ESC            0x7Du

/**
  *  @brief  FOF_VAL identifier used to replace FOF value
  */
#define FOF_VAL        0x5Eu

/**
  *  @brief  ESC_VAL identifier used to replace ESC value
  */
#define ESC_VAL        0x5Du

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/**
  *  @defgroup TRACE_MNG_Private_macros TRACE_MNG Privates macros
  *  @{
  */

/**
  *  @brief  Macro to write data without control inside the FIFO buffer
  */
#define TRACE_MNG_WDATA(_DATA_)  do {                                      \
                                      ptr[writepos] = (_DATA_);            \
                                      writepos = (writepos + 1) % fifosize;\
                                    } while(0);

/**
  *  @brief  Macro to write FOF data inside the FIFO
  */
#define TRACE_MNG_WDATAFOF()     do {                       \
                                      TRACE_MNG_WDATA(FOF); \
                                    } while(0);

/**
  *  @brief  Macro to write 8_bit data with control inside the FIFO
  */
#define TRACE_MNG_WDATAINT8(_DATA_)  \
  do {                               \
    if (FOF == (_DATA_))             \
    {                                \
      TRACE_MNG_WDATA(ESC);          \
      TRACE_MNG_WDATA(FOF_VAL);      \
    }                                \
    else if (ESC == (_DATA_))        \
    {                                \
      TRACE_MNG_WDATA(ESC);          \
      TRACE_MNG_WDATA(ESC_VAL);      \
    }                                \
    else                             \
    {                                \
      TRACE_MNG_WDATA(_DATA_);       \
    }                                \
  } while(0);

/**
  *  @brief  Macro to write 16_bit data inside the FIFO
  */
#define TRACE_MNG_WDATAINT16(_DATA_)            \
  do {                                          \
    TRACE_MNG_WDATAINT8((_DATA_ & 0xFF));       \
    TRACE_MNG_WDATAINT8(((_DATA_ >> 8) & 0xFF));\
  } while(0);

/**
  *  @brief  Macro to write 32_bit data inside the FIFO
  */
#define TRACE_MNG_WDATAINT32(_DATA_)            \
  do {                                          \
    TRACE_MNG_WDATAINT8((_DATA_ & 0xFF));       \
    TRACE_MNG_WDATAINT8(((_DATA_ >> 8) & 0xFF));\
    TRACE_MNG_WDATAINT8(((_DATA_ >>16) & 0xFF));\
    TRACE_MNG_WDATAINT8(((_DATA_ >>24) & 0xFF));\
  } while(0);

/**
  * @}
  */

/* Private enum --------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/**
  * @defgroup TRACE_MNG_private_typedef TRACE_MNG private typedef
  * @{
  */

/**
  * @brief list of the command for channel 0
  * @note : the command could not have the value FOF, ESC
  */
typedef enum
{
  COMMAND_GETAPPCHANNELINFO    = 0x01U, /*!< command to get the application info of the channel */
  COMMAND_SETCHANNELVERBOSE    = 0x02U, /*!< command to set the application info of the channel */
  COMMAND_GETCHANNELINFO       = 0x03U, /*!< command to get the channel info : registered/available */
  COMMAND_CHREGISTRATION       = 0x05U, /*!< command to send registration info */
  COMMAND_RESET                = 0x06U, /*!< command to send reset */
  COMMAND_OVERFLOW             = 0x08U, /*!< command to send an overflow */
  COMMAND_INFORM_STATEVERBOSE  = 0x09U, /*!< command to inform tools on the change of the state verbose value */
  COMMAND_ACK                  = 0xFFU, /*!< command to send an ACK */
  COMMAND_NAK                  = 0xFEU, /*!< command to send an NAK */
  COMMAND_NONE                 = 0x00U  /*!< reserved command */
} TRACE_MNG_Command_t;

/**
  * @brief TRACE_MNG_RXStateMachine_t
  * define the state of the machine to manage the reception
  */
typedef enum
{
  FRAME_WAIT_FOF,
  FRAME_WAIT_CHANNEL,
  FRAME_WAIT_DATA_SIZE,
  FRAME_DATA_RECEPTION,
  FRAME_WAIT_END_FOF,
} TRACE_MNG_RXStateMachine_t;

/**
  *  @brief TRACE_MNG_ApplicationInfo_t.
  *  this structure contains all the data to handle the trace manager context.
  *  @note any move inside the structure definition will impact the command formating
  */
typedef union
{
  uint16_t Val;                 /*!< all application info                     */
  struct
  {
    uint16_t Verbose:6;         /*!< Current application verbose level        */
    uint16_t State:2;           /*!< current state of the application channel */
    uint16_t Instance:5;        /*!< Instance value                           */
    uint16_t Core:3;            /*!< Core value                               */
  } Data;                       /*!< Used to set or get a data value          */
  struct
  {
    uint16_t State_Verbose:8;   /*!< verbose + state application info         */
    uint16_t Core_Instance:8;   /*!< core + instance application info         */
  } Command;                    /*!< Used in command mode                     */
} TRACE_MNG_ApplicationInfo_t;

/**
  *  @brief  TRACE_MNG_ChannelInfo_t channel information.
  */
typedef struct
{
  void (*RXCallback)(uint8_t, UTIL_TRACE_MNG_RxStatus_t);   /*!<function ptr to transfer rx data */
  uint32_t *ApplicationID;                                  /*!<Application ID of the channel    */
  TRACE_MNG_ApplicationInfo_t ApplicationInfo;                /*!<Application information          */
  uint16_t ApplicationVersion;                              /*!<Application version              */
} TRACE_MNG_ChannelInfo_t;

/**
  *  @brief  TRACE_MNG_Context.
  *  this structure contains all the data to handle the trace manager context.
  */
typedef struct
{
  TRACE_MNG_ChannelInfo_t ChannelCtx[TRACE_MNG_MAXCHANNEL];     /*!<this variable contains the information about all the applications */
  uint8_t NumberOfRegisteredChannel;                            /*!<this variable contains the number of registered application */
  void (*currentRXCallback)(uint8_t, UTIL_TRACE_MNG_RxStatus_t);/*!<this variable contains the current callback receiving data  */
} TRACE_MNG_Context_t;

/**
  *  @}
  */

/* Private variables ---------------------------------------------------------*/
/**
  * @defgroup TRACE_MNG_private_variable TRACE_MNG private variable
  * @{
  */

/**
  *  @brief  TRACE_MNG_Ctx
  *  this variable contains all the internal data of the advanced trace system.
  */
TRACE_MNG_Context_t TRACE_MNG_Ctx;

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/**
  * @defgroup TRACE_MNG_transport_function TRACE_MNG transport function
  * @{
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_TransportInit(void);
UTIL_TRACE_MNG_Status_t UTIL_ADV_TRACE_TransportSetRxCallback(void (*UserCallback)(uint8_t *PData, uint16_t Size, uint8_t Error));
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_TransportAllocateBuffer(uint16_t Size, uint8_t **pData, uint16_t *FifoSize, uint16_t *WritePos);
/**
  * @}
  */

/**
  * @defgroup TRACE_MNG_private_function TRACE_MNG private function
  * @{
  */
static void TRACE_MNG_ExecuterCommand(uint8_t *Data, uint8_t Size);
static void TRACE_MNG_Channel0Responder(uint8_t Data, UTIL_TRACE_MNG_RxStatus_t Status);
static void TRACE_NMG_RXCallback(uint8_t *PData, uint16_t Size, uint8_t Error);
static uint8_t TRACE_MNG_GetINT8Size(uint8_t val);
static uint8_t TRACE_MNG_GetINT16Size(uint16_t val);
static uint8_t TRACE_MNG_GetINT32Size(uint32_t val);
static void TRACE_MNG_SendCommand(uint8_t Command, uint8_t CommandOrigin, uint8_t *PtrData, uint8_t DSize);
static uint8_t TRACE_MNG_Format_ChannelInfo(uint8_t Channel, uint8_t *pData);
static uint8_t TRACE_MNG_ControlTraceCondition(uint8_t Channel, uint8_t VerboseLevel);
static void TRACE_MNG_COMMANDOverFlow(uint8_t **PData, uint16_t *Size);
uint32_t UTIL_TRACE_MNG_GetTimeStamp(void);

/**
  * @}
  */

/* Functions Definition ------------------------------------------------------*/
/**
  * @addtogroup TRACE_MNG_exported_function
  * @{
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Init(void)
{
  /* Initialize the advanced trace module */
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_TransportInit();

  /* initialize the Ptr for Read/Write */
  (void)UTIL_TRACE_MNG_MEMSET8(&TRACE_MNG_Ctx, 0x0, sizeof(TRACE_MNG_Context_t));

  /* Channel zero is enabled by default, channel 0 disabled means no trace */
  TRACE_MNG_Ctx.ChannelCtx[0].RXCallback = TRACE_MNG_Channel0Responder;
  TRACE_MNG_Ctx.ChannelCtx[0].ApplicationInfo.Data.State  = TRACE_MNG_CHANNELSTATE_STATE_DISABLED;

  /* Start the RX reception */
  UTIL_ADV_TRACE_TransportSetRxCallback(TRACE_NMG_RXCallback);

  if (UTIL_TRACE_MNG_OK == retr)
  {
    /* Send the command Reset to indicate an application reset */
    TRACE_MNG_SendCommand(COMMAND_RESET, 0, NULL, 0u);
  }

  UTIL_ADV_TRACE_RegisterOverRunFunction(TRACE_MNG_COMMANDOverFlow);
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_DeInit(void)
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  /* Un-initialize the Low Level interface */
  if (UTIL_ADV_TRACE_OK != UTIL_ADV_TRACE_DeInit())
  {
    retr = UTIL_TRACE_MNG_ERR;
  }
  return retr;
}

uint8_t UTIL_TRACE_MNG_RegisterApplication(uint32_t *const AppID, uint16_t AppVersionNumber, uint8_t Core,
                                           uint8_t Instance,
                                           void (*RXCallback)(uint8_t PData, UTIL_TRACE_MNG_RxStatus_t Status))
{
  uint8_t retr = 0;
  uint8_t pdata[22];
  for (uint8_t index = 1; index < TRACE_MNG_MAXCHANNEL; index++)
  {
    /* check of the buffer is empty */
    if (((TRACE_MNG_Ctx.ChannelCtx[index].ApplicationID == AppID) &&
         (TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.Instance == Instance) &&
         (TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.Core == Core)) ||
        TRACE_MNG_Ctx.ChannelCtx[index].ApplicationID == NULL)
    {
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationID = AppID;
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.State = TRACE_MNG_CHANNELSTATE_STATE_ENABLED;
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.Verbose = 0;
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.Core = Core;
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationInfo.Data.Instance = Instance;
      TRACE_MNG_Ctx.ChannelCtx[index].ApplicationVersion = AppVersionNumber;
      TRACE_MNG_Ctx.ChannelCtx[index].RXCallback = RXCallback;
      TRACE_MNG_Ctx.NumberOfRegisteredChannel++;

      TRACE_MNG_SendCommand(COMMAND_CHREGISTRATION, 0, pdata, TRACE_MNG_Format_ChannelInfo(index, pdata));
      retr = index;
      break;
    }
  }
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_SetChannelState(uint8_t Channel, UTIL_TRACE_MNG_ChannelState_t State,
                                                       uint8_t VerboseLevel)
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_RESETNOTALLOWED;

  if (Channel < TRACE_MNG_MAXCHANNEL)
  {
    switch (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Data.State)
    {
      case TRACE_MNG_CHANNELSTATE_STATE_ENABLED:
      case TRACE_MNG_CHANNELSTATE_STATE_DISABLED:
      {
        if (TRACE_MNG_CHANNELSTATE_RESET != State)
        {
          uint8_t data_command[2];
          /* set the state of the Verbose */
          TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Data.Verbose = VerboseLevel;
          TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Data.State = State;
          retr = UTIL_TRACE_MNG_OK;
          data_command[0] = Channel;
          data_command[1] = TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Command.State_Verbose;
          /* Inform PC tools about the state/verbose update */
          TRACE_MNG_SendCommand(COMMAND_INFORM_STATEVERBOSE, 0, data_command, 2);
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
  else
  {
    retr = UTIL_TRACE_MNG_CHANNELNUMBERERROR;
  }
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_IDStringVerbose(uint8_t Channel, uint8_t VerboseLevel, uint16_t IDString)
{

  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  uint32_t size = 2;
  uint32_t timeStampVal = UTIL_TRACE_MNG_GetTimeStamp();
  uint8_t *ptr = NULL;
  uint16_t fifosize;
  uint16_t writepos;

  if (TRACE_MNG_ControlTraceCondition(Channel, VerboseLevel) == 1)
  {
    size += TRACE_MNG_GetINT32Size(timeStampVal);
    size += TRACE_MNG_GetINT8Size(Channel);
    size += 2; /* corresponding to TRACE_MNG_GetTimeDecoderSize(DECODER_ID_STRING);*/

    /* Calculate the Frame Size */
    if (IDString > 0xFFu)
    {
      size += TRACE_MNG_GetINT16Size((uint16_t) IDString);
    }
    else
    {
      size += TRACE_MNG_GetINT8Size((uint8_t) IDString);
    }

    /* Allocate the data buffer */
    if ((UTIL_TRACE_MNG_OK == UTIL_TRACE_MNG_TransportAllocateBuffer(size, &ptr, &fifosize, &writepos)) && (0 != fifosize))
    {
      /* Push FOF */
      TRACE_MNG_WDATAFOF();
      /* Push timestamp */
      TRACE_MNG_WDATAINT32(timeStampVal);
      /* Push channel */
      TRACE_MNG_WDATAINT8(Channel);
      /* Push decoder */
      TRACE_MNG_WDATAINT16(DECODER_ID_STRING);

      if (IDString > 0xFFu)
      {
        TRACE_MNG_WDATAINT16(IDString);
      }
      else
      {
        TRACE_MNG_WDATAINT8(IDString);
      }

      /* Push FOF */
      TRACE_MNG_WDATAFOF();

      /* Validate the data send */
      UTIL_ADV_TRACE_ZCSend_Finalize();
    }
    else
    {
      /* not enough space in the memory */
      retr = UTIL_TRACE_MNG_MEMFULL;
    }
  }
  else
  {
    /* Channel disabled/Reset or mismatch with verbose level */
    retr = UTIL_TRACE_MNG_CHANNELDISABLED;
  }
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_IDString(uint8_t Channel, uint16_t IDString)
{
  return UTIL_TRACE_MNG_Decoder_IDStringVerbose(Channel, 0, IDString);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_StringVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                             uint8_t StringLen, const uint8_t *String)
{

  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  uint32_t size = 4; /* 2 FOF + DecoderID */
  uint32_t timeStampVal = UTIL_TRACE_MNG_GetTimeStamp();
  uint8_t *ptr = NULL;
  uint16_t fifosize;
  uint16_t writepos;

  if (TRACE_MNG_ControlTraceCondition(Channel, VerboseLevel) == 1)
  {
    /* Calculate the Frame Size */
    size += TRACE_MNG_GetINT32Size(timeStampVal);
    size += TRACE_MNG_GetINT8Size(Channel);
    size += TRACE_MNG_GetINT8Size(StringLen);

    for (uint8_t index = 0; index < StringLen; index++)
    {
      size += TRACE_MNG_GetINT8Size(String[index]);
    }

    /* Allocate the data buffer */
    if ((UTIL_TRACE_MNG_OK == UTIL_TRACE_MNG_TransportAllocateBuffer(size, &ptr, &fifosize, &writepos)) && (0 != fifosize))
    {
      /* Push FOF */
      TRACE_MNG_WDATAFOF();
      /* Push timestamp */
      TRACE_MNG_WDATAINT32(timeStampVal);
      /* Push channel */
      TRACE_MNG_WDATAINT8(Channel);
      /* Push decoder */
      TRACE_MNG_WDATAINT16(DECODER_STRING);
      /* Push the string length */
      TRACE_MNG_WDATAINT8(StringLen);
      for (uint8_t index = 0; index < StringLen; index++)
      {
        TRACE_MNG_WDATAINT8(String[index]);
      }
      /* Push FOF */
      TRACE_MNG_WDATAFOF();
      /* Validate the data send */
      UTIL_ADV_TRACE_ZCSend_Finalize();
    }
    else
    {
      /* not enough space in the memory */
      retr = UTIL_TRACE_MNG_MEMFULL;
    }
  }
  else
  {
    /* Channel disabled/Reset or mismatch with verbose level */
    retr = UTIL_TRACE_MNG_CHANNELDISABLED;
  }
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_String(uint8_t Channel, uint8_t StringLen, const uint8_t *String)
{
  return UTIL_TRACE_MNG_Decoder_StringVerbose(Channel, 0, StringLen, String);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VariableVerbose(uint8_t Channel, uint8_t VerboseLevel, uint16_t Decoder,
                                                               uint16_t IDString, uint8_t VarSize, const void *Varptr)
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  uint32_t size = 2; /* 2 FOF */
  uint32_t timeStampVal = UTIL_TRACE_MNG_GetTimeStamp();
  uint8_t *ptr = NULL;
  uint16_t fifosize;
  uint16_t writepos;

  if ((TRACE_MNG_ControlTraceCondition(Channel, VerboseLevel) == 1) &&
      ((Decoder >= DECODER_VARIABLE_DEC) && (Decoder <= DECODER_VARIABLE_DEC_SIGNED))
     )
  {
    /* Calculate the Frame Size */
    size += TRACE_MNG_GetINT32Size(timeStampVal);
    size += TRACE_MNG_GetINT8Size(Channel);
    size += 2; /* corresponding to TRACE_MNG_GetTimeDecoderSize(Decoder);*/
    size += TRACE_MNG_GetINT16Size(IDString);
    size += TRACE_MNG_GetINT8Size(VarSize);

    for (uint8_t index = 0; index < VarSize; index++)
    {
      size += TRACE_MNG_GetINT8Size(((uint8_t *)Varptr)[index]);
    }

    /* Allocate the data buffer */
    if ((UTIL_TRACE_MNG_OK == UTIL_TRACE_MNG_TransportAllocateBuffer(size, &ptr, &fifosize, &writepos)) && (0 != fifosize))
    {
      /* Push FOF */
      TRACE_MNG_WDATAFOF();
      /* Push timestamp */
      TRACE_MNG_WDATAINT32(timeStampVal);
      /* Push channel */
      TRACE_MNG_WDATAINT8(Channel);
      /* Push decoder */
      TRACE_MNG_WDATAINT16(Decoder);
      /* Push ID String */
      TRACE_MNG_WDATAINT16(IDString);
      /* Push datasize */
      TRACE_MNG_WDATAINT8(VarSize);
      for (uint8_t index = 0; index < VarSize; index++)
      {
        TRACE_MNG_WDATAINT8(((uint8_t *)Varptr)[index]);
      }
      /* Push FOF */
      TRACE_MNG_WDATAFOF();

      /* Validate the data send */
      UTIL_ADV_TRACE_ZCSend_Finalize();
    }
    else
    {
      /* not enough space in the memory */
      retr = UTIL_TRACE_MNG_MEMFULL;
    }
  }
  else
  {
    /* Channel disabled/Reset or mismatch with verbose level */
    retr = UTIL_TRACE_MNG_CHANNELDISABLED;
  }
  return retr;
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDECVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString, uint8_t VarSize,
                                                                  const void *Varptr)
{
  /* used the decoder DEC */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, VerboseLevel, DECODER_VARIABLE_DEC, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                           const void *Varptr)
{
  /* used the decoder DEC */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, 0, DECODER_VARIABLE_DEC, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC_SIGNEDVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                         uint16_t IDString, uint8_t VarSize,
                                                                         const void *Varptr)
{
  /* used the decoder DEC_SIGNED */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, VerboseLevel, DECODER_VARIABLE_DEC_SIGNED, IDString, VarSize,
                                                Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC_SIGNED(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                                  const void *Varptr)
{
  /* used the decoder DEC_SIGNED */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, 0, DECODER_VARIABLE_DEC_SIGNED, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleHEXVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString, uint8_t VarSize,
                                                                  const void *Varptr)
{
  /* used the decoder HEX */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, VerboseLevel, DECODER_VARIABLE_HEX, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleHEX(uint8_t Channel, uint16_t IDString,
                                                           uint8_t VarSize, const void *Varptr)
{
  /* used the decoder HEX */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, 0, DECODER_VARIABLE_HEX, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleBINVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString, uint8_t VarSize,
                                                                  const void *Varptr)
{
  /* used the decoder BIN */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, VerboseLevel, DECODER_VARIABLE_BIN, IDString, VarSize, Varptr);
}

UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleBIN(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                           const void *Varptr)
{
  /* used the decoder BIN */
  return UTIL_TRACE_MNG_Decoder_VariableVerbose(Channel, 0, DECODER_VARIABLE_BIN, IDString, VarSize, Varptr);
}

/**
  * @}
  */

/**
  * @addtogroup TRACE_MNG_transport_function
  * @{
  */

/**
  * @brief Initialize the transport layer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
__WEAK UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_TransportInit(void)
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  if (UTIL_ADV_TRACE_OK != UTIL_ADV_TRACE_Init())
  {
    retr = UTIL_TRACE_MNG_TRANSPORTERROR;
  }
  return retr;
}

/**
  * @brief set the reception callback of the transport layer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
__WEAK UTIL_TRACE_MNG_Status_t UTIL_ADV_TRACE_TransportSetRxCallback(void (*UserCallback)(uint8_t *PData,
                                                                           uint16_t Size, uint8_t Error))
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_OK;
  if (UTIL_ADV_TRACE_OK != UTIL_ADV_TRACE_StartRxProcess(UserCallback))
  {
    retr = UTIL_TRACE_MNG_TRANSPORTERROR;
  }
  return retr;
}

/**
  * @brief  Allocate buffer inside the transport layer
  * @retval 1 if success else 0
  */
__WEAK UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_TransportAllocateBuffer(uint16_t Size, uint8_t **pData,
                                                       uint16_t *FifoSize, uint16_t *WritePos)
{
  UTIL_TRACE_MNG_Status_t retr = UTIL_TRACE_MNG_TRANSPORTERROR;
  if (UTIL_ADV_TRACE_OK == UTIL_ADV_TRACE_ZCSend_Allocation(Size, pData, FifoSize, WritePos))
  {
    retr = UTIL_TRACE_MNG_OK;
  }
  return retr;
}

/**
  * @}
  */

/**
  * @addtogroup TRACE_MNG_private_function
  *  @{
  */

/**
  * @brief  calculate the space to store a data int8
  * @param  Value data value
  * @retval the size in bit
  */
static uint8_t TRACE_MNG_GetINT8Size(uint8_t Value)
{
  uint8_t size = 1;
  if ((Value == FOF) || (Value == ESC))
  {
    size++;
  }
  return size;
}

/**
  * @brief  calculate the space to store a data int16
  * @param  Value data value
  * @retval the size in bit
  */
static uint8_t TRACE_MNG_GetINT16Size(uint16_t Value)
{
  uint8_t size;
  size = TRACE_MNG_GetINT8Size((uint8_t)(Value & 0xFF));
  size += TRACE_MNG_GetINT8Size((uint8_t)((Value >> 8) & 0xFF));
  return size;
}

/**
  * @brief  calculate the space to store a data int32
  * @param  Value data value
  * @retval the size in bit
  */
static uint8_t TRACE_MNG_GetINT32Size(uint32_t Value)
{
  uint8_t size;
  size  = TRACE_MNG_GetINT8Size((uint8_t)(Value & 0xFF));
  size += TRACE_MNG_GetINT8Size((uint8_t)((Value >> 8) & 0xFF));
  size += TRACE_MNG_GetINT8Size((uint8_t)((Value >> 16) & 0xFF));
  size += TRACE_MNG_GetINT8Size((uint8_t)((Value >> 24) & 0xFF));
  return size;
}

/**
  * @brief  this function execute the command receive on the channel 0
  * @param  Data pointer on the data Value data value
  * @param Size data size
  */
static void TRACE_MNG_ExecuterCommand(uint8_t *Data, uint8_t Size)
{
  uint8_t tmpBuffer[22];
  uint8_t Appchannel = Data[1];
  uint8_t command = Data[0];
  TRACE_MNG_Command_t status = COMMAND_NAK;

  if (Size >= 1) /* Minimum size for a command */
  {
    switch (command)
    {
      case COMMAND_GETAPPCHANNELINFO:
      {
        if (TRACE_MNG_Ctx.ChannelCtx[Appchannel].ApplicationID != NULL)
        {
          TRACE_MNG_SendCommand(COMMAND_ACK, COMMAND_GETAPPCHANNELINFO, tmpBuffer,
                                /* the buffer used here should be 22-bit minmun */
                                TRACE_MNG_Format_ChannelInfo(Appchannel, tmpBuffer));
          status = COMMAND_NONE;
        }
        break;
      }
      case COMMAND_SETCHANNELVERBOSE:
      {
        if (((Appchannel == 0)
             ||
             ((Appchannel < TRACE_MNG_MAXCHANNEL) &&
              (TRACE_MNG_Ctx.ChannelCtx[Appchannel].ApplicationID != NULL) &&
              (TRACE_MNG_Ctx.ChannelCtx[Appchannel].ApplicationInfo.Data.State != TRACE_MNG_CHANNELSTATE_RESET))
            )
           )
        {
          TRACE_MNG_Ctx.ChannelCtx[Appchannel].ApplicationInfo.Command.State_Verbose = Data[2];
          status = COMMAND_ACK;
        }
        break;
      }
      case COMMAND_GETCHANNELINFO:
        tmpBuffer[0] = TRACE_MNG_Ctx.NumberOfRegisteredChannel;
        tmpBuffer[1] = UTIL_TRACE_MNG_MAXAPPLICATIONCHANNEL; /* number of channel available for the application */
        TRACE_MNG_SendCommand(COMMAND_ACK, COMMAND_GETCHANNELINFO, tmpBuffer, 2);
        status = COMMAND_NONE;
        break;
      default:
        break;
    }

    switch (status)
    {
      case COMMAND_NAK:
      case COMMAND_ACK:
        TRACE_MNG_SendCommand(status, command, NULL, 0);
        break;
      case COMMAND_NONE:
      default:
        break;
    }
  }
}

/**
  * @brief  this function is the channel 0 responder in charge to receive/treat the data for the channel 0
  * @param  Data data value
  * @param  Status based on @ref UTIL_TRACE_MNG_RxStatus_t
  */
static void TRACE_MNG_Channel0Responder(uint8_t Data, UTIL_TRACE_MNG_RxStatus_t Status)
{
  static uint8_t data[MAX_COMMAND_LENGTH];
  static uint8_t index = 0;

  switch (Status)
  {
    case TRACE_MNG_RXSTART:
      index = 0;
    break;
    case TRACE_MNG_RXADDDATA:
    {
      if (index < MAX_COMMAND_LENGTH)
      {
        data[index++] = Data;
      }
      break;
    }
    case TRACE_MNG_RXCOMPLETE:
    {
      /* execute the command */
      TRACE_MNG_ExecuterCommand(data, index);
      break;
    }
    case TRACE_MNG_RXSIZE:
      break;
    case TRACE_MNG_RXERROR:
    case TRACE_MNG_FRAMEERROR:
    default:
    {
      TRACE_MNG_SendCommand(COMMAND_NAK, 0, NULL, 0);
      index = 0;
      break;
    }
  }
}

/**
  * @brief  this function is the channel 0 responder in charge to receive/treat the data for the channel 0
  * @param  Data data value
  * @param status based on @ref UTIL_TRACE_MNG_RxStatus_t
  */
static void TRACE_MNG_SendCommand(uint8_t Command, uint8_t CommandOrigin, uint8_t *PtrData, uint8_t DSize)
{
  uint16_t size = 10; /* 2FOF + 4TS + channel 0 + command*/
  uint8_t *ptr;
  uint16_t fifosize;
  uint16_t writepos;

  if (CommandOrigin != 0)
  {
    /* size + 1 to add the origin command inside the frame */
    size += 1;
  }

  for (uint8_t index = 0; index < DSize; index++)
  {
    size += TRACE_MNG_GetINT8Size(PtrData[index]);
  }

  /* Allocation of a buffer inside the trace system */
  if ((UTIL_TRACE_MNG_OK == UTIL_TRACE_MNG_TransportAllocateBuffer(size, &ptr, &fifosize, &writepos)) && (0 != fifosize))
  {
    /* Push FOF */
    TRACE_MNG_WDATAFOF();

    /* Write zero 7 zero : timeStamp (4) + channel 0 (1) + decoder 0 (2)*/
    for (uint8_t index = 0; index < 7; index++)
    {
      TRACE_MNG_WDATA(0x00u);
    }

    TRACE_MNG_WDATA(Command);
    if (CommandOrigin != 0)
    {
      TRACE_MNG_WDATA(CommandOrigin);
    }

    for (uint16_t index = 0; index < DSize; index++)
    {
      TRACE_MNG_WDATAINT8(PtrData[index]);
    }

    /* Push FOF */
    TRACE_MNG_WDATAFOF();

    /* Validate the data send */
    UTIL_ADV_TRACE_ZCSend_Finalize();
  }
}

/**
  * @brief  this function is the channel 0 responder in charge to receive/treat the data for the channel 0
  * @param  Channel the channel number
  * @param  PData pointer to store the data value
  * @retval data size
  */
static uint8_t TRACE_MNG_Format_ChannelInfo(uint8_t Channel, uint8_t *PData)
{
  /* channel info */
  PData[0] = Channel;

  /* Application ID */
  for (uint8_t datapos = 0; datapos < 4; datapos++)
  {
    PData[4 * datapos + 1] = TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationID[datapos] & 0xFF;
    PData[4 * datapos + 2] = (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationID[datapos] >> 8) & 0xFF;
    PData[4 * datapos + 3] = (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationID[datapos] >> 16) & 0xFF;
    PData[4 * datapos + 4] = (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationID[datapos] >> 24) & 0xFF;
  }

  /* State_Verbose */
  PData[17] = TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Command.State_Verbose;

  /* CoreInstance */
  PData[18] = TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Command.Core_Instance;

  /* Version */
  PData[19] = TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationVersion & 0xFF;
  PData[20] = (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationVersion >> 8) & 0xFF;

  /* channel state */
  return 21;
}

/**
  * @brief  this function manage the frame to send an overflow command
  * @param  PData pointer on the data frame
  * @param  Size pointer to get the size of the data
  */
static void TRACE_MNG_COMMANDOverFlow(uint8_t **PData, uint16_t *Size)
{
  static const uint8_t overflow_command[] = { FOF, 0x0, 0x0, 0x0, 0x0,
                                                   0x0,
                                                   0x0, 0x0,
                                                   COMMAND_OVERFLOW,
                                             FOF };
  *PData = (uint8_t *)overflow_command;
  *Size = sizeof(overflow_command);
}

/**
  * @brief  this function control if the trace could be sent
  * @param Channel the channel number
  * @param Verbose the verbose Level
  * @retval status based on 0/1 FALSE/TRUE
  */
static uint8_t TRACE_MNG_ControlTraceCondition(uint8_t Channel, uint8_t VerboseLevel)
{
  uint8_t retr = 0; /* FALSE condition */
  if (
    /* control of the channel number */
    (Channel > 0) && (Channel < TRACE_MNG_MAXCHANNEL) &&
    /* control if the channel 0 is enabled */
    (TRACE_MNG_Ctx.ChannelCtx[CHANNEL_CONTROL].ApplicationInfo.Data.State == TRACE_MNG_CHANNELSTATE_STATE_ENABLED) &&
    /* control if the app channel is enabled */
    (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Data.State == TRACE_MNG_CHANNELSTATE_STATE_ENABLED) &&
    /* control the verbose level of the app channel */
    (TRACE_MNG_Ctx.ChannelCtx[Channel].ApplicationInfo.Data.Verbose >= VerboseLevel))
  {
    retr = 1; /* TRUE condition */
  }
  return retr;
}

/**
  * @brief  this function manage the data reception
  * @param  pointer on data buffer
  * @param  size of the data buffer
  * @param  error code 0 : OK else error
  */
void TRACE_NMG_RXCallback(uint8_t *PData, uint16_t Size, uint8_t Error)
{
  static TRACE_MNG_RXStateMachine_t state = FRAME_WAIT_FOF;
  static uint8_t escape = 0;
  static uint8_t data_size = 0;

  /* Manage the RX callback reception */
  if (Error != 0)
  {
    /* Report an error to the current channel and discard reception machine */
    if (TRACE_MNG_Ctx.currentRXCallback != NULL)
    {
      TRACE_MNG_Ctx.currentRXCallback(0, TRACE_MNG_RXERROR);
    }
    /* wait for the next frame */
    state = FRAME_WAIT_FOF;
    escape = 0;
    TRACE_MNG_Ctx.currentRXCallback = NULL;
  }
  else
  {
    for (uint16_t index = 0; index < Size; index++)
    {
      if (PData[index] == FOF)
      {
        /* corresponds to 2 case the begin of a data or the end of data transfer */
        switch (state)
        {
          case FRAME_WAIT_FOF:
            state = FRAME_WAIT_CHANNEL;
            break;
          case FRAME_WAIT_END_FOF:
            /* Frame reception complete */
            if (TRACE_MNG_Ctx.currentRXCallback != NULL)
            {
              TRACE_MNG_Ctx.currentRXCallback(0, TRACE_MNG_RXCOMPLETE);
            }
            /* wait the nexframe */
            state = FRAME_WAIT_FOF;
            break;
          default:
            /* case when receiving an unexpected FOF, a frame error is reported */
            if (TRACE_MNG_Ctx.currentRXCallback != NULL)
            {
              TRACE_MNG_Ctx.currentRXCallback(0, TRACE_MNG_FRAMEERROR);
            }
            /* going to wait for channel if it is the beginning of a new frame */
            state = FRAME_WAIT_CHANNEL;
            break;
        }
        escape = 0;
        TRACE_MNG_Ctx.currentRXCallback = NULL;
      }
      else
      {
        /* corresponds to data reception */
        if ((PData[index] == ESC) && (escape == 0))
        {
          /* start sequence of replacement */
          escape = 1;
        }
        else
        {
          uint8_t data = PData[index];
          if (escape == 1)
          {
            escape = 0;
            switch (data)
            {
              case ESC_VAL:
                data = ESC;
                break;
              case FOF_VAL:
                data = FOF;
                break;
              default:
                if (TRACE_MNG_Ctx.currentRXCallback != NULL)
                {
                  TRACE_MNG_Ctx.currentRXCallback(data, TRACE_MNG_FRAMEERROR);
                }
                state = FRAME_WAIT_FOF;
                break;
            }
          }
          switch (state)
          {
            case FRAME_WAIT_CHANNEL:
            {
              TRACE_MNG_Ctx.currentRXCallback = TRACE_MNG_Ctx.ChannelCtx[data].RXCallback;
              if (NULL != TRACE_MNG_Ctx.currentRXCallback)
              {
                TRACE_MNG_Ctx.currentRXCallback(data, TRACE_MNG_RXSTART);
                state = FRAME_WAIT_DATA_SIZE;
              }
              else
              {
                /* the data are not store because there is no application to treat them */
                state = FRAME_WAIT_FOF;
              }
              break;
            }
            case FRAME_WAIT_DATA_SIZE:
            {
              data_size = data;
              TRACE_MNG_Ctx.currentRXCallback(data, TRACE_MNG_RXSIZE);
              state = FRAME_DATA_RECEPTION;
              break;
            }
            case FRAME_DATA_RECEPTION:
              data_size--;
              TRACE_MNG_Ctx.currentRXCallback(data, TRACE_MNG_RXADDDATA);
              if (data_size == 0)
              {
                state = FRAME_WAIT_END_FOF;
              }
              break;
            case FRAME_WAIT_END_FOF:
              TRACE_MNG_Ctx.currentRXCallback(data, TRACE_MNG_FRAMEERROR);
              state = FRAME_WAIT_FOF;
              break;
            default:
              break;
          }
        }
      }
    }
  }
}

/**
  * @brief read the time stamp
  * @retval the time stamp value in 32-bit
  */
__WEAK uint32_t UTIL_TRACE_MNG_GetTimeStamp(void)
{
  return 0;
}

/**
  * @}
  */

/**
  * @}
  */
