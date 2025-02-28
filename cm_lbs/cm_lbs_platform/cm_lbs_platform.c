/**
 *  @file    cm_lbs_platform.c
 *  @brief   LBS定位平台适配层
 *  @copyright copyright © 2022 China Mobile IOT. All rights reserved.
 *  @author by cmiot
 *  @date 2025/02/18
 */

#include <stdint.h>
#include <stdlib.h>

#include "cm_lbs_platform.h"
#include "mbedtls/md5.h"
#include "string.h"


#define LBS_IMSI_LENMX (24)                                          /*IMSI最大长度*/

static uint8_t g_imsi[LBS_IMSI_LENMX] = {0};                         /*获取到的IMSI*/

/*
*@brief 获取默认cid(必选)
*
*@return cid
*/
int __cm_lbs_get_cid(void)
{
    return 7;
}

/*
*@brief 获取网络状态(必选)
*
*@param arg 预留
*@return CM_LBS_OK：网络正常 LBS_ERROR：网络异常
*
*/
cm_lbs_state_e __cm_lbs_get_netstate(void *arg)
{
    (void)arg;
    if(cm_get_pdptype() == 0)
    {
        return CM_LBS_ERR;
    }
    return CM_LBS_OK;
}

/*
*@brief 获取IMEI(必选)
*
*@param[in] imei 读取到的IMEI
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_GetImei(uint8_t *imei)
{
    //strcpy((char *)imei, "866818030002271");
    //CMLBS_LOG("%s IMEI=%s",__FUNCTION__,imei);
    //return CM_LBS_OK;
#if 1
    if(0 != cm_sys_get_imei((char *)imei))
    {
        CMLBS_LOG("%s IMEI=%s",__FUNCTION__,imei);
        return CM_LBS_OK;
    }
    return CM_LBS_ERR;
#endif

}

#if 0
/*
*@brief 获取IMSI(必选)
*
*@param[in] imsi 读取到的IMSI
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_GetImsi(uint8_t *imsi)
{
    if(0 != cm_get_imsi((uint8_t *)imsi))
    {
        memset(g_imsi, 0, LBS_IMSI_LENMX);
        int len = strlen((const char *)imsi);
        if(len <= LBS_IMSI_LENMX)
        {
            memcpy(g_imsi, imsi, len);
        }
        else
        {
            return CM_LBS_ERR;
        }
        return CM_LBS_OK;
    }
    return CM_LBS_ERR;
}
#endif 

/*
*@brief 获取mcc(必选)
*
*@param[in] mcc 读取到的mcc
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_GetCell_info(cm_service_cell_info_t *cell_info)
{
    char *rsp = NULL;

    rsp = malloc(128);
    memset(rsp, 0, 128);

    // if(cm_at_send_cmd_to_modem("AT^MONSC\r\n", rsp, 128) < 0)
    // {
    //     cm_lbs_free(rsp);
    //     return CM_LBS_ERR;
    // }

    // CMLBS_LOG("AT^MONSC :%s", rsp);

    char *RAT = NULL;
    char *result_str = strstr(rsp, "^MONSC: ");

    if(result_str == NULL)
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    result_str += strlen("^MONSC: ");
    char *result_next = NULL;
    cm_at_parse_all_parameters(result_str, &result_next, "s", &RAT);

    if(RAT == NULL)
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    char *Cell_ID = NULL;
    char *TAC = NULL;
    char *MCC = NULL;
    char *MNC = NULL;
    uint64_t cell_id_tmp = 0;
    int TAC_temp = 0;
    if(strcasecmp(RAT, "LTE") == 0)
    {
        result_str = result_next;
        char *ARFCN = NULL;
        char *PCI = NULL;
        float RSRP = 0;
        float RSRQ = 0;
        float RSSI = 0;
        cm_at_parse_all_parameters(result_str, &result_next, "s,s,s,s,s,s,f,f,f",
                                   &MCC, &MNC, &ARFCN, &Cell_ID, &PCI, &TAC, &RSRP, &RSRQ, &RSSI);
        cell_info->signal = RSSI;           //RSSI为负值，不需要经过RSSI*2-113转换
    }
    else if(strcasecmp(RAT, "NR") == 0)
    {
        result_str = result_next;
        char *ARFCN = NULL;
        char *SCS = NULL;
        char *PCI = NULL;
        float RSRP = 0;
        float RSRQ = 0;
        float SINR = 0;
        cm_at_parse_all_parameters(result_str, &result_next, "s,s,s,s,s,s,s,f,f,f",
                                   &MCC, &MNC, &ARFCN, &SCS, &Cell_ID, &PCI, &TAC, &RSRP, &RSRQ, &SINR);
        //NR因为获取不到RSSI,暂不支持NR LBS
       cm_lbs_free(rsp);
       return CM_LBS_ERR;
    }
    else
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }
    if(Cell_ID)
    {
        // memcpy(cell_info->cellid, Cell_ID, strlen(Cell_ID));
        cell_id_tmp = (uint64_t)cm_util_hextoDec(Cell_ID);
        sprintf((char*)cell_info->cellid,"%d",(int)cell_id_tmp);
    }
    if(TAC)
    {
        // memcpy(cell_info->lac, "13112", strlen("13112"));
        // CMLBS_LOG("%s tac_temp=\"13112\" tec_origin=%s",__FUNCTION__,(char *)TAC);
        
        TAC_temp = cm_util_hextoDec(TAC);
        sprintf((char*)cell_info->lac,"%d",TAC_temp);
    }
    cell_info->mcc = atoi(MCC);
    cell_info->mnc = atoi(MNC);
    CMLBS_LOG("%s singal=%d",__FUNCTION__,(int)cell_info->signal);
    if(Cell_ID)
    {
        CMLBS_LOG("%s cellid_origin=%s cellid_tmp=%ld cellid=%s",__FUNCTION__, Cell_ID,(long int)cell_id_tmp,(char*)cell_info->cellid);
    }
    if(TAC)
    {
        CMLBS_LOG("%s tac_origin=%s tac_temp=%d tac_real=%s",__FUNCTION__, TAC, TAC_temp, cell_info->lac);
    }
    cm_lbs_free(rsp);
    return CM_LBS_OK;
}

/*
*@brief 获取网络类型（可选项，平台不支持时，直接返回LBS_ERROR即可）
*
*@param[in] network 获取到的网络类型
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetNetwork(uint8_t *network)
{
    return CM_LBS_ERR;
}

/*
*@brief 获取基站新鲜度（使用高德API2.0 的必选项）
*
*@param[in] cage 获取到基站新鲜度
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetCage(uint32_t *cage)
{

    *cage = 5;
    return CM_LBS_OK;
    //    return CM_LBS_ERR;
}

/*
*@brief 获取网关IP（可选项，平台不支持时，直接返回LBS_ERROR即可）
*
*@param[in] serverip 获取到的网关IP
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetServerip(uint8_t *serverip)
{
    return CM_LBS_ERR;
}


typedef struct
{
    int event;
} cm_nearbts_msg_t;


/*
*@brief 获取邻区基站信息（可选项，平台不支持时，直接返回LBS_ERROR即可，不支持时会降低定位精度及动态特性）
*
*@param[in] nearbts 获取到邻区基站信息
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetNearbts(uint8_t *nearbts)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
    if(NULL == nearbts)
    {
        return CM_LBS_ERR;
    }
#if CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT

    char *rsp = NULL;

    rsp = malloc(128);
    memset(rsp, 0, 128);

    if(cm_at_send_cmd_to_modem("AT^MONSC\r\n", rsp, 128) < 0)
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    CMLBS_LOG("AT^MONSC :%s", rsp);

    char *RAT = NULL;
    char *result_str = strstr(rsp, "^MONSC: ");

    if(result_str == NULL)
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    result_str += strlen("^MONSC: ");
    char *result_next = NULL;
    cm_at_parse_all_parameters(result_str, &result_next, "s", &RAT);

    if(RAT == NULL)
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    char *MCC = NULL;
    char *MNC = NULL;
    if(strcasecmp(RAT, "LTE") == 0)
    {
        result_str = result_next;
        cm_at_parse_all_parameters(result_str, &result_next, "s,s",
                                   &MCC, &MNC);
    }
    else if(strcasecmp(RAT, "NR") == 0)
    {
        result_str = result_next;
        cm_at_parse_all_parameters(result_str, &result_next, "s,s",
                                   &MCC, &MNC);
    }
    else
    {
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    rsp = realloc(rsp, 1024);
    memset(rsp, 0, 1024);
    if(cm_at_send_cmd_to_modem("AT^MONNC\r\n", rsp, 1024) < 0)
    {
        CMLBS_LOGE("AT^MONNC ERROR");
        cm_lbs_free(rsp);
        return CM_LBS_ERR;
    }

    //cm_log_printf("rsp len:%d, AT^MONNC :%s", strlen(rsp), rsp);
    char *proc_buff = rsp;
    char *oneline_str = NULL;
    char *nextline_str = NULL;
    out_len += sprintf(nearbts + out_len, "nearbts=");

    do
    {
        RAT = NULL;
        result_str = strstr(proc_buff, "^MONNC: ");

        if(result_str == NULL)
        {
            CMLBS_LOGE("FIND ^MONNC ERROR");
            cm_lbs_free(rsp);
            return CM_LBS_ERR;
        }

        result_str += strlen("^MONNC: ");
        oneline_str = cm_freqlock_getoneline(result_str, &nextline_str);
        //cm_log_printf("oneline_str: %s", result_str);
        //cm_log_printf("nextline_str: %s", nextline_str);
        result_next = NULL;
        cm_at_parse_all_parameters(oneline_str, &result_next, "s", &RAT);

        if(RAT == NULL)
        {
            CMLBS_LOGE("^MONNC RAT ERROR");
            cm_lbs_free(rsp);
            return CM_LBS_ERR;
        }
        else if(strcasecmp(RAT, "LTE") == 0)
        {
            result_str = result_next;
            char *ARFCN = NULL;
            char *PCI = NULL;
            float RSRP = 0;
            float RSRQ = 0;
            float SINR = 0;
            uint64_t PCI_Dec = 0;
            result_next = NULL;
            cm_at_parse_all_parameters(result_str, &result_next, "s,s,f,f,f", &ARFCN, &PCI, &RSRP, &RSRQ, &SINR);
            PCI_Dec = cm_util_hextoDec(PCI);
            //
            out_len += sprintf(nearbts + out_len, "%s,%s,,%d,|\r\n", MCC, MNC, PCI_Dec);
            //cm_uart_printf(atOpId, (uint8_t *)out);
        }
        else
        {
            continue;
        }
        if(nextline_str)
        {
            proc_buff = nextline_str;
        }
        else
        {
            break;
        }
        
        //cm_log_printf("out_len: %d", out_len);
    }
    while (out_len  < sizeof(nearbts) - 64);
    cm_lbs_free(rsp);
#endif /*CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT*/
    return ret;

}



/**
 * @brief 延时函数(可选)
 *
 * @param [in] time_ms    
 */
void cm_lbs_delay(uint32_t time_ms)
{
    /* Ticks to sleep.(1s=200ticks) */
    uv_sleep(time_ms);
}

/**
 * @brief URC上报函数(必选)
 *
 * @param [in] arg   urc函数参数
 * @param [in] urc_buf URC上报的字符串
 */
void cm_lbs_urc_rsp(void *arg, char *urc_buf)
{
    #if 0
    CMLBS_LOG("%s URC atOpId=%d urc=%s",__FUNCTION__,g_lbs_atOpId,urc_buf);
    cm_uart_printf_urc_data((void *)g_lbs_atOpId,(const uint8_t *)urc_buf,strlen(urc_buf),NULL,0);
    #endif
    // cm_uart_printf_data(g_lbs_atOpId, (const uint8_t *)urc_buf, strlen(urc_buf), NULL, 0, false, false);     //应采用异步接口
}

static void __cm_lbs_data_to_hex(char *hex_ptr, char *bin_ptr, int length)
{
    uint8_t semi_octet = 0;
    int i = 0;

    for(i = 0; i < length; i++)
    {
        // get the high 4 bits
        semi_octet = (char)((bin_ptr[i] & 0xF0) >> 4);
        if(semi_octet <= 9)  //semi_octet >= 0
        {
            *hex_ptr = (char)(semi_octet + '0');
        }
        else
        {
            *hex_ptr = (char)(semi_octet + 'A' - 10);
        }

        hex_ptr++;

        // get the low 4 bits
        semi_octet = (char)(bin_ptr[i] & 0x0f);
        if(semi_octet <= 9)  // semi_octet >= 0
        {
            *hex_ptr = (char)(semi_octet + '0');
        }
        else
        {
            *hex_ptr = (char)(semi_octet + 'A' - 10);
        }
        hex_ptr++;
    }
}

