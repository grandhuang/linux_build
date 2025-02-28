/**
 * @file        cm_lbs_api.h
 * @brief       LBS AT层接口
 * @copyright   Copyright © 2025 China Mobile IOT. All rights reserved.
 * @author      By cmiot
 * @date        2025/02/18
 *
 * @defgroup lbs
 * @ingroup LBS
 * @{
 */

#ifndef __CM_LBS_API_H__
#define __CM_LBS_API_H__

#include <string.h>
#include <stdlib.h>

#include "cm_lbs.h"
#include "cm_lbs_platform.h"
#include "cm_lbs_amap.h"
#include "cm_lbs_oneospos.h"



/************************************ML302A内置oneos pos key***************************************/
#ifdef ML302A_SUPPORT
    #ifdef GPS_SUPPORT
        #ifdef CRANEL_4MRAM
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "tZku0CrbZs"     /*ML302A_GCLM*/
        #else
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "yYCS04BUL5"     /*ML302A_GSLM*/
        #endif /*CRANEL_4MRAM*/
    #else
        #ifdef CRANEL_4MRAM
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "EgxMgmRy2c"     /*ML302A_DCLM*/
        #else
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "b3v3npaCbN"     /*ML302A_DSLM*/
        #endif /*CRANEL_4MRAM*/
    #endif/*GPS_SUPPORT*/
#endif /*ML302A_SUPPORT*/

/*************************************ML307A内置oneos pos key**************************************/
#ifdef ML307A_SUPPORT
    #ifdef GPS_SUPPORT
        #ifdef CRANEL_4MRAM
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "hdYY4DcAIK"     /*ML307A_GCLN*/
        #else
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "VcZ10hX9ka"     /*ML307A_GSLN*/
        #endif /*CRANEL_4MRAM*/
    #else
        #ifdef CRANEL_4MRAM
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "auznwH4g4x"     /*ML307A_DCLN*/
        #else
            #define CM_LBS_ONEOSPOS_INTERNAL_PID  "R80tzf6dzw"     /*ML307A_DSLN*/
        #endif /*CRANEL_4MRAM*/
    #endif/*GPS_SUPPORT*/
#endif /*ML307A_SUPPORT*/
/*************************************ML305A内置oneos pos key**************************************/
#ifndef CM_LBS_ONEOSPOS_INTERNAL_PID
    #ifdef CRANEL_8MRAM
        #define CM_LBS_ONEOSPOS_INTERNAL_PID  "NRycjZfME4"     /*ML305A_DS*/
    #elif  CRANEL_4MRAM
        #define CM_LBS_ONEOSPOS_INTERNAL_PID  "XCxPc61D6X"     /*ML305A_DC*/
    #else
        #define CM_LBS_ONEOSPOS_INTERNAL_PID  "qKQjFpRIvY"     /*ML305A_DL*/
    #endif
#endif/*CM_LBS_ONEOSPOS_INTERNAL_PID*/

/*************************************ML305A内置oneos pos key**************************************/


#define CM_LBS_PRECISION_MIN (1)                         /*经纬度输出小数位数最小值*/
#define CM_LBS_PRECISION_DEFAULT (8)                     /*经纬度输出小数位数默认值*/
#define CM_LBS_PRECISION_MAX (8)                         /*经纬度输出小数位数最大值*/

#define CM_LBS_DESCR_MODE_MIN (0)                        /*是否允许输出具体位置描述配置最小值*/
#define CM_LBS_DESCR_MODE_DEFAULT (0)                    /*是否允许输出具体位置描述配置默认值*/
#define CM_LBS_DESCR_MODE_MAX (1)                        /*是否允许输出具体位置描述配置最大值*/

#define CM_LBS_LOATION_TIMEOUT (10)                      /*定位超时时间 sec*/

#if (!CM_LBS_AMAPV1_SUPPORT&&!CM_LBS_AMAPV1_SUPPORT)
    #define CM_LBS_METHOD_DEFAULT CM_LBS_PLAT_ONEOSPOS    /*默认使用的平台*/
#else
    #define CM_LBS_METHOD_DEFAULT CM_LBS_PLAT_ONEOSPOS
