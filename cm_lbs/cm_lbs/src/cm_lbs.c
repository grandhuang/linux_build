#include "cm_lbs.h"
#include "cm_lbs_platform.h"
#include "cm_lbs_amap.h"
#include "cm_lbs_oneospos.h"


#ifndef ONEOSPOS_PID_INTERNAL
	#define ONEOSPOS_PID_INTERNAL  "VuJCIgMPvN"
#endif

#define CM_LBS_OC_SUPPORT (0)              /*是否启用对OC的支持*/


#define CM_LBS_GETUEINFO_TIMEOUT (5000)    /*获取UE链接信息超时时间*/
#define AMAPKEY_FILTER_COUNT (6)           /*需要过滤的高德key数量*/
#define AMAPKEY_FILTER_ENABLE (1)          /*过滤amap key使能*/

typedef void* LbsTaskHandle;
typedef void* LbsQueueHandle;
typedef pthread_mutex_t* LbsMutexHandle;
typedef uv_timer_t* LbsTimerHandle;
/** LBS消息队列数据缓存大小 */
#define CM_LBS_MESSAGE_QUEUE_BUFLEN  64

/**LBS线程句柄(仅用于判断线程是否创建)*/
typedef void *cm_lbs_thread_id_t;

/**LBS线程处理函数*/
typedef void (*cm_lbs_thread_func_t)(void *param);

/**LBS消息队列句柄*/
typedef void *cm_lbs_message_queue_t;

/*LBS任务事件*/
typedef enum
{
    CM_LBS_LOCATION = 1,
    CM_LBS_GET_UEINFO

} cm_lbs_event_e;


/*LBS任务消息队列*/
typedef struct
{
    cm_lbs_event_e event;
} cm_lbs_msg_t;


/*LBS上一层的回调信息*/
typedef struct
{
    cm_lbs_callback cb;
    void *cb_arg;
} cm_lbs_callback_msg_t;


/*LBS主任务句柄*/
static LbsTaskHandle cm_lbs_task = NULL;

/*LBS主任务消息队列*/
static LbsQueueHandle cm_lbs_queun = NULL;

/*初始化的定位平台*/
static int s_platform = 0;

/*需要屏蔽的高德key*/
static char *s_amap_key_filter[AMAPKEY_FILTER_COUNT] =
{
    "17c91c31e704f1306d3b870d2cef76ca",
    "72ce730145e10ae5b04d307a442162ba",
    "624d3d5bb15c40adba27c14310f85b23",
    "eb5c394e6f300b26c86bf7594aad5fbb",
    "f81a5036d2320176e6fbeac959792571",
    "a815405b46390e87193110fdb70ac017"
};


cm_lbs_state_e cm_lbs_write_nv_data(cm_lbs_nvconfig_t *nvcfg);
/************************************************OC function *****************************************/

/*****************************************************************************************************/

#if CM_LBS_OC_SUPPORT
/*amap 配置缓存 用于OC获取配置信息接口*/
static char amap_attr_apikey[CM_LBS_AMAP_APIKET_LEN] = {0};
static char amap_attr_digital_key[CM_LBS_AMAP_SIGNATUREKEY_LEN] = {0};
static cm_lbs_amap_location_attr_t s_amap_attr_cache = {0};
/*oneospos 配置缓存 用于OC获取配置信息接口*/
static char oneospos_pid[CM_LBS_ONEOSPOS_PID_LEN] = {0};
static cm_lbs_oneospos_attr_t s_oneospos_attr_cache = {0};

#endif /*CM_LBS_OC_SUPPORT*/

/*LBS消息队列适配器*/
typedef struct
{
    int msgid;								/*消息ID，用于唯一标识队列中的消息*/
    uint32_t msg_len;						/*消息长度，表示该消息内容的字节大小*/
} cm_lbs_message_queue_adapter;

/*LBS消息队列消息结构体*/
typedef struct
{
    long type; 								/*消息类型，一个大于0的数值，用于区分不同的消息内*/
    char mtext[CM_LBS_MESSAGE_QUEUE_BUFLEN];	/*消息文本，存储具体的消息内容，长度由CM_MESSAGE_QUEUE_BUFLEN定义*/
} cm_lbs_messagequeue_msgstru_t;

/**
 * @brief 创建lbs线程处理函数（屏蔽与cmsis_os接口差异）
 *
 * @param [in] stack_size       栈大小，无效
 * @param [in] priority         优先级,无效
 * @param [in] func             线程处理函数
 *
 * @return 线程句柄,无效值（非NULL表示成功）
 *
 * @details 适配平台线程创建接口
 */
static void *cm_lbs_thread_adapter_handle(void *arg)
{
    cm_lbs_thread_func_t func = (cm_lbs_thread_func_t)arg;

    //执行用户线程函数
    func(NULL);

    pthread_exit(NULL);
}

/*
*@brief 创建任务(必选)
*
*@param[in] func 任务执行函数
*@param[in] arg  预留
*@return osThreadId_t 任务句柄
*
*/
LbsTaskHandle __cm_lbs_CreateTask(cm_lbs_thread_func_t func, void *arg)
{
    (void)arg;
	pthread_attr_t attr;
    pthread_t task_id = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int result = pthread_create(&task_id, &attr, cm_lbs_thread_adapter_handle, func);

    if (result != 0)
    {
        CMLBS_LOGI("Failed to create thread, errno: %d", errno);
        return NULL;
    }

    return (cm_lbs_thread_id_t)10086;  //为了兼容NB模组软件版本返回指针，实际上该值无效，仅用于判断线程已被创建
}

/*
*@brief 删除任务(可选)
*
*@param[in] task_handle 任务句柄
*
*@return CM_LBS_OK：成功    其他：失败
*
*/
cm_lbs_state_e __cm_lbs_DeleteTask(LbsTaskHandle task_handle)
{
    task_handle = NULL;
    pthread_exit(NULL);
    return CM_LBS_OK;
}

/*
*@brief 创建消息队列(必选)
*
*@param[in] msg_count 消息队列数据数量
*@param[in] msg_size 队列单个数据长度
*@param[in] arg  预留
*@return  LbsQueueHandle 消息队列句柄
*
*/
LbsQueueHandle __cm_lbs_CreateQuen(uint32_t msg_count, uint32_t msg_size, void* arg)
{
    (void)arg;
    cm_lbs_message_queue_adapter *message_adapter = cm_lbs_malloc(sizeof(cm_lbs_message_queue_adapter));

    if(!message_adapter)
    {
        CMLBS_LOGE("struct malloc is null");
        return NULL;
    }

    if(msg_size > CM_LBS_MESSAGE_QUEUE_BUFLEN)  //发送的单个消息不可大于msg结构体数据缓存大小
    {
        CMLBS_LOGE("message len outof limit");
        cm_lbs_free(message_adapter);
        return NULL;
    }

    message_adapter->msgid = msgget(IPC_PRIVATE, IPC_EXCL | IPC_CREAT | 0777);

    if (message_adapter->msgid == -1)
    {
        CMLBS_LOGE("message id error");
        cm_lbs_free(message_adapter);
        return NULL;
    }

    message_adapter->msg_len = msg_size;
    return message_adapter;

    //return cm_messagequeue_create(msg_count, msg_size);
}


/*
*@brief 消息队列发送数据(必选)
*
*@param[in] queue_handle 消息队列句柄
*@param[in] msg 需要发送的消息
*@param[in] arg  预留
*
*@return  成功：CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_QueueSend(LbsQueueHandle queue_handle, const void *msg)
{
    if(NULL == queue_handle)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_OK;
	cm_lbs_message_queue_adapter *message_adapter = (cm_lbs_message_queue_adapter *)queue_handle;

    if (message_adapter == NULL)
    {
        CMLBS_LOGE("message id is null");
        return -1;
    }

    int msgid = message_adapter->msgid;
    int msg_len = message_adapter->msg_len;

    if(msg_len > CM_LBS_MESSAGE_QUEUE_BUFLEN)  //发送的单个消息不可大于msg结构体数据缓存大小
    {
        CMLBS_LOGE("message len outof limit");
        return -1;
    }

    cm_lbs_messagequeue_msgstru_t msgstru = {0};

    msgstru.type = 1;
    memcpy(msgstru.mtext, (void*)msg, msg_len);

    if(msgsnd(msgid, &msgstru, msg_len, IPC_NOWAIT) == -1)  //非阻塞方式发送
    {
        CMLBS_LOGI("msgsnd error :%d", errno);
        ret =  CM_LBS_ERR;
    }
    else
    {
        ret =  CM_LBS_OK;
    }
    // cm_message_queue_t mq_id = queue_handle;
    // if(0 == cm_messagequeue_send(mq_id, (void*)(msg)))
    // {
    //     ret = CM_LBS_OK;
    // }
    return ret;
}

/*
*@brief 消息队列接收数据(必选)
*
*@param[in] queue_handle 消息队列句柄
*@param[in] msg 需要接收的消息
*@param[in] time_out  超时时间
*
*@return  成功：CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_QueueRev(LbsQueueHandle queue_handle, void *msg, uint32_t time_out)
{
    cm_lbs_state_e ret = CM_LBS_OK;
    cm_lbs_message_queue_adapter *message_adapter = (cm_lbs_message_queue_adapter *)queue_handle;

    if (message_adapter == NULL)
    {
        CMLBS_LOGE("id is null");
        ret =  CM_LBS_ERR;
    }

    int msgid = message_adapter->msgid;
    int msg_len = message_adapter->msg_len;

    if(msg_len > CM_LBS_MESSAGE_QUEUE_BUFLEN)  //发送的单个消息不可大于msg结构体数据缓存大小
    {
        CMLBS_LOGE("message len outof limit");
        ret =  CM_LBS_ERR;
    }

    cm_lbs_messagequeue_msgstru_t msgstru = {0};

    if(msgrcv(msgid, &msgstru, msg_len, 1, 0) == -1)
    {
        CMLBS_LOGE("msgrcv error");
        ret =  CM_LBS_ERR;
    }

    //获取数据
    memcpy(msg, msgstru.mtext, msg_len);

    //if(0 == cm_messagequeue_wait(mq_id, (void*)(msg)))
    //{
    //    ret = CM_LBS_OK;
    //}
    return ret;
}

/*
*@brief 删除消息队列(必选)
*
*@param[in] queue_handle 消息队列句柄
*
*@return  成功：CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_QueueDelete(LbsQueueHandle queue_handle)
{
    cm_lbs_state_e ret = CM_LBS_OK;
	cm_lbs_message_queue_adapter *message_adapter = (cm_lbs_message_queue_adapter *)queue_handle;

    if (message_adapter == NULL)
    {
        ret =  CM_LBS_ERR;
    }

    int msgid = message_adapter->msgid;

    if(msgctl(msgid, IPC_RMID, NULL) != 0)
    {
        ret =  CM_LBS_ERR;
    }

    cm_lbs_free(message_adapter);
    //if(0 == cm_messagequeue_delete(mq_id))
    //{
    //    ret = CM_LBS_OK;
    //}
    return ret;
}

/*
*@brief 创建定时器(必选)
*
*@param[in] cb 定时器回调函数
*
*@return  成功：定时器句柄 失败：NULL
*/
LbsTimerHandle __cm_lbs_TimeCreat(void *cb)
{
    return NULL;
}

/*
*@brief 启动定时器(必选)
*
*@param[in] timer_id 定时器句柄
**@param[in] timer_id 定时时间sec
*
*@return  CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_TimeStar(LbsTimerHandle timer_id, uint32_t time_s)
{
#if 0 
    cm_lbs_state_e ret = CM_LBS_ERR;
    if(0 >= time_s)
    {
        CMLBS_LOGI("time is fail! %d", time_s);
        return ret;
    }
    uint32_t ticks = (time_s * 1000) / (1000 / osKernelGetTickFreq());
    if(0 == osTimerStart(timer_id, ticks))
    {
        ret = CM_LBS_OK;
    }
    return ret;
#endif
    return 0;
}

/*
*@brief 获取定时器状态(必选)
*
*@param[in] timer_id 定时器句柄
*
*@return  CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_TimeState(LbsTimerHandle timer_id)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
#if 0
    if(true == osTimerIsRunning(timer_id))
    {
        ret = CM_LBS_OK;
    }
#endif
    return ret;
}

/*
*@brief 停止定时器(必选)
*
*@param[in] timer_id 定时器句柄
*
*@return  CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_TimeStop(LbsTimerHandle timer_id)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
#if 0
    if(0 == osTimerStop(timer_id))
    {
        ret = CM_LBS_OK;
    }
#endif
    return ret;
}

/*
*@brief 删除定时器(必选)
*
*@param[in] timer_id 定时器句柄
*
*@return  CM_LBS_OK 失败：其他（详见cm_lbs_state_e）
*/
cm_lbs_state_e __cm_lbs_TimeDelete(LbsTimerHandle timer_id)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
#if 0
    if(0 == osTimerDelete(timer_id))
    {
        timer_id = NULL;
        ret = CM_LBS_OK;
    }
#endif
    return ret;
}


/*
*@brief 创建互斥锁(必选)
*
*@param[in] arg 预留
*
*@return  LbsMutexHandle 互斥锁id
*/
LbsMutexHandle __cm_lbs_MutexCreate(void *arg)
{
    (void)arg;
    LbsMutexHandle mutex = (LbsMutexHandle)cm_lbs_malloc(sizeof(LbsMutexHandle));
    (void)pthread_mutex_init(mutex, NULL);
    return mutex;
}

/*
*@brief 加锁(必选)
*
*@param[in] mutexid 互斥锁id
*@param[in] timeout 超时时间
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_MutexAcquire(LbsMutexHandle mutexid, uint32_t timeout)
{
    if(NULL == mutexid)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_ERR;
    struct timespec tout;
    tout.tv_sec = timeout;
    if(0 == pthread_mutex_timedlock(mutexid, &tout))
    {
        ret = CM_LBS_OK;
    }
    return ret;
}

/*
*@brief 释放锁(必选)
*
*@param[in] mutexid 互斥锁id
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_MutexRelease(LbsMutexHandle mutexid)
{
    if(NULL == mutexid)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_ERR;
    if(0 == pthread_mutex_unlock(mutexid))
    {
        ret = CM_LBS_OK;
    }
    return ret;
}

/*
*@brief 删除锁(必选)
*
*@param[in] mutexid 互斥锁id
*
*@return 成功:CM_LBS_OK 失败：LBS_ERROR
*/
cm_lbs_state_e __cm_lbs_MutexDelete(LbsMutexHandle mutexid)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
    if(0 == pthread_mutex_destroy(mutexid))
    {
        ret = CM_LBS_OK;
    }
    cm_lbs_free(mutexid);
    return ret;
}

static void __cm_lbs_amap_backup_attr(cm_lbs_amap_location_attr_t *attr)
{
#if CM_LBS_OC_SUPPORT
    s_amap_attr_cache.digital_sign_enable = attr->digital_sign_enable;
    s_amap_attr_cache.nearbts_enable = attr->nearbts_enable;
    s_amap_attr_cache.show_fields_enable = attr->show_fields_enable;
    s_amap_attr_cache.time_out = attr->time_out;

    if(NULL != attr->api_key)
    {
        if(strlen((char *)attr->api_key) < 1)
        {
            s_amap_attr_cache.api_key = NULL;
        }
        else
        {
            int apikeylen = strlen((char *)attr->api_key);
            memset(amap_attr_apikey, '\0', CM_LBS_AMAP_APIKET_LEN);
            memcpy(amap_attr_apikey, (char *)attr->api_key, apikeylen);
            s_amap_attr_cache.api_key = (uint8_t *)amap_attr_apikey;
        }
    }
    else
    {
        s_amap_attr_cache.api_key = NULL;
    }

    if(NULL != attr->digital_key)
    {
        if(strlen((char *)attr->digital_key) < 1)
        {
            s_amap_attr_cache.digital_key = NULL;
        }
        else
        {
            int digital_keylen = strlen((char *)attr->digital_key);
            memset(amap_attr_digital_key, '\0', CM_LBS_AMAP_SIGNATUREKEY_LEN);
            memcpy(amap_attr_digital_key, (char *)attr->digital_key, digital_keylen);
            s_amap_attr_cache.digital_key = (uint8_t *)amap_attr_digital_key;
        }
    }
    else
    {
        s_amap_attr_cache.digital_key = NULL;
    }
 #endif /*CM_LBS_OC_SUPPORT*/
}


static void __cm_lbs_oneospos_backup_attr(cm_lbs_oneospos_attr_t *attr)
{
#if CM_LBS_OC_SUPPORT
    s_oneospos_attr_cache.nearbts_enable = attr->nearbts_enable;
    s_oneospos_attr_cache.time_out = attr->time_out;
    if(NULL != attr)
    {
        if(strlen((char *)attr->pid) < 1)
        {
            s_oneospos_attr_cache.pid = NULL;
        }
        else
        {
            int pidlen = strlen((char *)attr->pid);
            memset(oneospos_pid, '\0', CM_LBS_ONEOSPOS_PID_LEN);
            memcpy(oneospos_pid, (char *)attr->pid, pidlen);
            s_oneospos_attr_cache.pid=oneospos_pid;
        }
    }
    else
    {
        s_oneospos_attr_cache.pid = NULL;
    }
#endif /*CM_LBS_OC_SUPPORT*/
}


/******************************************************************************************************************************
*amap function
*******************************************************************************************************************************/
/*判断输入的key 是否是需要屏蔽*/
static cm_lbs_state_e __cm_amap_key_filter(char *key)
{
#if AMAPKEY_FILTER_ENABLE
    int i = 0;
    for(i = 0; i < AMAPKEY_FILTER_COUNT; i++)
    {
        if(0 == strcmp(key, s_amap_key_filter[i]))
        {
            return CM_LBS_ERR;
        }
    }
#endif /*AMAPKEY_FILTER_ENABLE*/
    return CM_LBS_OK;
}

/*LBS AMAP句柄*/
static cm_amap_t *s_amap_attr = NULL;

/*
*@brief 高德定位平台的回调函数
*
*@param[in] rsp_event 回调事件
*@param[in] location_rsp 定位结果，定位失败时为空
*@param[in] cb_arg 回调函数参数，此处
*
*@return CM_LBS_OK：成功 其他：失败
*/
static void __cm_lbs_amap_callback(cm_lbs_amap_event_e rsp_event, cm_lbs_amap_location_rsp_t *location_rsp, void *arg)
{
    CMLBS_LOGI("rsp_event:%d", rsp_event);

    cm_lbs_callback_msg_t *cbmsg = (cm_lbs_callback_msg_t *)arg;
    static cm_lbs_location_rsp_t *lbs_location_rsp = NULL;
    cm_lbs_callback_event_e ret = CM_LBS_LOCATION_OK;
    switch(rsp_event)
    {
        case  AMAP_LOCATION_SUCCEED:
            {
                lbs_location_rsp = (cm_lbs_location_rsp_t *)cm_lbs_malloc(sizeof(cm_lbs_location_rsp_t));
                if(NULL == lbs_location_rsp)
                {
                    rsp_event = AMAP_LOCATION_UNKNOWN_ERR;
                    break;
                }

                if(NULL != location_rsp)
                {
                    lbs_location_rsp->adcode = location_rsp->adcode;
                    lbs_location_rsp->city = location_rsp->city ;
                    lbs_location_rsp->citycode = location_rsp->citycode;
                    lbs_location_rsp->country = location_rsp->country;
                    lbs_location_rsp->latitude = location_rsp->latitude;
                    lbs_location_rsp->location_describe = location_rsp->location_describe;
                    lbs_location_rsp->longitude = location_rsp->longitude;
                    lbs_location_rsp->poi = location_rsp->poi;
                    lbs_location_rsp->province = location_rsp->province;
                    lbs_location_rsp->radius = location_rsp->radius;
                }
                ret = CM_LBS_LOCATION_OK;
                break;
            }
        case  AMAP_LOCATION_FAIL :
            {
                ret = CM_LBS_UNKNOW_ERR  ;
                break;
            }
        case  AMAP_LOCATION_TIMEOUT:
            {
                ret = CM_LBS_TIMEOUT ;
                break;
            }
        case  AMAP_LOCATION_CONNECTFAILL :
            {
                ret = CM_LBS_NET_ERR ;
                break;
            }
        case AMAP_LOCATION_KEY_INVALID:
            {
                ret = CM_LBS_KEY_ERR ;
                break;
            }
        case  AMAP_LOCATION_OVER_DAYQUOTA :
            {
                ret = CM_LBS_OVER_DAYQUTA ;
                break;
            }
        case  AMAP_LOCATION_OVER_ALLQUOTA :
            {
                ret = CM_LBS_OVER_QUOTA ;
                break;
            }
        case  AMAP_LOCATION_OVER_QPS :
            {
                ret = CM_LBS_OVER_QPS ;
                break;
            }
        case  AMAP_LOCATION_UNKNOWN_ERR :
            {
                ret = CM_LBS_UNKNOW_ERR ;
                break;
            }
        default:
            ret = CM_LBS_UNKNOW_ERR ;
            break;
    }


    if(NULL != lbs_location_rsp)
    {
        lbs_location_rsp->state = ret;
    }

    /*直接上报给上一层处理*/
    cbmsg->cb(ret, lbs_location_rsp, cbmsg->cb_arg);

    /*释放返回结果*/
    free(lbs_location_rsp);
    lbs_location_rsp = NULL;

}

