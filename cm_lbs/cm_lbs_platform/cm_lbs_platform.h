/**
 *  @file    cm_lbs_platform.h
 *  @brief   LBS定位平台适配层
 *  @copyright copyright © 2025 China Mobile IOT. All rights reserved.
 *  @author by cmiot
 *  @date 2025/02/18
 */
#ifndef __CM_LBS_PLATFORM_H__
#define __CM_LBS_PLATFORM_H__

#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <linux/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/time.h>

// #include "immd_command.h"
// #include "cm_common_at.h"
// #include "cm_sys.h"
#include "mbedtls/md5.h"
#include "cJSON.h"
#include "uv.h"
#include "curl/curl.h"

#define __lbs_test_ (0)
#define MR380M_SUPPORT (1)
extern unsigned short g_lbs_atOpId;

/*MR380M LOG: xxxI:info xxxW:warning xxxE:error*/
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_INFO	6	/* informational */
#define LOG_MODULE_AP_IMMD                   0x0000   
#define LOG_LEVEL_AP_HIGH                    LOG_ERR    
#define LOG_LEVEL_AP_MIDDLE                  LOG_WARNING    
#define LOG_LEVEL_AP_LOW                     LOG_INFO
#define LBS_LOGI(arg, ...)        \
                syslog(LOG_MODULE_AP_IMMD|LOG_LEVEL_AP_LOW, arg, ##__VA_ARGS__)
#define LBS_LOGW(arg, ...)        \
                syslog(LOG_MODULE_AP_IMMD|LOG_LEVEL_AP_MIDDLE, arg, ##__VA_ARGS__)
#define LBS_LOGE(arg, ...)        \
                syslog(LOG_MODULE_AP_IMMD|LOG_LEVEL_AP_HIGH, arg, ##__VA_ARGS__)

#define CMLBS_LOG(fmt, args...)    LBS_LOGI("[CMLBS_LOG]%s(%d) "fmt"\r\n", __FUNCTION__, __LINE__, ##args)
#define CMLBS_LOGI(fmt, args...)    LBS_LOGI("[CMLBS_LOGI]%s(%d) "fmt"\r\n", __FUNCTION__, __LINE__, ##args)
#define CMLBS_LOGW(fmt, args...)    LBS_LOGW("[CMLBS_LOGW]%s(%d) "fmt"\r\n", __FUNCTION__, __LINE__, ##args)
#define CMLBS_LOGE(fmt, args...)    LBS_LOGE("[CMLBS_LOGE]%s(%d) "fmt"\r\n", __FUNCTION__, __LINE__, ##args)

/*********************************************OPEN CPU版本 BEGIN****************************************************/

/************************************ML302A OC内置oneos pos key(OC版本)***************************************/
#ifdef ML302A_SUPPORT
    #ifdef GPS_SUPPORT
        #ifdef CRANEL_4MRAM
            #define ONEOSPOS_PID_INTERNAL  "M9lnuxu81f"     /*ML302A_GCLM*/
        #else
            #define ONEOSPOS_PID_INTERNAL  "ZUsVti7GRk"     /*ML302A_GSLM*/
        #endif /*CRANEL_4MRAM*/
    #else
        #ifdef CRANEL_4MRAM
            #define ONEOSPOS_PID_INTERNAL  "nMgrFd35Eq"     /*ML302A_DCLM*/
        #else
            #define ONEOSPOS_PID_INTERNAL  "VuJCIgMPvN"     /*ML302A_DSLM*/
        #endif /*CRANEL_4MRAM*/
    #endif/*GPS_SUPPORT*/
#endif /*ML302A_SUPPORT*/

/*************************************ML307A OC内置oneos pos key**************************************/
#ifdef ML307A_SUPPORT
    #ifdef GPS_SUPPORT
        #ifdef CRANEL_4MRAM
            #define ONEOSPOS_PID_INTERNAL  "VW9h4ybCLK"     /*ML307A_GCLN*/
        #else
            #define ONEOSPOS_PID_INTERNAL  "B7rKhYWAJ1"     /*ML307A_GSLN*/
        #endif /*CRANEL_4MRAM*/
    #else
        #ifdef CRANEL_4MRAM
            #define ONEOSPOS_PID_INTERNAL  "V9PBUrxENI"     /*ML307A_DCLN*/
        #else
            #define ONEOSPOS_PID_INTERNAL  "cfgVs4ukXu"     /*ML307A_DSLN*/
        #endif /*CRANEL_4MRAM*/
    #endif/*GPS_SUPPORT*/
#endif /*ML307A_SUPPORT*/

/*********************************************OPEN CPU版本 END****************************************************/





/*********************************************AT版本 BEGIN****************************************************/

/************************************MR380M 内置oneos pos key(AT版本)***************************************/

#ifdef MR380M_SUPPORT

    #define CM_LBS_ONEOSPOS_INTERNAL_PID  "m5CAIkqYGa"     /*MR380M*/

#endif

/************************************MR880A 内置oneos pos key(AT版本)***************************************/

#ifdef MR880A_SUPPORT

    #define CM_LBS_ONEOSPOS_INTERNAL_PID  "4d9TPJJpQs"     /*MR880A*/

#endif

/************************************ML307X 内置oneos pos key(AT版本)***************************************/

#ifdef ML307X_SUPPORT
    
    #define CM_LBS_ONEOSPOS_INTERNAL_PID  "4d9TPJJpQs"     /*ML307X*/

#endif

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

/*********************************************AT版本 END****************************************************/

#define osWaitForever (0xFFFFFFFF)

/*MR380M 定位平台及AT手动设置的LBS配置存储路径：/userdata/cm_lbs/xxx_lbscfg下*/
#define CM_LBS_CONFIG_FILE_PATH         "/userdata/cm_lbs"                        /*定位平台通用配置文件保存路径*/  
#define CM_LBS_ATCMD_CONFIG_FILE_PATH   "/userdata/cm_lbs/atcmd_lbscfg"           /*AT命令通用配置文件保存路径*/
#define CM_LBS_ATCMD_AMAPV10_FILE_PATH  "/userdata/cm_lbs/amapv10cmd_lbscfg"      /*高德1.0平台配置保存路径*/
#define CM_LBS_ATCMD_AMAPV20_FILE_PATH  "/userdata/cm_lbs/amapv20cmd_lbscfg"      /*高德2.0平台配置保存路径*/
#define CM_LBS_ATCMD_ONEOSPOS_FILE_PATH "/userdata/cm_lbs/oneosposcmd_lbscfg"     /*oneos 平台配置保存路径*/

#ifdef MR380M_SUPPORT
#define CM_LBS_PLATFORM_AMAPV1_SUPPORT (1)
#define CM_LBS_PLATFORM_AMAPV2_SUPPORT (1)
#endif

#define CM_LBS_PLATFORM_UEINFO_SERVERIP_SUPPORT (0)          /*是否支持获取基站网关IP*/
#define CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT (0)          /*是否支持获取周边基站信息*/
#define CM_LBS_PLATFORM_UEINFO_NETEWORK_TYPE_SUPPORT (0)     /*是否支持获取无线网络类型*/

#define CM_LBS_PLATFORM_NVRW_SUPPORT (0)                     /*是否支持NV区域的读写*/

#define LBS_TASKSTACK_LENMAX (1024*3)                        /*LBS主任务栈大小*/

//打印LOG 需根据平台适配

// #define CMLBS_LOG(fmt, args...)   printf("[CMCMLBS_LOG]%s(%d) "fmt"\r\n",__func__, __LINE__, ##args)
// #define CMLBS_LOGI(fmt, args...)   printf("[CMCMLBS_LOGI]%s(%d) "fmt"\r\n",__func__, __LINE__, ##args)
// #define CMLBS_LOGX(fmt, args...)   printf("[CMCMLBS_LOGX]%s(%d) "fmt"\r\n",__func__, __LINE__, ##args)
// #define CMLBS_LOGE(fmt, args...)   printf("[CMCMLBS_LOGE]%s(%d) "fmt"\r\n",__func__, __LINE__, ##args)

#ifndef FALSE
#define FALSE                                               (0U)
#endif

#ifndef TRUE
#define TRUE                                                (1U)
#endif

/**CURLL HTTP可创建实例个数*/
#ifndef MAX_CURL_CLIENT_NUM
#define MAX_CURL_CLIENT_NUM 8
#endif

/*CURL HTTP/HTTPS协议类型 */
#define HTTP_SUPPORT 	(1)
#define HTTPS_SUPPORT 	(0)
#if HTTP_SUPPORT
#define HTTP_TYPE 0
#endif
#if HTTPS_SUPPORT
#define HTTP_TYPE 1
#endif


/**HTTP默认端口*/
#define HTTP_DEFAULT_PORT 80

/**HTTPS默认端口*/
#define HTTPS_DEFAULT_PORT 443

/**HTTP可创建实例个数*/
#ifndef HTTPCLIENT_CTX_MAX_NUM
#define HTTPCLIENT_CTX_MAX_NUM 8
#endif
/**HTTP连接超时最大时间*/
#define HTTPCLIENT_CONNECT_TIMEOUT_MAXTIME 180

/**HTTP请求响应超时最大时间*/
#define HTTPCLIENT_WAITRSP_TIMEOUT_MAXTIME 60

/**HTTP连接超时默认时间*/
#define HTTPCLIENT_CONNECT_TIMEOUT_DEFAULT 60

/**HTTP请求响应超时默认时间*/
#define HTTPCLIENT_WAITRSP_TIMEOUT_DEFAULT 0



/*服务小区信息*/
typedef struct
{
    uint8_t lac[12];                            /*位置区域码  在2G/3G中为  LAC 在    LTE中为TAC*/
    uint8_t cellid[12];                         /*基站小区编号*/
    int mcc;                                    /*国家代码*/
    int mnc;                                    /*移动网号 中国移动：0; 中国联通：1*/
    int signal;                                 /*信号强度 0~113dbm*/
} cm_service_cell_info_t;

/*函数执行错误码*/
typedef enum 
{
  CM_LBS_OK=0,       
  CM_LBS_ERR=-1,     
  CM_LBS_BUSY=-2,       
  CM_LBS_PARAMERR=-3,
  CM_LBS_MEMROYERR=-4,
  CM_LBS_REPEERR
}cm_lbs_state_e;

/**
 * @}
 */

/** @defgroup httpclient_enum Enum
 *  @{
 */

/** @brief   This enumeration defines the http type.  */
typedef enum{
    SCHEME_HTTP = 0,
    SCHEME_HTTPS,
    SCHEME_UNSUPPORT,
} http_type_e;

 /** @brief   This enumeration defines the http parse state.  */
typedef enum 
{
    HTTP_STATE_IND = 0,
    HTTP_STATE_START,
    HTTP_STATE_HEAD_SENDING,
    HTTP_STATE_HEAD_SENT,
    HTTP_STATE_REDIR_START,
    HTTP_STATE_REDIR,
    HTTP_STATE_HEAD_PARSE_START,
    HTTP_STATE_HEAD_PARSE_MORE,
    HTTP_STATE_HEAD_PARSE_FIN,
    HTTP_STATE_DATA_PARSE_START,
    HTTP_STATE_DATA_PARSE_MORE,
    HTTP_STATE_MAX,
} httpclient_state_e;

// /** @brief   This enumeration defines the http task event.  */
// enum
// {
//     HTTP_SIG_CON = HTTP_TSK_EVENT_BASE,             /**< http connect event. */
//     HTTP_SIG_REDIR,                                 /**< http send data event. */
//     HTTP_SIG_RECV,                                  /**< http receive data event. */
//     HTTP_SIG_TERMINATE,                             /**< http terminate event. */
//     HTTP_SIG_CLOSE,                                 /**< http close event. */
//     HTTP_SIG_DELETE,                                /**< delete httpclient event. */
// };

/**HTTP请求类型*/
typedef enum 
{
	HTTPCLIENT_REQUEST_NONE = 0,
	HTTPCLIENT_REQUEST_GET,
	HTTPCLIENT_REQUEST_POST,
	HTTPCLIENT_REQUEST_PUT,
	HTTPCLIENT_REQUEST_DELETE,
	HTTPCLIENT_REQUEST_HEAD,
	HTTPCLIENT_REQUEST_MAX
} cm_httpclient_request_type_e;

/**HTTP回调事件*/
typedef enum 
{
	CM_HTTP_CALLBACK_EVENT_REQ_START_SUCC_IND = 1,      /**< 请求启动成功事件.  */
	CM_HTTP_CALLBACK_EVENT_RSP_HEADER_IND,              /**< 接收到报头事件.    */
	CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND,             /**< 接收到消息体事件.  */
	CM_HTTP_CALLBACK_EVENT_RSP_END_IND,                 /**< 请求响应结束事件.  */
	CM_HTTP_CALLBACK_EVENT_ERROR_IND,                   /**< 请求失败事件.      */
} cm_httpclient_callback_event_e;

/**HTTP异常状态码*/
typedef enum 
{
	CM_HTTP_EVENT_CODE_DNS_FAIL = 1,                    /**< DNS解析失败.      */
	CM_HTTP_EVENT_CODE_CONNECT_FAIL,                    /**< 连接服务器失败.      */
	CM_HTTP_EVENT_CODE_CONNECT_TIMEOUT,                 /**< 连接超时.      */
	CM_HTTP_EVENT_CODE_SSL_CONNECT_FAIL,                /**< SSL握手失败.      */
	CM_HTTP_EVENT_CODE_CONNECT_BREAK,                   /**< 连接异常断开.      */
	CM_HTTP_EVENT_CODE_WAITRSP_TIMEOUT,                 /**< 等待响应超时.      */
	CM_HTTP_EVENT_CODE_DATA_PARSE_FAIL,                 /**< 数据解析失败.      */
	CM_HTTP_EVENT_CODE_CACHR_NOT_ENOUGH,                /**< 缓存空间不足.      */
	CM_HTTP_EVENT_CODE_DATA_DROP,                       /**< 数据丢包.      */
	CM_HTTP_EVENT_CODE_WRITE_FILE_FAIL,                 /**< 写文件失败.      */
	CM_HTTP_EVENT_CODE_UNKNOWN = 255,                   /**< 未知错误.      */
} cm_httpclient_error_event_e;

typedef enum
{
	CM_CURL_RET_CODE_OK = 0,
	CM_CURL_RET_CODE_CLIENT_IS_BUSY,
	CM_CURL_RET_CODE_PARAM_ERROR,

	CM_CURL_RET_CODE_MAX = 100,
}cm_curl_ret_code_e;
/**
 * @}
 */

/** @defgroup httpclient_struct Struct
  * @{
  */

typedef struct
{
#ifdef CM_OPENCPU_SUPPORT
    uint8_t ssl_enable;             /*!< 是否使用HTTPS */
    int32_t ssl_id;                 /*!< ssl 索引号 */
    uint8_t cid;                    /*!< PDP索引 */
    uint8_t conn_timeout;           /*!< 连接超时时间 */
    uint8_t rsp_timeout;            /*!< 响应超时时间 */
    uint8_t dns_priority;           /*!< dns解析优先级 0：使用全局优先级。1：v4优先。2：v6优先 */
#else
#ifdef HTTPCLIENT_SSL_ENABLE
    uint8_t ssl_enable;             //是否使用HTTPS
    int32_t ssl_id;                 //ssl 索引号
#endif
    uint8_t cid;                    //PDP索引
    uint8_t conn_timeout;           //连接超时时间
    uint8_t rsp_timeout;            //响应超时时间
#endif
}cm_curl_httpclient_cfg_t;

typedef struct 
{
	CURL *handle;
	uint8_t is_connected;
	http_type_e http_type;
	struct curl_slist *headers;
	void *userdata;
	char *response_buf;
	size_t response_len;
	httpclient_state_e state;
	cm_curl_httpclient_cfg_t curl_client_cfg;
} cm_curl_http_client_t;


/**HTTP回调*/
/*
* @param [in] client_handle                         客户端句柄
* @param [in] event                                 回调消息事件
* @param [in] param                                 事件相关对应响应数据
*
* CM_HTTP_CALLBACK_EVENT_REQ_START_SUCC_IND         NULL
* CM_HTTP_CALLBACK_EVENT_RSP_HEADER_IND             cm_httpclient_callback_rsp_header_param_t
* CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND            cm_httpclient_callback_rsp_content_param_t
* CM_HTTP_CALLBACK_EVENT_RSP_END_IND                NULL
* CM_HTTP_CALLBACK_EVENT_ERROR_IND                  cm_httpclient_error_event_e
*/
typedef void (*cm_httpclient_event_callback_func)(cm_curl_http_client_t client_handle, cm_httpclient_callback_event_e event, void *param);

/**HTTP相关回调函数*/
typedef struct 
{
	uint16_t response_code;
	uint16_t response_header_len;
	const uint8_t *response_header;
} cm_httpclient_callback_rsp_header_param_t;

/**HTTP相关回调函数*/
typedef struct rsp_st
{
    uint32_t total_len;
    uint32_t sum_len;
    uint32_t current_len;
    const uint8_t *response_content;
} cm_httpclient_callback_rsp_content_param_t;



#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/*
 * @brief ASR平台专用，设置ATHANDLE 以区分不同SIM卡槽的 cellid 和lac
 *
 * @param [in] atHandle   at句柄
 */
//void cm_lbs_asr_set_athandle(UInt32 atHandle);

/*
*@brief 获取默认cid(必选)
*
*@return cid
*/
int __cm_lbs_get_cid(void);

/*
*@brief 获取网络状态(必选)
*
*@param arg 预留
*@return CM_LBS_OK：网络正常 LBS_ERROR：网络异常
*
*/
cm_lbs_state_e __cm_lbs_get_netstate(void *arg);

/*
*@brief 获取IMEI(必选)
*
*@param[in] imei 读取到的IMEI
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_GetImei(uint8_t *imei);

#if 0 //暂时未实现,实现后打开 25/02/18
/*
*@brief 获取IMSI(必选)
*
*@param[in] imsi 读取到的IMSI
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
// cm_lbs_state_e __cm_lbs_GetImsi(uint8_t *imsi);
#endif

/*
*@brief 获取mcc(必选)
*
*@param[in] mcc 读取到的mcc
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_GetCell_info(cm_service_cell_info_t *cell_info);

/*
*@brief 获取网络类型（可选项，平台不支持时，直接返回LBS_ERROR即可）
*
*@param[in] network 获取到的网络类型
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetNetwork(uint8_t *network);

/*
*@brief 获取基站新鲜度（使用高德API2.0 的必选项）
*
*@param[in] cage 获取到基站新鲜度
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetCage(uint32_t *cage);

/*
*@brief 获取网关IP（可选项，平台不支持时，直接返回LBS_ERROR即可）
*
*@param[in] serverip 获取到的网关IP
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetServerip(uint8_t *serverip);

/*
*@brief 获取邻区基站信息（可选项，平台不支持时，直接返回LBS_ERROR即可）
*
*@param[in] nearbts 获取到邻区基站信息
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*
*/
cm_lbs_state_e __cm_lbs_GetNearbts(uint8_t *nearbts);

/**
 * @brief 计算MD5函数(必选)
 *
 * @param [in] input   需要计算的值
 * @param [in] output  计算后MD5值
 */
void cm_lbs_md5(char *input,char *output);

/*
 * @brief 延时函数(必选)
 *
 * @param [in] time_ms     延时时间，单位5ms
 */
void cm_lbs_delay(uint32_t time_ms);

/*
 * @brief URC上报函数(必选)
 *
 * @param [in] arg   urc函数参数
 * @param [in] urc_buf URC上报的字符串
 */
void cm_lbs_urc_rsp(void *arg,char *urc_buf);


/*
 *@bief 底层截取异频邻区信息(asr平台用)
 *
 * @param [in] nearbts 邻区信息
 *
*/
void cm_lbs_GetNEarbts_Capture_teInterFreqInfoInd(char *nearbts);


/*
 *@bief 底层截取同频邻区信息（asr平台用）
 *
 * @param [in] nearbts 邻区信息
 *
*/
void cm_lbs_GetNEarbts_Capture_teIntraFreqInfoInd(char *nearbts);

#if 1
/*
 *@bief 写配置到NV
 *
 * @param [in] cfg 配置信息
 * @param [in] size 需要写入的大小
 *
*/
cm_lbs_state_e cm_lbs_writenv_config(void *cfg, int size);

/*
 *@bief 读取NV内的配置
 *
 * @param [in] cfg 配置信息
 * @param [in] size 需要读取的大小
 *
*/
cm_lbs_state_e cm_lbs_readnv_config(void *cfg, int size);
#endif


#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif/*__CM_LBS_PLATFORM_H__*/