/**
 * @brief 计算MD5函数(必选)
 *
 * @param [in] input   需要计算的值
 * @param [in] output  计算后MD5值
 */
void cm_lbs_md5(char *input, char *output)
{
    
    int len = strlen(input);
    mbedtls_md5_context  md5;

    mbedtls_md5_init(&md5);
    mbedtls_md5_starts(&md5);
    mbedtls_md5_update(&md5, (void *)input, len);
    mbedtls_md5_finish(&md5, (void *)input);
    __cm_lbs_data_to_hex(output, input, 16);
    
}

#if 1

//#include "MRD.h"
//#include "RDisk.h"
//#include "utilities.h"

#define CM_LBS_NVCONFIG_FILE_NAME "lbsnvconfig.bin"
#define CM_LBS_NVCONFIG_FILE_FULL_NAME "lbsnvconfig"
//extern MRDErrorCodes MRDInitNucleus(void);

/*写入配置NV区域(重新烧录程序不会擦除的区域，可选项，不支持直接返回CM_LBS_ERR即可) */
cm_lbs_state_e cm_lbs_writenv_config(void *cfg, int size)
{
#if 0
#ifdef  CM_LBS_PLATFORM_NVRW_SUPPORT
    MRDErrorCodes mrdrc;
    char Date[9 + 1] = {"12NOV2021"};
    char Version[4 + 1] = {"0101"};
    UINT32 version;
    UINT32 date = 0;
    FILE_ID LbsnvconfigFile = NULL;
    MRDErrorCodes ret_val = MRD_NO_ERROR;

    ret_val = MRDInitNucleus();
    if(ret_val != MRD_NO_ERROR)
    {
        return CM_LBS_ERR;
    }
    date = GetDate(Date);
    version = atoi(Version);

    mrdrc = MRDFileRemove(CM_LBS_NVCONFIG_FILE_NAME, (UINT32)MRD_LBS_NVCONFIG_TYPE);
    if(mrdrc != MRD_NO_ERROR)
    {
        cm_log("lbs MRDFileRemove %d\n", mrdrc);
    }


    LbsnvconfigFile = Rdisk_fopen(CM_LBS_NVCONFIG_FILE_FULL_NAME);
    if(LbsnvconfigFile != NULL)
    {
        Rdisk_fwrite((const char *)cfg, 1, size, LbsnvconfigFile);
        Rdisk_fclose(LbsnvconfigFile);
    }
    else
    {
        cm_log("Lbsoneospospid is NULL\n");
    }

    mrdrc = MRDFileAdd(CM_LBS_NVCONFIG_FILE_NAME, 0, (UINT32)MRD_LBS_NVCONFIG_TYPE, version, date, CM_LBS_NVCONFIG_FILE_FULL_NAME);

    // Write back MRD to flash
    // write or not write is handled in this API
    MRDClose();
    if(mrdrc == MRD_NO_ERROR)
    {
        cm_log("Lbsoneospospid Write ok");
        return CM_LBS_OK;
    }

#endif /*CM_LBS_PLATFORM_NVRW_SUPPORT*/

#endif
    return CM_LBS_ERR;
}