static cm_lbs_state_e __cm_lbs_creare_cm_amap_ueinfo(cm_amap_ue_info_t **ueinfo, int nearbts_enable)
{
    return  cm_amap_get_ueinfo(ueinfo, nearbts_enable);
}

static void __cm_lbs_copy_amap_authentication_by_attr(int platform, cm_lbs_amap_location_attr_t *attr, cm_amap_authentication_t *auth)
{
    auth->platform = platform;
    auth->time_out = attr->time_out;
    auth->show_fields_enable = attr->show_fields_enable;
    auth->digital_sign_enable = attr->digital_sign_enable;

    /*拷贝apikey*/
    if(NULL != attr->api_key)
    {
        int apikey_len = strlen((char *)attr->api_key);
        uint8_t *apikey = (uint8_t *)cm_lbs_malloc((apikey_len + 1) * sizeof(uint8_t));
        if(NULL != apikey)
        {
            strncpy((char *)apikey, (char *) attr->api_key, apikey_len);
            auth->apikey = (char *)apikey;
        }

    }

    /*拷贝数字签名*/
    if(NULL != attr->digital_key)
    {
        int digital_key_len = strlen((char *)attr->api_key);
        uint8_t *digital_key = (uint8_t *)cm_lbs_malloc((digital_key_len + 1) * sizeof(uint8_t));
        if(NULL != digital_key)
        {
            strncpy((char *)digital_key, (char *)attr->digital_key, digital_key_len);
            auth->digital_sign_key = (char *)digital_key;
        }
    }

}

static void __cm_lbs_free_authentication(cm_amap_authentication_t *auth)
{
    if(NULL == auth)
    {
        return ;
    }

    if(NULL != auth->apikey)
    {
        free(auth->apikey);
        auth->apikey = NULL;
    }

    if(NULL != auth->digital_sign_key)
    {
        free(auth->digital_sign_key);
        auth->digital_sign_key = NULL;
    }

    free(auth);
    auth = NULL;
}

static void __cm_lbs_free_cm_amap_ueinfo(cm_amap_ue_info_t *ueinfo)
{
    cm_amap_free_ueinfo(ueinfo);
}

static void __cm_lbs_s_amap_attr_free(void)
{
    if(NULL != s_amap_attr)
    {
        /*释放鉴权信息*/
        __cm_lbs_free_authentication(s_amap_attr->auth);

        /*释放UEINFO*/
        __cm_lbs_free_cm_amap_ueinfo(s_amap_attr->ueinfo);

        free(s_amap_attr);
        s_amap_attr = NULL;
    }

}

static int32_t cm_lbs_init_amap(cm_lbs_location_platform_e platform, void *attr)
{
    
    // CMLBS_LOG("%s",__FUNCTION__);    
    if(NULL != s_amap_attr)
    {
        return -2;
    }
    if(NULL == attr)
    {
        return -1;
    }
    cm_lbs_amap_location_attr_t *lbs_attr = (cm_lbs_amap_location_attr_t *)attr;

    cm_amap_t *amap_attr = (cm_amap_t *)cm_lbs_malloc(sizeof(cm_amap_t));
    if(NULL == amap_attr)
    {
        return -2;
    }

    cm_amap_authentication_t *amap_auth = (cm_amap_authentication_t*)cm_lbs_malloc(sizeof(cm_amap_authentication_t));
    if(NULL == amap_auth)
    {
        free(amap_attr);
        return -2;
    }

    /*备份atrr 供oc查询使用*/
    __cm_lbs_amap_backup_attr(lbs_attr);

    __cm_lbs_copy_amap_authentication_by_attr(platform, lbs_attr, amap_auth);

    amap_attr->auth = amap_auth;
    amap_attr->cb = __cm_lbs_amap_callback;
    amap_attr->nearbts_enable = lbs_attr->nearbts_enable;
    s_amap_attr = amap_attr;
    return 0;
}

