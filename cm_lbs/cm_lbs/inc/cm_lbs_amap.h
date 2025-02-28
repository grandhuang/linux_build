/**
 * @file        cm_lbs_amap.h
 * @brief       高德定位API接口
 * @copyright   Copyright © 2025 China Mobile IOT. All rights reserved.
 * @author      By cmiot
 * @date        2025/02/18
 *
 * @defgroup lbs
 * @ingroup LBS
 * @{
 */


#ifndef __CM_LBS_AMAP_H__
#define __CM_LBS_AMAP_H__

#include "cm_lbs_platform.h"
#include "cJSON.h"
// #include "cm_common_at.h"
#include "cm_lbs.h"


#define CM_LBS_AMAPV1_SUPPORT CM_LBS_PLATFORM_AMAPV1_SUPPORT      /*是否支持高德V1.0 LBS接口*/
#define CM_LBS_AMAPV2_SUPPORT CM_LBS_PLATFORM_AMAPV2_SUPPORT      /*是否支持高德V2.0 LBS接口*/

#define CM_LBS_AMAPV1 (10)                          /*高德LBS定位1.0*/
#define CM_LBS_AMAPV2 (11)                          /*高德LBS定位2.0*/

#define CM_LBS_AMAPV1_URL "http://apilocate.amap.com" /*高德LBS 1.0 URL*/
#define CM_LBS_AMAPV2_URL "http://restapi.amap.com"   /*高德LBS 2.0 URL*/

#define CM_LBS_AMAPV1_PATH "/position?"           /*高德LBS 1.0 请求路径*/
#define CM_LBS_AMAPV2_PATH "/v5/position/IoT?"    /*高德LBS 2.0 请求路径*/

#define CM_LBS_AMAP_APIKET_LEN (128)              /*高德LBS apikey 最大长度*/
#define CM_LBS_AMAP_SIGNATUREKEY_LEN (128)        /*高德LBS 数字签名私钥最大长度*/
#define CM_LBS_AMAP_RSP_LENMAX (256)              /*定位结果单个值最大长度*/

#define CM_LBS_AMAP_LOCATION_RSP_VALID_TIME (2*60*60) /*定位结果缓存最大有效时间 2h*/


#define CM_LBS_UEINFO_SERVERIP_SUPPORT CM_LBS_PLATFORM_UEINFO_SERVERIP_SUPPORT             /*是否支持获取基站网关IP*/
#define CM_LBS_UEINFO_NEARBETS_SUPPORT CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT             /*是否支持获取周边基站信息*/
#define CM_LBS_UEINFO_NETEWORK_TYPE_SUPPORT CM_LBS_PLATFORM_UEINFO_NETEWORK_TYPE_SUPPORT   /*是否支持获取无线网络类型*/





/*高德LBS定位回调事件*/
typedef enum
{
    AMAP_LOCATION_SUCCEED = 1,                      /*定位成功*/
    AMAP_LOCATION_FAIL,                             /*定位失败*/
    AMAP_LOCATION_TIMEOUT,                          /*定位超时*/
    AMAP_LOCATION_CONNECTFAILL,                     /*连接错误*/
    AMAP_LOCATION_KEY_INVALID,                      /*key非法或过期*/
    AMAP_LOCATION_OVER_DAYQUOTA,                    /*请求超出日配额*/
    AMAP_LOCATION_OVER_ALLQUOTA,                    /*请求超出总配额*/
    AMAP_LOCATION_OVER_QPS,                         /*请求超出并发量*/
    AMAP_LOCATION_GET_UEINFO_FAIL,
    AMAP_LOCATION_APIKEY_FOBIDEN,
    AMAP_LOCATION_UNKNOWN_ERR                       /*未知错误*/
} cm_lbs_amap_event_e;

/*高德LBS定位结果*/
typedef struct
{
    cm_lbs_amap_event_e event;      /*定位结果事件*/
    char *longitude;                /*!<经度*/
    char *latitude;                 /*!<纬度*/
    char *radius;                   /*!<精度半径*/
    char *country;                  /*!<国家*/
    char *province;                 /*!<省份*/
    char *city;                     /*!<市*/
    char *district;                 /*!<区*/
    char *citycode;                 /*!<城市编码*/
    char *adcode;                   /*!<区域编码*/
    char *street;                   /*!<街道名称*/
    char *road;                     /*!<道路名称*/
    char *poi;                      /*!<附近POI名称*/
    char *location_describe;        /*!<具体位置描述*/

} cm_lbs_amap_location_rsp_t;


/*高德LBS定位回调函数*/
typedef void (*cm_lbs_amap_location_callback)(cm_lbs_amap_event_e rsp_event, cm_lbs_amap_location_rsp_t *location_rsp, void *arg);

/*设备信息*/
typedef struct
{
    uint8_t imei[24];                           /*imei*/
    uint8_t imsi[24];                           /*imsi*/
    /*高德LBS 2.0 专有参数*/
    uint32_t cage;                              /*基站新鲜度 单位：秒，表示信号不变持续时间，值越大数据越陈旧，偏离实际位置越多*/
    cm_service_cell_info_t cell_info;

    uint8_t *network_type;                      /*无线网络类型GSM/GPRS/EDGE/HSUPA/HSDPA/WCDMA*/
    /*可选参数*/
    uint8_t *nearbts;                           /*周边基站信息 格式：基站信息1|基站信息2|*/
    uint8_t *server_ip;                         /*设备接入基站时对应网关ip*/
} cm_amap_ue_info_t;

/*鉴权信息*/
typedef struct
{
    int platform;                                /*!定位所用平台*/
    int digital_sign_enable;                     /*!使能数字签名*/
    char *digital_sign_key;                      /*!数字签名私匙*/
    char *apikey;                                /*!apikey*/
    int time_out;                                /*!定位超时时间*/
    int show_fields_enable;                      /*!高德定位2.0是否请求具体的位置描述 0：不请求 1：请求*/
} cm_amap_authentication_t;

typedef enum
{
    AMAP_FREE = 0,
    AMAP_BUSY
} cm_amap_state_e;

/*高德定位所需信息*/
typedef struct
{
    CURL *http_client;                             /*http句柄*/
    cm_curl_http_client_t curl_client_cfg;         /*http句柄信息*/
    cm_amap_state_e state;                         /*amap状态*/
    cm_amap_ue_info_t *ueinfo;                     /*UE信息*/
    cm_amap_authentication_t *auth;                /*鉴权信息*/
    cm_lbs_amap_location_callback cb;              /*定位回调*/
    void *cb_arg;                                  /*回调参数*/
    int nearbts_enable;                            /*是否使用邻区信息*/
} cm_amap_t;


#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif


/*
*@brief 获取定位信息
*
*@param[in] attr 定位参数
*
*@return CM_LBS_OK:执行成功 其他：失败（详见cm_lbs_state_e）
*
*/
cm_lbs_state_e cm_amap_location(cm_amap_t *attr);

/*
*@brief 获取设备信息
*
*@param[in] ueinfo 设备信息
*
*@return CM_LBS_OK:执行成功 其他：失败（详见cm_lbs_state_e）
*
*/
cm_lbs_state_e cm_amap_get_ueinfo(cm_amap_ue_info_t **ue_info,int nearbts_enable);

/*
*@brief 释放设备信息
*
*@param[in] ueinfo 设备信息
*
*/
void cm_amap_free_ueinfo(cm_amap_ue_info_t *ue_info);


#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif
