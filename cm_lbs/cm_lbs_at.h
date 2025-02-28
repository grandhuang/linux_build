/**
 * @file        cm_lbs_at.h
 * @brief       LBS扩展指令
 * @copyright   Copyright © 2025 China Mobile IOT. All rights reserved.
 * @author      By cmiot
 * @date        2025/02/18
 *
 * @defgroup AT-LBS
 * @ingroup AT-LBS
 * @{
 */
    
#ifndef CM_ATCMD_LBS_H_    
#define CM_ATCMD_LBS_H_    
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "immd_common.h"
#include "immd_command.h"
#include "immd_channel.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/


/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

int32_t CMIOT_MLBSCFG_Cmd_Test(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, IMMD_AT_DATA_BUF* rsp_ptr);

int32_t CMIOT_MLBSCFG_Cmd_Set(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, uint8_t* param_ptr, IMMD_AT_DATA_BUF* rsp_ptr);

int32_t CMIOT_MLBSLOC_CmdProc(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, uint8_t* param_ptr, IMMD_AT_DATA_BUF* rsp_ptr);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CM_ATCMD_LBS_H_ */