static int32_t cm_lbs_location_amap(cm_lbs_callback cb, void *cb_arg)
{
    if(NULL == s_amap_attr)
    {
        return -1;
    }

    if(CM_LBS_OK != __cm_lbs_get_netstate(NULL))
    {
        __cm_lbs_s_amap_attr_free();
        return -2;
    }

    static cm_lbs_callback_msg_t lbs_callback_msg = {0};
    lbs_callback_msg.cb = cb;
    lbs_callback_msg.cb_arg = cb_arg;

    s_amap_attr->cb_arg = (void *)&lbs_callback_msg;
    cm_lbs_msg_t msg = {0};
    msg.event = CM_LBS_LOCATION;

    cm_lbs_state_e ret =  __cm_lbs_QueueSend(cm_lbs_queun, (void *)&msg);
    if(CM_LBS_OK != ret)
    {
        __cm_lbs_s_amap_attr_free();
        return -3;
    }
    return 0;
}


static void cm_lbs_asyn_location_amap(void)
{

    cm_amap_ue_info_t *ue_info = NULL;
    cm_lbs_state_e ret = __cm_lbs_creare_cm_amap_ueinfo(&ue_info, s_amap_attr->nearbts_enable);
    
    if(NULL == ue_info || ret != CM_LBS_OK)
    {
        __cm_lbs_amap_callback(AMAP_LOCATION_GET_UEINFO_FAIL, NULL, s_amap_attr->cb_arg);
        cm_amap_free_ueinfo(ue_info);
        return ;
    }

    if(CM_LBS_OK != __cm_amap_key_filter(s_amap_attr->auth->apikey))
    {
        __cm_lbs_amap_callback(AMAP_LOCATION_APIKEY_FOBIDEN, NULL, s_amap_attr->cb_arg);
        cm_amap_free_ueinfo(ue_info);
        return ;
    }
    
    s_amap_attr->ueinfo = ue_info;

    ret = cm_amap_location(s_amap_attr);
    if(CM_LBS_OK != ret)
    {
        CMLBS_LOGI("amap_location fail :%d", ret);
        __cm_lbs_amap_callback(AMAP_LOCATION_UNKNOWN_ERR, NULL, s_amap_attr->cb_arg);
        cm_amap_free_ueinfo(ue_info);
        return ;
    }
}


static void __cm_oneospos_pid_config(char* input_pid, char *outpid)
{
    cm_lbs_state_e ret = CM_LBS_OK;
    if(NULL == input_pid || strlen(input_pid) < 1 || 0 == strcmp(input_pid, "0"))
    {
        cm_lbs_nvconfig_t *nvconfig = (cm_lbs_nvconfig_t*)cm_lbs_malloc(sizeof(cm_lbs_nvconfig_t));
        do
        {
            if(NULL==nvconfig)
            {
               return ;
            }
            
            if(CM_LBS_OK != cm_lbs_read_nv_data(nvconfig))
            {
                ret = CM_LBS_ERR;
                break;
            }

            if(strlen(nvconfig->oneospospid) < 1 || 0 == strcmp(nvconfig->oneospospid, "0"))
            {
                ret = CM_LBS_ERR;
                break;
            }

            memcpy(outpid, nvconfig->oneospospid, strlen(nvconfig->oneospospid));
            CMLBS_LOG("use nv pid");

            
        } while(0);
        free(nvconfig);


        if(CM_LBS_ERR == ret)
        {
            memcpy(outpid, ONEOSPOS_PID_INTERNAL, strlen(ONEOSPOS_PID_INTERNAL));
        }
        
    }

    else
    {
        memcpy(outpid, input_pid, strlen(input_pid));
    }
}

/***************************************************************************************************************************
**   oneospos function & data
***************************************************************************************************************************/
static cm_oneospos_attr_t *s_oneospos_attr_t = NULL;

static void __cm_lbs_s_oneospos_attr_free(void)
{
    if(NULL != s_oneospos_attr_t)
    {
        cm_oneospos_free_ueinfo(s_oneospos_attr_t->ueinfo);
        if(NULL != s_oneospos_attr_t->pid)
        {
            free(s_oneospos_attr_t->pid);
            s_oneospos_attr_t->pid = NULL;
        }
        free(s_oneospos_attr_t);
        s_oneospos_attr_t = NULL;
    }
}

static void  cm_lbs_oneospos_callback(cm_oneospos_callback_event_e event, cm_oneospos_location_rsp_t *rsp, void *cb_arg)
{
    cm_lbs_callback_msg_t *app_callback = (cm_lbs_callback_msg_t *)cb_arg;
    cm_lbs_location_rsp_t *lbs_location_rsp = NULL;
    cm_lbs_callback_event_e ret = CM_LBS_UNKNOW_ERR ;
	CMLBS_LOGI("cm_lbs_oneospos_cb event:%d %d", event, (int)app_callback->cb);

    switch(event)
    {
        case CM_ONEOSPOS_EVENT_SUCCESS:
            {
                lbs_location_rsp = (cm_lbs_location_rsp_t *)cm_lbs_malloc(sizeof(cm_lbs_location_rsp_t));
                if(NULL == lbs_location_rsp)
                {
                    ret = CM_LBS_UNKNOW_ERR;
                    break;
                }
                lbs_location_rsp->latitude = rsp->latitude;
                lbs_location_rsp->longitude = rsp->longitude;
                ret = CM_LBS_LOCATION_OK;
                break;
            }
        case CM_ONEOSPOS_EVENT_FAIL:
            {
                ret = CM_LBS_UNKNOW_ERR;
                break;
            }
        case CM_ONEOSPOS_EVENT_OVER_RESTRICT:
            {
                ret = CM_LBS_OVER_QPS ;
                break;
            }

        case CM_ONEOSPOS_EVENT_UNSET_PLATFORM :
            {
                ret = CM_LBS_UNKNOW_ERR;
                break;
            }
        default:
            break;
    }

    if(NULL != app_callback->cb)
    {
        app_callback->cb(ret, lbs_location_rsp, app_callback->cb_arg);
    }

    free(lbs_location_rsp);

    __cm_lbs_s_oneospos_attr_free();
}

