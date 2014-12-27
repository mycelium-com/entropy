/**
 * \file
 *
 * \brief Memory access control configuration file.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _CONF_ACCESS_H_
#define _CONF_ACCESS_H_

#include "compiler.h"
#include "board.h"

/*! \name Activation of Logical Unit Numbers
 */
//! @{
#define LUN_0                ENABLE    //!< On-Chip and Serial Flash Memories.
#define LUN_1                DISABLE   //!< Spare
#define LUN_2                DISABLE   //!< Spare
#define LUN_3                DISABLE   //!< Spare
#define LUN_4                DISABLE   //!< Spare
#define LUN_5                DISABLE   //!< Spare
#define LUN_6                DISABLE   //!< Spare
#define LUN_7                DISABLE   //!< Spare
#define LUN_USB              DISABLE   //!< Host Mass-Storage Memory.
//! @}

#define VIRTUAL_MEM          DISABLE
#define SD_MMC_0_MEM         DISABLE
#define SD_MMC_1_MEM         DISABLE
#define MEM_USB              LUN_USB

/*! \name LUN 0 Definitions
 */
//! @{
#define FW_ACCESS                               LUN_0
#define LUN_ID_FW_ACCESS                        LUN_ID_0
#define LUN_0_INCLUDE                           "fw-access.h"
#define Lun_0_test_unit_ready                   fw_test_unit_ready
#define Lun_0_read_capacity                     fw_read_capacity
#define Lun_0_wr_protect                        fw_wr_protect
#define Lun_0_removal                           fw_removal
#define Lun_0_usb_read_10                       fw_usb_read_10
#define Lun_0_usb_write_10                      fw_usb_write_10
#define Lun_0_mem_2_ram                         fw_mem_2_ram
#define Lun_0_ram_2_mem                         fw_ram_2_mem
#define LUN_0_NAME                              "\"IFP Mode 2\""
//! @}

/*! \name Actions Associated with Memory Accesses
 *
 * Write here the action to associate with each memory access.
 *
 * \warning Be careful not to waste time in order not to disturb the functions.
 */
//! @{
#define memory_start_read_action(nb_sectors)
#define memory_stop_read_action()
#define memory_start_write_action(nb_sectors)
#define memory_stop_write_action()
//! @}

/*! \name Activation of Interface Features
 */
//! @{
#define ACCESS_USB           true    //!< MEM <-> USB interface.
#define ACCESS_MEM_TO_RAM    false   //!< MEM <-> RAM interface.
#define ACCESS_STREAM        false   //!< Streaming MEM <-> MEM interface.
#define ACCESS_STREAM_RECORD false   //!< Streaming MEM <-> MEM interface in record mode.
#define ACCESS_MEM_TO_MEM    false   //!< MEM <-> MEM interface.
#define ACCESS_CODEC         false   //!< Codec interface.
//! @}

/*! \name Specific Options for Access Control
 */
//! @{
#define GLOBAL_WR_PROTECT    false   //!< Management of a global write protection.
//! @}


#endif // _CONF_ACCESS_H_
