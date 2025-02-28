/**
 * @file        cm_lbs_oneospos.h
 * @brief       ONEOS 定位服务API接口
 * @copyright   Copyright © 2025 China Mobile IOT. All rights reserved.
 * @author      By cmiot
 * @date        2025/02/18
 *
 * @defgroup lbs
 * @ingroup LBS
 * @{
 */

#ifndef __CM_LBS_ONEOSPOS_H__
#define __CM_LBS_ONEOSPOS_H__

#include "cm_lbs_platform.h"
#include "cm_lbs_api.h"
#include "cJSON.h"
// #include "cm_common_at.h"
#include "coordinate.h"


#define CM_LBS_ONEOSPOS_PID_LEN (64)                       /*oneospos平台 pid最大长度*/
#define CM_LBS_ONEOSPOS_LOCATION_RSP_VALID_TIME (2*60*60)  /*定位结果缓存有效时间 2h */


   

/*定位回调事件*/
typedef enum 
{
  CM_ONEOSPOS_EVENT_UNKNOW=-3,                      /*未知错误*/  
  CM_ONEOSPOS_EVENT_CONERR=-2,                      /*连接错误*/
  CM_ONEOSPOS_EVENT_TIMEOUT=-1,                     /*定位超时*/ 
  CM_ONEOSPOS_EVENT_SUCCESS=0,                      /*定位成功*/
  CM_ONEOSPOS_EVENT_FAIL,                           /*无位置信息*/  
  CM_ONEOSPOS_EVENT_OVER_RESTRICT,                  /*并发量或调用量超限*/
  CM_ONEOSPOS_EVENT_UNSET_PLATFORM,                 /*产品未配置服务器*/ 
  CM_ONEOSPOS_EVENT_UEINFO_FAIL,
  CM_ONEOSPOS_EVENT_ATTR_NULL
}cm_oneospos_callback_event_e;

/*定位所需UE信息*/
typedef struct 
{
    uint8_t imei[24];                               /*imei*/
    uint8_t imsi[24];                               /*imsi*/
    //uint8_t lac[24];                                /*位置区域码  在2G/3G中为  LAC 在    LTE中为TAC*/
    //uint8_t cellid[24];                             /*基站小区编号*/
    //int mcc;                                        /*国家代码*/
    //int mnc;                                        /*移动网号 中国移动：0; 中国联通：1*/
    //int signal;                                     /*信号强度 0~113dbm*/
    cm_service_cell_info_t cell_info;
    /*可选参数*/   
    uint8_t *nearbts;                               /*周边基站信息 格式：基站信息1|基站信息2|*/
}cm_oneospos_ueinfo_t;



/*定位结果*/
typedef struct
{
  cm_oneospos_callback_event_e event;               /*回调事件，定位结果*/
  char *latitude;                                   /*纬度*/
  char *longitude;                                  /*经度*/
}cm_oneospos_location_rsp_t;


/*定位回调函数*/
typedef void  (*cm_oneospos_callback)(cm_oneospos_callback_event_e event,cm_oneospos_location_rsp_t *rsp,void *cb_arg);

/*ONEOS 定位配置信息*/
typedef struct
{
  char *pid;                                        /*设备pid*/
  cm_oneospos_ueinfo_t *ueinfo;                     /*定位所需信息*/
  cm_oneospos_callback cb;                          /*回调函数*/
  void *cb_arg;                                     /*回调参数*/
  int nearbts_enable;                               /*是否启用邻区*/
  int time_out;                                     /*请求超时时间*/
}cm_oneospos_attr_t;

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif


/*
*@brief 执行oneos平台基站定位
*
*@param[in] attr 定位参数
*
*@return 成功:CM_ONEOSPOS_SUCCESS  失败:（详见cm_oneospos_err_e)
*/
int cm_oneospos_location(cm_oneospos_attr_t *attr);

/*
*@brief 获取定位所需设备信息
*
*@param[in] ueinfo 设备信息
*
**@return 成功:CM_ONEOSPOS_SUCCESS  失败:（详见cm_oneospos_err_e)
*/
int cm_oneospos_get_ueinfo(cm_oneospos_ueinfo_t **ueinfo,int nearbts_enable);

/*
*@brief 释放设备信息资源
*
*@param[in] ueinfo 设备信息
*
*/
void cm_oneospos_free_ueinfo(cm_oneospos_ueinfo_t *ue_info);


/*
*@brief 复制attr
*
*@param[in] src_artt 输入的attr数据
*@param[in] dec_attr 复制的结果
*/
void cm_oneospos_create_attr(cm_oneospos_attr_t *src_attr, cm_oneospos_attr_t **dec_attr);



#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /*__CM_LBS_ONEOSPOS_H__*/