/*初始化 oneospos 配置*/
static int32_t cm_lbs_init_oneospos(cm_lbs_location_platform_e platform, void *attr)
{
    if(NULL == attr)
    {
        return -1;
    }

    if(NULL != s_oneospos_attr_t)
    {
        return -2;
    }


    cm_lbs_oneospos_attr_t *inattr = (cm_lbs_oneospos_attr_t *)attr;
    cm_oneospos_attr_t input_attr = {0};
    input_attr.nearbts_enable = inattr->nearbts_enable;
    input_attr.pid = inattr->pid;

    /*备份attr 供oc查询使用*/
    __cm_lbs_oneospos_backup_attr(inattr);

    char *oneos_pid=(char *)cm_lbs_malloc(CM_LBS_ONEOSPOS_PID_LEN*sizeof(char));
    if(NULL==oneos_pid)
    {
      return -2;
    }

    /*配置PID 若传入的pid为空 则优先使用NV内的pid 其次使用代码内置pid*/
    __cm_oneospos_pid_config(input_attr.pid, oneos_pid);
    input_attr.pid = oneos_pid;

    /*创建配置*/
    cm_oneospos_create_attr(&input_attr, &s_oneospos_attr_t);

    free(oneos_pid);

    if(NULL == s_oneospos_attr_t)
    {
        return -2;
    }

    /*使用lbs层 callback 先进行数据预处理*/
    s_oneospos_attr_t->cb = cm_lbs_oneospos_callback;


    return 0;
}

/*向LBS任务发送执行oneospos 定位的操作*/
static int32_t cm_lbs_location_oneospos(cm_lbs_callback cb, void *cb_arg)
{
    if(NULL == cb || NULL == s_oneospos_attr_t)
    {
        return -1;
    }

    if(CM_LBS_OK != __cm_lbs_get_netstate(NULL))
    {
        __cm_lbs_s_oneospos_attr_free();
        return -2;
    }

    /*注册上层回调*/
    static cm_lbs_callback_msg_t lbs_callback_msg = {0};
    lbs_callback_msg.cb = cb;
    lbs_callback_msg.cb_arg = cb_arg;
    s_oneospos_attr_t->cb_arg = (void *)&lbs_callback_msg;

    /****/
    cm_lbs_msg_t msg = {0};
    msg.event = CM_LBS_LOCATION;

    cm_lbs_state_e ret =  __cm_lbs_QueueSend(cm_lbs_queun, (void *)&msg);
    if(CM_LBS_OK != ret)
    {
        __cm_lbs_s_oneospos_attr_free();
        return -3;
    }
    return 0;
}


/*异步执行oneospos 定位*/
static void cm_lbs_asyn_location_oneospos(void)
{
    
    if(NULL == s_oneospos_attr_t)
    {
        cm_lbs_oneospos_callback(CM_ONEOSPOS_EVENT_ATTR_NULL, NULL, s_oneospos_attr_t->cb_arg);
        return ;
    }

    cm_oneospos_ueinfo_t *oneospos_ue_info = NULL;
    int ret = cm_oneospos_get_ueinfo(&oneospos_ue_info, s_oneospos_attr_t->nearbts_enable);
    if(CM_LBS_OK != ret)
    {
        cm_lbs_oneospos_callback(CM_ONEOSPOS_EVENT_UEINFO_FAIL, NULL, s_oneospos_attr_t->cb_arg);
        cm_oneospos_free_ueinfo(oneospos_ue_info);
        return ;
    }

    s_oneospos_attr_t->ueinfo = oneospos_ue_info;

    ret = cm_oneospos_location(s_oneospos_attr_t);
    if(CM_LBS_OK != ret)
    {
        CMLBS_LOGI("location fail :%d", ret);
        cm_lbs_oneospos_callback(CM_ONEOSPOS_EVENT_UNKNOW, NULL, s_oneospos_attr_t->cb_arg);
        cm_oneospos_free_ueinfo(oneospos_ue_info);
    }

}


/*********************************************************************************************************************************
*task function
**********************************************************************************************************************************/
static void __cm_lbs_asyn_location(void)
{
    if(CM_LBS_PLAT_AMAP10 == s_platform || CM_LBS_PLAT_AMAP20 == s_platform)
    {
        cm_lbs_asyn_location_amap();
    }
    if(CM_LBS_PLAT_ONEOSPOS == s_platform)
    {
        cm_lbs_asyn_location_oneospos();
    }
}