#endif /*(!CM_LBS_AMAPV1_SUPPORT&&!CM_LBS_AMAPV1_SUPPORT)*/



#define CM_LBS_SIGNATURE_EN_DEFAULT (0)                  /*是否启用数字签名*/
#define CM_LBS_NEARBTS_ENABLE (0)                        /*是否启用邻区信息*/


#define CM_LBS_ATCMD_URCLEN_MAX  (2048)                  /*URC上报最大数据长度*/
#define CM_LBS_ATCMD_URCDESCRLEN_MAX (2048-512)          /*详细位置描述最大长度(用URC上报长度减去500计算所得)*/

#define CM_LBS_ONEOS_COORDINATE (1)                      /*oneos是否启动编码转换*/


/*执行指令返回错误码*/
typedef enum
{
    LBS_ATCMD_SUCCESS = 100,                             /*执行成功*/
    LBS_ATCMD_NETERR = 101,                              /*网络错误*/
    LBS_ATCMD_APIKEYNULL = 102,                          /*未配置APIKEY*/
    LBS_ATCMD_UNKNOWERR = 103,                           /*未知错误*/
    LBS_ATCMD_PARAERR = 104,                             /*参数错误*/
    LBS_ATCMD_BUSY                                       /*LBS忙*/
} cm_lbs_atcmd_errcode_e;


/*************************************************************************
* Public Function Prototypes
**************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 * @brief LBS分配内存空间
 *
 * @param [in] size     分配大小
 *
 * @return
 *      NULL        失败
 *      非NULL       内存地址
 *
 * @details 仅用于LBS malloc接口
 */
void *cm_lbs_malloc(uint32_t size);

/**
 * @brief LBS释放内存空间
 *
 * @param [in] mem     分配的内存地址
 *
 * @return
 *
 * @details 仅用于LBS free接口
 */
void cm_lbs_free(void *mem);

/*
*@brief AT+MLBSCFG测试指令
*
*@param[in] at_send 执行测试命令需要返回的数据
*/
void cm_lbs_atcmd_MLBSCFG_test(char *at_send);


/*
*@brief AT+MLBSCFG 读取参数
*
*@param[in] subcmd AT+MLBSCFG的首个参数
*@param[in] at_send 执行后AT口需要返回的数据
*
*@return CM_LBS_OK :执行成功  其他：失败（详见cm_lbs_state_e）
*/
cm_lbs_state_e cm_lbs_atcmd_MLBSCFG_get(char *subcmd, char *at_send);

/*
*@brief AT+MLBSCFG 设置参数
*
*@param[in] subcmd AT+MLBSCFG的首个参数
*@param[in] value  AT+MLBSCFG的第二个参数
*
*@return CM_LBS_OK :执行成功  其他：失败（详见cm_lbs_state_e）
*/
cm_lbs_state_e cm_lbs_atcmd_MLBSCFG_set(char *subcmd, void *value);

/*
*@brief AT+MLBSLOC 执行定位
*
*@param[in] cb_arg 回调参数（可为NULL）
*
*@return LBS_ATCMD_SUCCESS :执行成功  其他：失败（详见cm_lbs_atcmd_errcode_e）
*/
cm_lbs_atcmd_errcode_e cm_lbs_atcmd_MLBSLOC(void *cb_arg);


/*
*@brief AT+MLBS 执行定位(兼容送样版本的指令AT+MLBS)
*
*@param[in] cb_arg 回调参数（可为NULL）
*
*@return LBS_ATCMD_SUCCESS :执行成功  其他：失败（详见cm_lbs_atcmd_errcode_e）
*/
cm_lbs_atcmd_errcode_e cm_lbs_atcmd_MLBS(void *cb_arg);

/*
*@brief AT+MLBSFACTORY 工厂读写NV操作
*
*@param[in] subcmd 写入选项
*@param[in] arg    写入参数
*@param[in] at_rsp 查询时的AT返回值
*
*
*@return LBS_ATCMD_SUCCESS :执行成功  其他：失败（详见cm_lbs_atcmd_errcode_e）
*/
cm_lbs_atcmd_errcode_e cm_lbs_atcmd_mlbsfact(char *subcmd, void *arg, char *at_rsp);



#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /*__CM_LBS_API_H__*/

/** @}*/

