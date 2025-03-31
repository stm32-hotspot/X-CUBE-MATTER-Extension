****
  @verbatim
  ******************************************************************************
  * @file    Projects\STM32WBA65I-DK1\Applications\readme.txt
  * @author  MCD Application Team
  * @brief   This file describes the Projects structure for STM32WBA65I-DK1
  ******************************************************************************
  *
  * Copyright (c) 2019-2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim
  
WBA6 Projects structure description
===================================

* Projects\STM32WBA65I-DK1\Applications\Matter\Lighting-App
  Lighting_App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
  See readme.txt in project directory for more informations.
  
* Projects\STM32WBA65I-DK1\Applications\Matter\Window-App
  Window_App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
  See readme.txt in project directory for more informations.
  
* Projects\STM32WBA65I-DK1\Applications\Matter\Generic-Switch-App
  Generic_switch_App example is based on Matter and behaves as a Matter accessory communicating over a 802.15.4 Thread network.
  See readme.txt in project directory for more informations.

* Projects\STM32WBA65I-DK1\Applications\Matter\STM_Provisioning
  STM_Provisioning example is used to encrypt and store in flash and securely the Device Attestation private key using PSA ITS.
  See readme.txt in project directory for more informations.

* OEMiROT example
  Entry point: see readme.txt in Projects\STM32WBA65I-DK1\Applications\ROT\OEMiROT_Appli_TrustZone directory for more detailled informations
  about how-to compile and flash OEMiROT binaries.
  
  Here is a quick overview:
  Secure Boot and Secure Firmware update of Matter application based on OEMiROT.
  It consits of 3 projects:
  
    * Projects\STM32WBA65I-DK1\Applications\ROT\OEMiROT_Boot
      This project provides an OEMiROT boot path for MATTER.
       
    * Projects\STM32WBA65I-DK1\Applications\ROT\OEMiROT_Appli_TrustZone
      This project contains the OEMiROT MATTER Lighting example. It is a TrustZone project, ie it composed of 2 main parts:
        - OEMiROT_Appli_TrustZone\NonSecure: the Non-secure application = MATTER Lighting_App example.
        - OEMiROT_Appli_TrustZone\Secure: the secure application is responsible for re-configuring the HW IP and launching the non-secure application.
      See readme.txt in project directory for more informations.
      
    * Projects\STM32WBA65I-DK1\Applications\ROT\OEMiROT_Loader_TrustZone
      This project is used to generate the OEMiROT loader image file. It is not used for MATTER OEMiROT example but it is provided 
      as an example if you want to update the firmware via an UART link.

  To flash the STM32WBA65I-DK1 with the OEMiROT binaries, two scripts updated by the compilation process are used.
  The scripts can be found in the directory Projects\STM32WBA65I-DK1\ROT_Provisioning\OEMiROT