/*
*@brief LBS主任务
*/
static void cm_lbs_main_task(void *arg)
{
    (void)arg;
    while(1)
    {
        cm_lbs_msg_t msg = {0};
        __cm_lbs_QueueRev(cm_lbs_queun, (void *)&msg, osWaitForever);
        switch(msg.event)
        {
            case CM_LBS_LOCATION:
                {
                    __cm_lbs_asyn_location();
                    break;
                }
            case CM_LBS_GET_UEINFO:
                {
                    break;
                }
            default :
                break;
        }
    }
}

static cm_lbs_state_e cm_lbs_task_create(void)
{  
    if(NULL != cm_lbs_task)
    {
        return CM_LBS_OK;
    }

    cm_lbs_task = __cm_lbs_CreateTask(cm_lbs_main_task, NULL);
    if(NULL == cm_lbs_task)
    {
        return CM_LBS_ERR;
    }

    cm_lbs_queun = __cm_lbs_CreateQuen(1, sizeof(cm_lbs_msg_t), NULL);
    if(NULL == cm_lbs_queun)
    {
        return CM_LBS_ERR;
    }

    return CM_LBS_OK;
}

cm_lbs_state_e cm_lbs_write_nv_data(cm_lbs_nvconfig_t *nvcfg)
{
    if(NULL == nvcfg)
    {
        return CM_LBS_PARAMERR;
    }
    return cm_lbs_writenv_config((void *)nvcfg, sizeof(cm_lbs_nvconfig_t));
}


cm_lbs_state_e cm_lbs_read_nv_data(cm_lbs_nvconfig_t *nvcfg)
{
    if(NULL == nvcfg)
    {
        return CM_LBS_PARAMERR;
    }
    return cm_lbs_readnv_config((void *)nvcfg, sizeof(cm_lbs_nvconfig_t));
}


/************************************************************************************************
*public function
*************************************************************************************************/
void cm_lbs_deinit(void)
{
    __cm_lbs_s_amap_attr_free();

    __cm_lbs_s_oneospos_attr_free();
}

int32_t cm_lbs_init(cm_lbs_location_platform_e platform, void *attr)
{
    
    CMLBS_LOG("");    
    s_platform = platform;
    if(CM_LBS_OK != cm_lbs_task_create())
    {
        CMLBS_LOGE("LBS task create failed");    
        return -2;
    }

    if(CM_LBS_PLAT_AMAP10 == platform || CM_LBS_PLAT_AMAP20 == platform)
    {
        return cm_lbs_init_amap(platform, attr);
    }

    if(CM_LBS_PLAT_ONEOSPOS == platform)
    {
        return cm_lbs_init_oneospos(platform, attr);
    }

    return -1;
}

int32_t cm_lbs_location(cm_lbs_callback cb, void *cb_arg)
{

    if(CM_LBS_PLAT_AMAP10 == s_platform || CM_LBS_PLAT_AMAP20 == s_platform)
    {
        return  cm_lbs_location_amap(cb, cb_arg);
    }

    if(CM_LBS_PLAT_ONEOSPOS == s_platform)
    {
        return cm_lbs_location_oneospos(cb, cb_arg);
    }

    return -1;
}

int32_t cm_lbs_get_attr(int platform, void *attr)
{
    int ret=-1;
#if CM_LBS_OC_SUPPORT    
    if(NULL == attr)
    {
        CMLBS_LOG("input attr is null");
        return ret;
    }

    if(s_platform != platform)
    {
        CMLBS_LOG("platform uninit");
        return ret;
    }

    switch(platform)
    {
        case CM_LBS_PLAT_AMAP10:
            {
                cm_lbs_amap_location_attr_t *us_amap_attr = (cm_lbs_amap_location_attr_t *)attr;
                memcpy(us_amap_attr, &s_amap_attr_cache, sizeof(cm_lbs_amap_location_attr_t));
                us_amap_attr->api_key = s_amap_attr_cache.api_key;
                us_amap_attr->digital_key = s_amap_attr_cache.digital_key;
                break;
            }
        case CM_LBS_PLAT_AMAP20:
            {
                cm_lbs_amap_location_attr_t *us_amap_attr = (cm_lbs_amap_location_attr_t *)attr;
                memcpy(us_amap_attr, &s_amap_attr_cache, sizeof(cm_lbs_amap_location_attr_t));
                us_amap_attr->api_key = s_amap_attr_cache.api_key;
                us_amap_attr->digital_key = s_amap_attr_cache.digital_key;
                break;
            }

        case CM_LBS_PLAT_ONEOSPOS:
            {
                cm_lbs_oneospos_attr_t *us_oneospos_atrr = (cm_lbs_oneospos_attr_t *)attr;
                memcpy(us_oneospos_atrr, &s_oneospos_attr_cache, sizeof(cm_lbs_oneospos_attr_t));
                us_oneospos_atrr->pid = s_oneospos_attr_cache.pid;
                break;
            }
        default :
            return ret;
    }
    ret=0;

#endif /* CM_LBS_OC_SUPPORT*/
    return ret;
}