/*读取NV区域的数据，可选项，不支持时直接返回CM_LBS_ERR即可*/
cm_lbs_state_e cm_lbs_readnv_config(void *cfg, int size)
{
#if 0
#ifdef  CM_LBS_PLATFORM_NVRW_SUPPORT
    UINT32  version;
    UINT32  date = 0;
    FILE_ID LbsnvconfigFile = NULL;

    int ret = MRDInitNucleus();
    if(ret != MRD_NO_ERROR)
    {
        return CM_LBS_ERR;
    }

    if(MRDFileRead(CM_LBS_NVCONFIG_FILE_NAME, (UINT32)MRD_LBS_NVCONFIG_TYPE, 0, &version, &date, CM_LBS_NVCONFIG_FILE_FULL_NAME) == MRD_NO_ERROR)
    {
        LbsnvconfigFile = Rdisk_fopen(CM_LBS_NVCONFIG_FILE_FULL_NAME);
        if(LbsnvconfigFile != NULL)
        {
            Rdisk_fread((char *)cfg, size, 1, LbsnvconfigFile);
            Rdisk_fclose(LbsnvconfigFile);
            MRDClose();
            return CM_LBS_OK;
        }
    }
    MRDClose();
#endif /*CM_LBS_PLATFORM_NVRW_SUPPORT*/
#endif
    return CM_LBS_ERR;
}

#endif





