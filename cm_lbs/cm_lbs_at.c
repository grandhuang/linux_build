#include "stdlib.h"
#include "string.h"
#include "cm_lbs_at.h"
#include "cm_lbs.h"
#include "cm_lbs_api.h"
#include "cm_common_at.h"

// AT+MLBSCFG=?
int32_t CMIOT_MLBSCFG_Cmd_Test(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, IMMD_AT_DATA_BUF* rsp_ptr)
{
    char rsp[256] = {0};
    cm_lbs_atcmd_MLBSCFG_test((char *)&rsp);
    CM_AT_CMD_RETURN(chn_ptr, RET_OK, rsp_ptr, rsp, strlen(rsp));
}

// AT+MLBSCFG=xxx
int32_t CMIOT_MLBSCFG_Cmd_Set(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, uint8_t* param_ptr, IMMD_AT_DATA_BUF* rsp_ptr)
{
    char *start = param_ptr;
    char *next = NULL;

    char *subcmd_str = NULL;
    char *vuecmd_str = NULL;

    int param_num = cm_at_parse_all_parameters(start, &next, "s,s", &subcmd_str, &vuecmd_str);

    if(param_num <= 0 || strlen(subcmd_str) == 0 || param_num > 2 || next != NULL)
    {
        CMLBS_LOGE("parameters1  error");
        CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_INCORRECT_PARAMETERS, rsp_ptr, NULL, 0);
    }

    if(param_num == 1)
    {
        char rsp_test[100] = {0};
        cm_lbs_state_e fun_ret = cm_lbs_atcmd_MLBSCFG_get(subcmd_str, (char *)&rsp_test);
        if(CM_LBS_OK == fun_ret)
        {
            CM_AT_CMD_RETURN(chn_ptr, RET_OK, rsp_ptr, rsp_test, strlen(rsp_test));
        }
        else
        {
            CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_INCORRECT_PARAMETERS, rsp_ptr, NULL, 0);
        }
    }

    cm_lbs_state_e fun_ret = cm_lbs_atcmd_MLBSCFG_set(subcmd_str, vuecmd_str);
    if(CM_LBS_OK != fun_ret)
    {
        CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_INCORRECT_PARAMETERS, rsp_ptr, NULL, 0);
    }
    else
    {
        CM_AT_CMD_RETURN(chn_ptr, RET_OK, rsp_ptr, NULL, 0);
    }
}

// AT+MLBSLOC
int32_t CMIOT_MLBSLOC_CmdProc(void* chn_ptr, IMMD_TELEPHONY_ID_EN tel_id, uint8_t* param_ptr, IMMD_AT_DATA_BUF* rsp_ptr)
{
    static uint32_t cb_arg = 0;

    /*获取定位信息*/
    cm_lbs_atcmd_errcode_e loc_ret = cm_lbs_atcmd_MLBSLOC(&cb_arg);

    if(LBS_ATCMD_NETERR == loc_ret)
    {
        CMLBS_LOGE("NET ERROR!");
        CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_NOT_NETWORK_SERVICE, rsp_ptr, NULL, 0);
    }
    if(LBS_ATCMD_APIKEYNULL == loc_ret || LBS_ATCMD_PARAERR == loc_ret)
    {
        CMLBS_LOGE("APIKEYNULL!");
        CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_INCORRECT_PARAMETERS, rsp_ptr, NULL, 0);
    }
    if(LBS_ATCMD_BUSY == loc_ret)
    {
        CMLBS_LOGE("AT CMD busy!");
        CM_AT_CMD_RETURN(chn_ptr, CME_ERROR_GENERAL_TYPE_OPERATION_NOT_ALLOWED, rsp_ptr, NULL, 0);
    }
    if(LBS_ATCMD_SUCCESS == loc_ret)
    {
        CM_AT_CMD_RETURN(chn_ptr, RET_OK, rsp_ptr, NULL, 0);
    }
}
