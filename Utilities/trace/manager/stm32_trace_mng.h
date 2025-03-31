/**
  ******************************************************************************
  * @file    stm32_trace_mng.h
  * @author  MCD Application Team
  * @brief   Header for STM32 trace manager
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TRACE_MNG_H
#define __TRACE_MNG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32_adv_trace.h"

/** @defgroup TRACE_MNG advanced tracer
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup TRACE_MNG_exported_TypeDef TRACE_MNG exported Typedef
  *  @{
  */

/**
  * @brief Application type definition
  */
typedef uint32_t UTIL_TRACE_MNG_AppTypeDef;

/**
  * @brief Version type definition
  */
typedef uint16_t UTIL_TRACE_MNG_VersionTypeDef;

/**
  * @brief Decoder enumeration definition
  */
typedef enum
{
  DECODER_ID_STRING            = 0x1U, /*!<decoder IDString 16-bit value associated to a string  */
  DECODER_STRING               = 0x2U, /*!<decoder String   size 8-bit + table of 8-_bit  */
  DECODER_VARIABLE_DEC         = 0x3U, /*!<decoder DEC        IDString + Var size 8-bit + table of 8-_bit (variable) display in decimal format */
  DECODER_VARIABLE_HEX         = 0x4U, /*!<decoder HEX        IDString + Var size 8-bit + table of 8-_bit (variable) display in hexadecimal format */
  DECODER_VARIABLE_BIN         = 0x5U, /*!<decoder HEX        IDString + Var size 8-bit + table of 8-_bit (variable) display in binary format */
  DECODER_VARIABLE_DEC_SIGNED  = 0x6U, /*!<decoder DEC_SIGNED IDString + Var size 8-bit + table of 8-_bit (variable) display in decimal format */
} UTIL_TRACE_MNG_Decoder_t;

/**
  * @brief trace manager status type definition
  */
typedef enum
{
  UTIL_TRACE_MNG_OK,
  UTIL_TRACE_MNG_MEMFULL,
  UTIL_TRACE_MNG_CHANNELDISABLED,
  UTIL_TRACE_MNG_CHANNELUNREGISTERED,
  UTIL_TRACE_MNG_RESETNOTALLOWED,
  UTIL_TRACE_MNG_CHANNELNUMBERERROR,
  UTIL_TRACE_MNG_TRANSPORTERROR,
  UTIL_TRACE_MNG_ERR
} UTIL_TRACE_MNG_Status_t;

/**
  * @brief Rx Status type definition
  */
typedef enum
{
  TRACE_MNG_RXSTART,
  TRACE_MNG_RXSIZE,
  TRACE_MNG_RXADDDATA,
  TRACE_MNG_RXCOMPLETE,
  TRACE_MNG_RXERROR,
  TRACE_MNG_FRAMEERROR,
} UTIL_TRACE_MNG_RxStatus_t;

/**
  * @brief Channel state enumeration type definition
  */
typedef enum
{
  TRACE_MNG_CHANNELSTATE_RESET          = 0, /*!<state reset, the channel has not been registered  */
  TRACE_MNG_CHANNELSTATE_STATE_ENABLED  = 1, /*!<state enabled, the channel is active              */
  TRACE_MNG_CHANNELSTATE_STATE_DISABLED = 2 /*!<state reset, the channel is disabled              */
} UTIL_TRACE_MNG_ChannelState_t;


/**
  *  @}
  */

/* External variables --------------------------------------------------------*/
/** @defgroup TRACE_MNG_exported_variables TRACE_MNG exported variables
  *
  *  @{
  */
/**
  *  @}
  */

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @defgroup TRACE_MNG_exported_macro TRACE_MNG exported macro
  *  @{
  */

/**
  * @}
  */
/* Exported functions ------------------------------------------------------- */

/** @defgroup TRACE_MNG_exported_function TRACE_MNG exported function
  *  @{
  */

/**
  * @brief TraceInit Initializes Logging feature
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Init(void);

/**
  * @brief TraceDeInit module DeInitializes.
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_DeInit(void);


/**
  * @brief register an application.
  * @retval Application channel ID allocated to ApplicationID/Instance/Core Status the zero value indicate an error
  */
uint8_t UTIL_TRACE_MNG_RegisterApplication(uint32_t *const AppID, uint16_t AppVersionNumber, uint8_t Core,
                                           uint8_t Instance,
                                           void (*RXCallback)(uint8_t PData, UTIL_TRACE_MNG_RxStatus_t Status));

/**
  * @brief set the state of the channel and verbose
  * @param Channel application channel number
  * @param State @ref UTIL_TRACE_MNG_ChannelState_t
  * @param Verboselevel verbose level value
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_SetChannelState(uint8_t Channel, UTIL_TRACE_MNG_ChannelState_t State,
                                                       uint8_t Verboselevel);


/**
  * @brief send data trace with the decoder IDString
  * @param Channel application channel number
  * @param VerboseLevel verbose level
  * @param IDString id of the string
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_IDStringVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                               uint16_t IDString);

/**
  * @brief send data trace with the decoder IDString
  * @param Channel application channel number
  * @param IDString id of the string
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_IDString(uint8_t Channel, uint16_t IDString);

/**
  * @brief send data trace with the decoder string
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param StringLen length if the string
  * @param String pointer on a string
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_StringVerbose(uint8_t Channel, uint8_t VerboseLevel, uint8_t StringLen,
                                                             const uint8_t *String);

/**
  * @brief send data trace with the decoder string
  * @param Channel application channel number
  * @param StringLen length if the string
  * @param String pointer on a string
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_String(uint8_t Channel, uint8_t StringLen, const uint8_t *String);

/**
  * @brief send trace IDString + variable and decoder is used to define how variable is displayed
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param Decoder type @ref DECODER_VARIABLE_DEC or @ref DECODER_VARIABLE_DEC_SIGNED
  *        or @ref DECODER_VARIABLE_HEX or @ref DECODER_VARIABLE_BIN
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VariableVerbose(uint8_t Channel, uint8_t VerboseLevel, uint16_t Decoder,
                                                               uint16_t IDString, uint8_t VarSize, const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in decimal unsigned
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDECVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString, uint8_t VarSize, const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in decimal unsigned
  * @param Channel application channel number
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                           const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in decimal signed
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC_SIGNEDVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                         uint16_t IDString, uint8_t VarSize, const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in decimal signed
  * @param Channel application channel number
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleDEC_SIGNED(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                                  const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in hexadecimal
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleHEXVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString, uint8_t VarSize, const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in hexadecimal
  * @param Channel application channel number
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleHEX(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                           const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in binary
  * @param Channel application channel number
  * @param VerboseLevel Verbose level
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleBINVerbose(uint8_t Channel, uint8_t VerboseLevel,
                                                                  uint16_t IDString,
                                                                  uint8_t VarSize, const void *Varptr);

/**
  * @brief send trace IDString + variable and the variable is displayed in binary
  * @param Channel application channel number
  * @param IDString id of string
  * @param VarSize variable size ( 1 = uint8_t or 2 = uint16_t or 4 = uint32_t)
  * @param Varptr variable pointer
  * @retval status based on @ref UTIL_TRACE_MNG_Status_t
  */
UTIL_TRACE_MNG_Status_t UTIL_TRACE_MNG_Decoder_VaraibleBIN(uint8_t Channel, uint16_t IDString, uint8_t VarSize,
                                                           const void *Varptr);
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__TRACE_MNG_H */
