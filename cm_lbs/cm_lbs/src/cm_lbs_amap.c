#include "cm_lbs_amap.h"
#include "curl/curl.h"

#define CM_AMAP_HTTPPATH_LENMAX (1024)

#define CM_AMAP_REPEAT_REQUEST_FILTE (1)  /*过滤重复请求*/
#define CM_LBS_AMAP_DEBUG (0)             /*调试用宏定义*/


/*高德定位返回值映射*/
typedef struct
{
    cm_lbs_amap_event_e event;
    char *rsp_code;
} cm_lbs_amap_response_code_t;


/*定位传参句柄*/
typedef void* cm_amap_client_t;

cm_lbs_amap_response_code_t s_amap_rspcode[] =
{
    {AMAP_LOCATION_SUCCEED, "OK"},                            /*定位成功*/
    {AMAP_LOCATION_KEY_INVALID, "INVALID_USER_KEY"},          /*key非法或过期*/
    {AMAP_LOCATION_OVER_DAYQUOTA, "OVER_QUOTA"},              /*超出配额*/
    {AMAP_LOCATION_OVER_QPS, "CUQPS_HAS_EXCEEDED_THE_LIMIT"}, /*并发量超限*/
    {AMAP_LOCATION_UNKNOWN_ERR, "UNKNOWN_ERROR"}              /*未知错误*/

    /*  以下错误码 暂无需求 暂不使用
        LBS_SERVICE_NOT_EXIST,"SERVICE_NOT_EXIST",
        LBS_SERVICE_RESPONSE_ERROR,"SERVICE_RESPONSE_ERROR",
        LBS_INSUFFICIENT_PRIVILEGES,"INSUFFICIENT_PRIVILEGES",
        LBS_INVALID_PARAMS,"INVALID_PARAMS",
    */
};

/*CURL实现HTTP全局句柄*/
static CURL *s_amap_curl_handle = NULL;

/*CURLM全局句柄*/
static CURLM *s_amap_curl_multi_handle = NULL;

/*http 客户端句柄*/
static cm_curl_http_client_t *s_amap_http_client = NULL;

/*amap 句柄*/
static cm_amap_client_t s_amap_client = NULL;

/*定位结果缓存*/
static cm_lbs_amap_location_rsp_t *s_amap_location_rsp = NULL ;

/*定位结果缓存时间*/
static int s_rsp_time = 0;

static void __cm_amap_state_set(cm_amap_client_t amap_client, cm_amap_state_e state)
{
    cm_amap_t *client = (cm_amap_t *)amap_client;
    if(NULL != client)
    {
        client->state = state;
    }
}
static cm_amap_state_e __cm_amap_state_get(cm_amap_client_t amap_client)
{
    cm_amap_t *client = (cm_amap_t *)amap_client;
    if(NULL != client)
    {
        CMLBS_LOG("amap state :%d", client->state);
        return  client->state;
    }
    else
    {
        CMLBS_LOG("amap state null");
        return AMAP_FREE;
    }
}


static void __cm_amap_free_auth(cm_amap_authentication_t *auth)
{
    if(NULL == auth)
    {
        return ;
    }
    if(NULL != auth->apikey)
    {
        free(auth->apikey);
    }
    if(NULL != auth->digital_sign_key)
    {
        free(auth->digital_sign_key);
    }
    free(auth);
}



static void __cm_amap_delete(cm_amap_client_t client)
{
    cm_amap_t *amap = (cm_amap_t*)client;
    if(NULL == amap)
    {
        return ;
    }
    cm_amap_free_ueinfo(amap->ueinfo);

    __cm_amap_free_auth(amap->auth);

    __cm_amap_state_set(client, AMAP_FREE);

    free(amap);
    client = NULL;
    s_amap_client = NULL;
}



static cm_lbs_amap_event_e cm_lbs_amap_rspcode_parse(char *rsp_info)
{
    int count = sizeof(s_amap_rspcode) / sizeof(cm_lbs_amap_response_code_t);
    int i = 0;
    for(i = 0; i < count; i++)
    {
        if(0 == strcmp((char *)s_amap_rspcode[i].rsp_code, rsp_info))
        {
            return s_amap_rspcode[i].event;
        }
    }
    return AMAP_LOCATION_UNKNOWN_ERR;
}

static cm_lbs_state_e __amap_copy_location_rsp(char **location_rsp, char *json_vue)
{
    *location_rsp = NULL;
    if(NULL == json_vue)
    {
        return CM_LBS_ERR;
    }
    char *location_vue = NULL;
    int len = strlen(json_vue);

    if(len > 0 && len < CM_LBS_AMAP_RSP_LENMAX)
    {
        location_vue = cm_lbs_malloc((len + 1) * sizeof(char));
        if(NULL == location_vue)
        {
            return CM_LBS_MEMROYERR;
        }
    }
    else
    {
        CMLBS_LOG("len is long :%d", len);
        return CM_LBS_MEMROYERR;
    }

    memcpy(location_vue, json_vue, len);
    *location_rsp = location_vue;
    return CM_LBS_OK;
}

static void __amap_free(void *param)
{
    if(NULL != param)
    {
        free(param);
        param = NULL;
    }
}

static void __amap_location_rsp_free(cm_lbs_amap_location_rsp_t *rsp)
{
    if(NULL == rsp)
    {
        return ;
    }
    __amap_free(rsp->adcode);
    __amap_free(rsp->city);
    __amap_free(rsp->citycode);
    __amap_free(rsp->country);
    __amap_free(rsp->latitude);
    __amap_free(rsp->location_describe);
    __amap_free(rsp->longitude);
    __amap_free(rsp->poi);
    __amap_free(rsp->province);
    __amap_free(rsp->radius);
    __amap_free(rsp->road);
    __amap_free(rsp->district);
    __amap_free(rsp->street);
    __amap_free(rsp);
    rsp = NULL;
}

static cm_lbs_amap_event_e __cm_lbs_amapv1_parse(cm_lbs_amap_location_rsp_t *rsp, const uint8_t *param)
{

#if  CM_LBS_AMAP_DEBUG
    CMLBS_LOG("rsp:%s", param);
    CMLBS_LOG("rsp:%s", param + 64);
    CMLBS_LOG("rsp:%s", param + 128);
    CMLBS_LOG("rsp:%s", param + 192);
    CMLBS_LOG("rsp:%s", param + 256);
#endif /*CM_LBS_AMAP_DEBUG*/


    cm_lbs_amap_event_e ret = AMAP_LOCATION_SUCCEED;
    cJSON *location_json = NULL;
    location_json = cJSON_Parse((char *)param);

    do
    {
        if(NULL == location_json)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        //未用到,注释该参数
        //cJSON *infocode = cJSON_GetObjectItem(location_json, "infocode");
        cJSON *info = cJSON_GetObjectItem(location_json, "info");
        cJSON *status = cJSON_GetObjectItem(location_json, "status");
        cJSON *result = cJSON_GetObjectItem(location_json, "result");
        if(NULL != strstr(status->valuestring, "1") && NULL != result)
        {
            /*解析result子字段*/
            cJSON *city = cJSON_GetObjectItem(result, "city");
            cJSON *province = cJSON_GetObjectItem(result, "province");
            cJSON *adcode = cJSON_GetObjectItem(result, "adcode");
            cJSON *street = cJSON_GetObjectItem(result, "street");
            cJSON *poi = cJSON_GetObjectItem(result, "poi");
            cJSON *desc = cJSON_GetObjectItem(result, "desc");
            cJSON *country = cJSON_GetObjectItem(result, "country");
            //未用到,注释该参数
            //cJSON *type = cJSON_GetObjectItem(result, "type");
            cJSON *location_vue = cJSON_GetObjectItem(result, "location");
            cJSON *road = cJSON_GetObjectItem(result, "road");
            cJSON *radius = cJSON_GetObjectItem(result, "radius");
            cJSON *citycode = cJSON_GetObjectItem(result, "citycode");

            if(NULL != location_vue)
            {
                char *p_doc = strstr(location_vue->valuestring, ",");
                if(NULL != p_doc)
                {
                    int value_len = strlen(location_vue->valuestring);
                    char *p_tail = location_vue->valuestring + value_len;

                    int longitude_len = p_doc - location_vue->valuestring;
                    int latitude_len = p_tail - p_doc - 1;

                    if(longitude_len > 0 && longitude_len < CM_LBS_AMAP_RSP_LENMAX)
                    {
                        rsp->longitude = cm_lbs_malloc((longitude_len + 1) * sizeof(char));
                        if(NULL == rsp->longitude)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                        else
                        {
                            memcpy(rsp->longitude, location_vue->valuestring, longitude_len);
                        }
                    }
                    else
                    {
                        ret = AMAP_LOCATION_UNKNOWN_ERR;
                        break;
                    }

                    if(latitude_len > 0 && latitude_len < CM_LBS_AMAP_RSP_LENMAX)
                    {
                        rsp->latitude = cm_lbs_malloc((latitude_len + 1) * sizeof(char));
                        if(NULL == rsp->latitude)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                        else
                        {
                            memcpy(rsp->latitude, p_doc + 1, latitude_len);
                        }
                    }
                    else
                    {
                        ret = AMAP_LOCATION_UNKNOWN_ERR;
                        break;
                    }

                    cm_lbs_state_e  copy_ret=CM_LBS_OK;
                    if(NULL != city)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->city, city->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != province)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->province, province->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }


                    if(NULL != adcode)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->adcode, adcode->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != street)
                    {
                        copy_ret =   __amap_copy_location_rsp(&rsp->street, street->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != poi)
                    {
                        copy_ret =   __amap_copy_location_rsp(&rsp->poi, poi->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != desc)
                    {
                        copy_ret =   __amap_copy_location_rsp(&rsp->location_describe, desc->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != country)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->country, country->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != radius)
                    {
                        copy_ret =   __amap_copy_location_rsp(&rsp->radius, radius->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != road)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->road, road->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                    if(NULL != citycode)
                    {
                        copy_ret =    __amap_copy_location_rsp(&rsp->citycode, citycode->valuestring);
                        if(CM_LBS_OK != copy_ret)
                        {
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                            break;
                        }
                    }

                }
                else
                {
                    if(strcmp(info->valuestring,"OK"))        //当"info"不为OK的时候打印错误
                    {
                        ret = cm_lbs_amap_rspcode_parse(info->valuestring);
                        if(AMAP_LOCATION_SUCCEED == ret)
                        {
                            CMLBS_LOGI("get position failed!");
                            ret = AMAP_LOCATION_UNKNOWN_ERR;
                        }
                    }
                    else
                    {
                        ret = AMAP_LOCATION_UNKNOWN_ERR;
                    }
                    break;
                }
            }
            else
            {
                if(NULL != info)
                {
                    ret = cm_lbs_amap_rspcode_parse(info->valuestring);
                    if(AMAP_LOCATION_SUCCEED == ret)
                    {
                        CMLBS_LOGI("get position failed!");
                        ret = AMAP_LOCATION_UNKNOWN_ERR;
                    }                    
                }
                else
                {
                    ret = AMAP_LOCATION_UNKNOWN_ERR;
                }
                break;
            }
        }
        else
        {
            if(NULL != info)
            {
                ret = cm_lbs_amap_rspcode_parse(info->valuestring);
                if(AMAP_LOCATION_SUCCEED == ret)
                {
                    CMLBS_LOGI("get position failed!");
                    ret = AMAP_LOCATION_UNKNOWN_ERR;
                }
            }
            else
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
            }
            break;
        }

    } while(0);
    cJSON_Delete(location_json);
    return ret;
}
static cm_lbs_amap_event_e __cm_lbs_amapv2_parse(cm_lbs_amap_location_rsp_t *rsp, const uint8_t *param)
{

#if  CM_LBS_AMAP_DEBUG
    CMLBS_LOG("rsp:%s", param);
    CMLBS_LOG("rsp:%s", param + 64);
    CMLBS_LOG("rsp:%s", param + 128);
#endif /*CM_LBS_AMAP_DEBUG*/

    cm_lbs_amap_event_e ret = AMAP_LOCATION_SUCCEED;
    cJSON *location_json = NULL;
    location_json = cJSON_Parse((char *)param);

    do
    {
        if(NULL == location_json)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        cJSON *status = cJSON_GetObjectItem(location_json, "status");
        cJSON *info = cJSON_GetObjectItem(location_json, "info");
        cJSON *infocode = cJSON_GetObjectItem(location_json, "infocode");
        if(NULL == status || NULL == info || NULL == infocode)
        {
            if(NULL != info)
            {
                ret = cm_lbs_amap_rspcode_parse(info->valuestring);
            }
            else
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
            }
            break;
        }
        cJSON *position = cJSON_GetObjectItem(location_json, "position");
        if(NULL == position)
        {
            if(NULL != info)
            {
                ret = cm_lbs_amap_rspcode_parse(info->valuestring);
            }
            else
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
            }
            break;
        }
        //经纬度及精度解析
        cJSON *location_vue = cJSON_GetObjectItem(position, "location");
        cJSON *radius = cJSON_GetObjectItem(position, "radius");

        if(NULL == location_vue || NULL == radius)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }
        char *p_doc = strstr(location_vue->valuestring, ",");
        if(NULL == p_doc)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;

        }
        int value_len = strlen(location_vue->valuestring);
        char *p_tail = location_vue->valuestring + value_len;

        int longitude_len = p_doc - location_vue->valuestring;
        int latitude_len = p_tail - p_doc - 1;

        if(longitude_len > 0 && longitude_len < CM_LBS_AMAP_RSP_LENMAX)
        {
            rsp->longitude = cm_lbs_malloc((longitude_len + 1) * sizeof(char));
            if(NULL == rsp->longitude)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
            else
            {
                memcpy(rsp->longitude, location_vue->valuestring, longitude_len);
            }
        }
        else
        {
            CMLBS_LOG("long2 is long:%d", longitude_len);
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        if(latitude_len > 0 && latitude_len < CM_LBS_AMAP_RSP_LENMAX)
        {
            rsp->latitude = cm_lbs_malloc((latitude_len + 1) * sizeof(char));
            if(NULL == rsp->latitude)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
            else
            {
                memcpy(rsp->latitude, p_doc + 1, latitude_len);
            }
        }
        else
        {
            CMLBS_LOG("lat2 is long:%d", latitude_len);
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        char radius_str[12] = {0};
        sprintf(radius_str, "%d", radius->valueint);
        cm_lbs_state_e copy_ret =   __amap_copy_location_rsp(&rsp->radius, radius_str);
        if(CM_LBS_OK != copy_ret)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        //解析具体位置信息
        cJSON *desc = cJSON_GetObjectItem(location_json, "formatted_address");
        if(NULL == desc)
        {
            break;
        }
        copy_ret =   __amap_copy_location_rsp(&rsp->location_describe, desc->valuestring);
        if(CM_LBS_OK != copy_ret)
        {
            ret = AMAP_LOCATION_UNKNOWN_ERR;
            break;
        }

        cJSON *addressComponent = cJSON_GetObjectItem(location_json, "addressComponent");
        if(NULL == addressComponent)
        {
            break;
        }

        cJSON *country = cJSON_GetObjectItem(addressComponent, "country");
        cJSON *province = cJSON_GetObjectItem(addressComponent, "province");
        cJSON *city = cJSON_GetObjectItem(addressComponent, "city");
        cJSON *district = cJSON_GetObjectItem(addressComponent, "district");
        cJSON *citycode = cJSON_GetObjectItem(addressComponent, "citycode");
        cJSON *adcode = cJSON_GetObjectItem(addressComponent, "adcode");
        cJSON *street = cJSON_GetObjectItem(addressComponent, "street");
        cJSON *road = cJSON_GetObjectItem(addressComponent, "road");
        cJSON *poi = cJSON_GetObjectItem(addressComponent, "poi");


        if(NULL != country)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->country, country->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != province)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->province, province->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != city)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->city, city->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != district)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->district, district->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != citycode)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->citycode, citycode->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != adcode)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->adcode, adcode->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != street)
        {
            copy_ret =   __amap_copy_location_rsp(&rsp->street, street->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != road)
        {
            copy_ret =    __amap_copy_location_rsp(&rsp->road, road->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

        if(NULL != poi)
        {
            copy_ret =   __amap_copy_location_rsp(&rsp->poi, poi->valuestring);
            if(CM_LBS_OK != copy_ret)
            {
                ret = AMAP_LOCATION_UNKNOWN_ERR;
                break;
            }
        }

    } while(0);

    cJSON_Delete(location_json);
    return ret;
}
static cm_lbs_amap_event_e __cm_lbs_amap_http_location_parse(int platform, cm_lbs_amap_location_rsp_t *rsp, const uint8_t *param)
{
    if(NULL == rsp)
    {
        return AMAP_LOCATION_FAIL;
    }

    if(CM_LBS_AMAPV1 == platform)
    {
        return __cm_lbs_amapv1_parse(rsp, param);
    }
    else if(CM_LBS_AMAPV2 == platform)
    {
        return __cm_lbs_amapv2_parse(rsp, param);
    }
    else
    {
        return AMAP_LOCATION_FAIL;
    }
}

static void __cm_lbs_amap_http_release(CURL *client_handle)
{
    if(client_handle != NULL)
    {
        curl_multi_remove_handle(s_amap_curl_multi_handle, client_handle);
        curl_easy_cleanup(client_handle);
        client_handle = NULL;
    }

}


static void __cm_lbs_amap_rsp_backup(cm_lbs_amap_location_rsp_t *location_rsp)
{
    if(AMAP_LOCATION_SUCCEED != location_rsp->event)
    {
        if(NULL != s_amap_location_rsp)
        {
            __amap_location_rsp_free(s_amap_location_rsp);

        }
        s_amap_location_rsp = NULL;
        return ;
    }

    cm_lbs_amap_location_rsp_t *rsp = (cm_lbs_amap_location_rsp_t *)cm_lbs_malloc(sizeof(cm_lbs_amap_location_rsp_t));
    if(NULL == rsp)
    {
        return ;
    }
    rsp->event = location_rsp->event;

    __amap_copy_location_rsp(&rsp->longitude, location_rsp->longitude);
    __amap_copy_location_rsp(&rsp->latitude, location_rsp->latitude);
    __amap_copy_location_rsp(&rsp->radius, location_rsp->radius);
    __amap_copy_location_rsp(&rsp->country, location_rsp->country);
    __amap_copy_location_rsp(&rsp->province, location_rsp->province);
    __amap_copy_location_rsp(&rsp->city, location_rsp->city);
    __amap_copy_location_rsp(&rsp->citycode, location_rsp->citycode);
    __amap_copy_location_rsp(&rsp->adcode, location_rsp->adcode);
    __amap_copy_location_rsp(&rsp->poi, location_rsp->poi);
    __amap_copy_location_rsp(&rsp->location_describe, location_rsp->location_describe);

    __amap_location_rsp_free(s_amap_location_rsp);
    s_amap_location_rsp = rsp;

    s_rsp_time = time(NULL);

}

static size_t write_callback_curl(char *ptr, size_t size, size_t nmemb, void *rsp_content)
{
	size_t realsize = size * nmemb;
	cm_httpclient_callback_rsp_content_param_t *data = (cm_httpclient_callback_rsp_content_param_t *)rsp_content;

	// Reallocate buffer to fit new data
	uint8_t *temp = realloc(data->response_content, data->current_len + realsize);
	if (temp == NULL) 
    {
		CMLBS_LOGE("amap curl callback realloc rsp_content NULL");
		return 0;
	}
	data->response_content = temp;

	memcpy(&(data->response_content[data->current_len]), ptr, realsize);// Copy the data to the buffer
	data->current_len += realsize;

	// Update total length if this is the first time we are called or if we know the total length in advance
	// Note: In a real application, you might not know the total length beforehand, so you might want to handle this differently
	if (data->total_len == 0) 
    {
		// If total_len is 0, assume we are setting it for the first time (this is a simplification)
		data->total_len = data->current_len;
	}
	data->sum_len += realsize; // This can be used to track the cumulative number of bytes received, if needed
    CMLBS_LOG("amap curl callback rsp_content info :\ntotal_len = %d, sum_len = %d, current_len = %d", data->total_len, data->sum_len, data->current_len);
	return realsize;
}

static void cm_lbs_http_callback(CURL *client_handle, cm_httpclient_callback_event_e event, void *param)
{
    static int callback_state = 0; //避免接收HTTP消息重复触发。
    cm_amap_t *amap_client = s_amap_client;

    if(client_handle != amap_client->http_client)
    {
        return ;
    }

    void *cb_arg = amap_client->cb_arg;
    int platform = amap_client->auth->platform;

    CMLBS_LOGI("cm_lbs_http_callback:%d %d", event, (int)param);
    switch(event)
    {
        case CM_HTTP_CALLBACK_EVENT_REQ_START_SUCC_IND:
            {
                break;
            }
        case CM_HTTP_CALLBACK_EVENT_ERROR_IND:
            {
                cm_httpclient_error_event_e err_event = (cm_httpclient_error_event_e)(int)param;
                if(CM_HTTP_EVENT_CODE_CONNECT_TIMEOUT == err_event)
                {
                    amap_client->cb(AMAP_LOCATION_TIMEOUT, NULL, cb_arg);
                }
                else
                {
                    amap_client->cb(AMAP_LOCATION_CONNECTFAILL, NULL, cb_arg);
                }

                /*释放HTTP*/
                __cm_lbs_amap_http_release(s_amap_curl_handle);

                /*释放请求参数*/
                __cm_amap_delete(s_amap_client);
                break;
            }
        case CM_HTTP_CALLBACK_EVENT_RSP_HEADER_IND:
            {
                cm_httpclient_callback_rsp_header_param_t *callback_param = (cm_httpclient_callback_rsp_header_param_t *)param;

                if(200 != callback_param->response_code)
                {
                    CMLBS_LOGI("http resp_code:%d", callback_param->response_code);
                    amap_client->cb(AMAP_LOCATION_FAIL, NULL, cb_arg);
                }
                break;
            }
        case CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND:
            {
                if(2 == callback_state)
                {
                    return ;
                }
                callback_state = 2;

                cm_httpclient_callback_rsp_content_param_t *callback_param = (cm_httpclient_callback_rsp_content_param_t *)param;
                cm_lbs_amap_location_rsp_t *location_rsp = (cm_lbs_amap_location_rsp_t *)cm_lbs_malloc(sizeof(cm_lbs_amap_location_rsp_t));
                if(NULL == location_rsp)
                {
                    amap_client->cb(AMAP_LOCATION_UNKNOWN_ERR, NULL, cb_arg);
                }
                else
                {
                    cm_lbs_amap_event_e event_ret = __cm_lbs_amap_http_location_parse(platform, location_rsp, callback_param->response_content);
                    CMLBS_LOGI("%s,rsponse=%s ret=%d",__FUNCTION__,callback_param->response_content,event_ret);
                    location_rsp->event = event_ret;

                    /*缓存定位结果*/
                    __cm_lbs_amap_rsp_backup(location_rsp);

                    amap_client->cb(event_ret, location_rsp, cb_arg);
                }
                /*释放定位结果*/
                __amap_location_rsp_free(location_rsp);
                break;
            }
        case CM_HTTP_CALLBACK_EVENT_RSP_END_IND:
            {

                /*释放HTTP*/
                __cm_lbs_amap_http_release(s_amap_curl_handle);

                /*释放请求参数*/
                __cm_amap_delete(s_amap_client);

                callback_state = 0;

                break;
            }
        default :
            break;
    }
}


static int __cm_amap_digital_signature(char *requst_param, char *sign_key, char *sign)
{

#if CM_LBS_AMAP_DEBUG
    CMLBS_LOG("rp:%s", requst_param);
    CMLBS_LOG("rp:%s", requst_param + 64);
    CMLBS_LOG("rp:%s", requst_param + 128);
    CMLBS_LOG("rp:%s", requst_param + 192);
#endif
    char *temp_buf = (char *)cm_lbs_malloc(CM_AMAP_HTTPPATH_LENMAX * sizeof(char));
    if(NULL == temp_buf)
    {
        return -1;
    }
    strcat(temp_buf, requst_param);
    strcat(temp_buf, sign_key);

    cm_lbs_md5(temp_buf, sign);

    int len = strlen(sign);
    int i = 0;
    for(i = 0; i < len; i++)
    {
        if(sign[i] >= 'A' && sign[i] <= 'Z')
        {
            sign[i] += 32;
        }

    }
#if CM_LBS_AMAP_DEBUG
    CMLBS_LOG("m5:%s", sign);
    CMLBS_LOG("m5:%s", sign + 64);
#endif
    free(temp_buf);
    return 0;
}

static int __cm_amapv1_create_requst_HttpPath(char *http_path, cm_amap_ue_info_t *ue_info, cm_amap_authentication_t *auth)
{
    /**********************************************************/
    /*特别注意：此处字符串拼接顺序，影响数字签名，切不可修改*/
    /**********************************************************/
    int ret = -1;
#if CM_LBS_AMAPV1_SUPPORT
    strcat(http_path, CM_LBS_AMAPV1_PATH);
    strcat(http_path, "accesstype=0");

    char bts[64] = {0};
    sprintf(bts, "&bts=%d,%d,%s,%s,%d", ue_info->cell_info.mcc, ue_info->cell_info.mnc, (char *) ue_info->cell_info.lac, (char *) ue_info->cell_info.cellid, ue_info->cell_info.signal);
    strcat(http_path, bts);

    strcat(http_path, "&cdma=0");

    strcat(http_path, "&imei=");
    strcat(http_path, (char *) ue_info->imei);

    strcat(http_path, "&key=");
    strcat(http_path, (char *)auth->apikey);

    if(NULL != ue_info->nearbts)
    {
        strcat(http_path, "&nearbts=");
        strcat(http_path, (char *)ue_info->nearbts);
    }

    if(NULL != ue_info->network_type)
    {
        strcat(http_path, "&network=");
        strcat(http_path, (char *)ue_info->network_type);
    }

    strcat(http_path, "&output=json");

    if(NULL != ue_info->server_ip)
    {
        strcat(http_path, "&serverip=");
        strcat(http_path, (char *)ue_info->server_ip);
    }

    /*进行数字签名*/
    if(auth->digital_sign_enable)
    {
        int unuse_head_len = strlen(CM_LBS_AMAPV1_PATH);
        char sign_md5[64] = {0};
        if(0 == __cm_amap_digital_signature((char *)(http_path + unuse_head_len), auth->digital_sign_key, sign_md5))
        {
            strcat(http_path, "&sig=");
            strcat(http_path, sign_md5);
        }
        else
        {
            CMLBS_LOGI("sign1 fail");
        }
    }
    ret = strlen(http_path);

#if CM_LBS_AMAP_DEBUG
    CMLBS_LOG("requst:%s", http_path);
    CMLBS_LOG("requst:%s", http_path + 64);
    CMLBS_LOG("requst:%s", http_path + 128);
#endif /*CM_LBS_AMAP_DEBUG*/

#endif /*CM_LBS_AMAPV1_SUPPORT*/
    return ret;
}


/*处理高德2.0邻区信息*/
static char * __amav2deal_nearbts_create(char *near_bts)
{
    char *ret = NULL;
    if(NULL == near_bts)
    {
        return ret;
    }
    int length = strlen(near_bts);
    if(length<=5)
    {
        return ret;
    }    
    int i = 0;
    int j = 0;
    ret = (char *)cm_lbs_malloc((length + 64) * sizeof(char));
    if(NULL == ret)
    {
        return ret;
    }

    for(i = 0; i < length; i++,j++)
    {
        if('|' == near_bts[i])
        {
            ret[j] = ',';
            j++;
            ret[j] = '5';
            j++;
        }
        ret[j] = near_bts[i];
    }
    strcat(ret,",5");
    return ret;
}

static void __amav2deal_nearbts_delete(char *near_bts)
{
   free(near_bts);
}

static int __cm_amapv2_create_requst_HttpPath(char *http_path, cm_amap_ue_info_t *ue_info, cm_amap_authentication_t *auth)
{
    /**********************************************************/
    /*特别注意：此处字符串拼接顺序，影响数字签名，切不可修改*/
    /**********************************************************/
    int ret = -1;
#if  CM_LBS_AMAPV2_SUPPORT

    strcat(http_path, CM_LBS_AMAPV2_PATH);
    strcat(http_path, "accesstype=0");

    char bts[64] = {0};
    sprintf(bts, "&bts=%d,%d,%s,%s,%d,%d", ue_info->cell_info.mcc, ue_info->cell_info.mnc, (char *) ue_info->cell_info.lac, (char *) ue_info->cell_info.cellid, ue_info->cell_info.signal, ue_info->cage);
    strcat(http_path, bts);

    strcat(http_path, "&cdma=0");

    strcat(http_path, "&imei=");
    strcat(http_path, (char *) ue_info->imei);

    strcat(http_path, "&key=");
    strcat(http_path, (char *)auth->apikey);

#if 1
    if(NULL != ue_info->nearbts)
    {
        char *near=__amav2deal_nearbts_create((char *)ue_info->nearbts);
        if(NULL!=near)
        {
          strcat(http_path, "&nearbts=");
          strcat(http_path, (char *)near);
        }
        __amav2deal_nearbts_delete(near);
    }
#endif

    if(NULL != ue_info->network_type)
    {
        strcat(http_path, "&network=");
        strcat(http_path, (char *)ue_info->network_type);
    }

    strcat(http_path, "&output=json");

    if(NULL != ue_info->server_ip)
    {
        strcat(http_path, "&serverip=");
        strcat(http_path, (char *)ue_info->server_ip);
    }

    if(1 == auth->show_fields_enable) //开启逆地理编码结果开关
    {
        strcat(http_path, "&show_fields=formatted_address,addressComponent");
    }

    /*进行数字签名*/
    if(auth->digital_sign_enable)
    {
        int unuse_head_len = strlen(CM_LBS_AMAPV2_PATH);
        char sign_md5[64] = {0};
        if(0 == __cm_amap_digital_signature((char *)(http_path + unuse_head_len), auth->digital_sign_key, sign_md5))
        {
            strcat(http_path, "&sig=");
            strcat(http_path, sign_md5);
        }
        else
        {
            CMLBS_LOGI("sign2 fail");
        }
    }
    ret = strlen(http_path);

#if CM_LBS_AMAP_DEBUG
    CMLBS_LOG("requst:%s", http_path);
    CMLBS_LOG("requst:%s", http_path + 64);
    CMLBS_LOG("requst:%s", http_path + 128);
#endif /*CM_LBS_AMAP_DEBUG*/

#endif /*CM_LBS_AMAPV2_SUPPORT*/
    return ret;
}

static int __cm_amap_create_requst_HttpPath(char *http_path, cm_amap_ue_info_t *ueinfo, cm_amap_authentication_t *auth)
{
    int ret = -1;
    int platfrom = auth->platform;
    if(CM_LBS_AMAPV1 == platfrom)
    {
        ret = __cm_amapv1_create_requst_HttpPath(http_path, ueinfo, auth);
    }
    if(CM_LBS_AMAPV2 == platfrom)
    {
        ret = __cm_amapv2_create_requst_HttpPath(http_path, ueinfo, auth);
    }
    return ret;
}


static cm_lbs_state_e __cm_amap_http_requst(cm_amap_client_t client)
{
    cm_amap_t *attr = (cm_amap_t *)client;

    cm_lbs_state_e ret = CM_LBS_OK;
    uint8_t *http_path = (uint8_t *)cm_lbs_malloc(CM_AMAP_HTTPPATH_LENMAX * sizeof(uint8_t));
    char *curl_url;
    void *rsp_content;

    if(NULL == http_path)
    {
        return CM_LBS_MEMROYERR;
    }

    do
    {
        /*创建http 请求串*/
        int http_len = __cm_amap_create_requst_HttpPath((char *)http_path, attr->ueinfo, attr->auth);
        if(http_len < 0)
        {
            ret = CM_LBS_ERR;
            break;
        }

        /*创建http client*/
        char *http_url = (attr->auth->platform == CM_LBS_AMAPV1) ? CM_LBS_AMAPV1_URL : CM_LBS_AMAPV2_URL;

        /*初始化oneos curl handle and client*/
        if(NULL == s_amap_curl_handle && NULL == s_amap_http_client)
        {
            s_amap_curl_handle = curl_easy_init();
            s_amap_curl_multi_handle = curl_multi_init();
            if (NULL == s_amap_curl_handle && NULL == s_amap_curl_multi_handle)
            {
                ret = CM_LBS_ERR;
                break;
            }
        } 
        attr->http_client = s_amap_curl_handle;

        snprintf(curl_url, strlen(http_url) + strlen((char *)http_path), "%s%s", http_url, (char *)http_path);

	    curl_easy_setopt(s_amap_curl_handle, CURLOPT_URL, curl_url);
	    curl_easy_setopt(s_amap_curl_handle, CURLOPT_NOSIGNAL, 1L);

        /*执行curl HTTP请求*/
        CURLMcode res = curl_multi_add_handle(s_amap_curl_multi_handle, s_amap_curl_handle);
        if (CURLM_OK != res )
        {
            ret = CM_LBS_ERR;
            break;
        }
        int still_running = 0;
        res = curl_multi_perform(s_amap_curl_multi_handle, &still_running);
        if (CURLM_OK != res )
        {
            ret = CM_LBS_ERR;
            break;
        }

	    curl_easy_setopt(s_amap_curl_handle, CURLOPT_WRITEFUNCTION, write_callback_curl);
	    curl_easy_setopt(s_amap_curl_handle, CURLOPT_WRITEDATA, rsp_content);

        cm_lbs_http_callback(attr->http_client, CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND, (void*)rsp_content);

    } while(0);

    free(http_path);

    return ret;
}

static void __cm_amap_copy_ueinfo(cm_amap_ue_info_t *src, cm_amap_ue_info_t *des)
{
    memcpy(des, src, sizeof(cm_amap_ue_info_t));

    int len = 0;
    if(NULL != src->nearbts)
    {
        len = strlen((char *)src->nearbts);
        uint8_t *nearbts = (uint8_t *)cm_lbs_malloc((len + 1) * sizeof(uint8_t));
        if(NULL == nearbts)
        {
            return ;
        }
        strcpy((char *)nearbts, (char *)src->nearbts);
        des->nearbts = nearbts;
    }

    if(NULL != src->server_ip)
    {
        len = strlen((char *)src->server_ip);
        uint8_t *server_ip = (uint8_t *)cm_lbs_malloc((len + 1) * sizeof(uint8_t));
        if(NULL == server_ip)
        {
            return ;
        }
        strcpy((char *)server_ip, (char *)src->server_ip);
        des->nearbts = server_ip;
    }

    if(NULL != src->network_type)
    {
        len = strlen((char *)src->network_type);
        uint8_t *network_type = (uint8_t *)cm_lbs_malloc((len + 1) * sizeof(uint8_t));
        if(NULL == network_type)
        {
            return ;
        }
        strcpy((char *)network_type, (char *)src->network_type);
        des->nearbts = network_type;
    }

}



static void __cm_amap_copy_auth(cm_amap_authentication_t *src, cm_amap_authentication_t *des)
{
    memcpy(des, src, sizeof(cm_amap_authentication_t));

    int len = 0;
    if(NULL != src->apikey)
    {
        len = strlen((char *)src->apikey);
        uint8_t *apikey = (uint8_t *)cm_lbs_malloc((len + 1) * sizeof(uint8_t));
        if(NULL == apikey)
        {
            return ;
        }
        strcpy((char *)apikey, (char *)src->apikey);
        des->apikey = (char *) apikey;
    }

    if(NULL != src->digital_sign_key)
    {
        len = strlen((char *)src->digital_sign_key);
        uint8_t *digital_sign_key = (uint8_t *)cm_lbs_malloc((len + 1) * sizeof(uint8_t));
        if(NULL == digital_sign_key)
        {
            return ;
        }
        strcpy((char *)digital_sign_key, (char *)src->digital_sign_key);
        des->digital_sign_key = (char *)digital_sign_key;
    }

}

/*上一次ueinfo缓存*/
static cm_amap_ue_info_t *s_ue_info_last = NULL;

static cm_lbs_state_e __cm_amap_ueinfo_compare(cm_amap_ue_info_t *new_ueinfo)
{
    if(NULL == s_ue_info_last)
    {
        return CM_LBS_ERR;
    }

    if(NULL == new_ueinfo)
    {
        return CM_LBS_ERR;
    }

    cm_amap_ue_info_t *old_ueinfo = s_ue_info_last;

    cm_lbs_state_e ret = CM_LBS_ERR;

    do
    {

        if(0 != strcmp((char *)old_ueinfo->imei, (char *)new_ueinfo->imei))
        {
            break;
        }

        if(0 != strcmp((char *)old_ueinfo->imsi, (char *)new_ueinfo->imsi))
        {
            break;
        }

        if(0 != strcmp((char *)old_ueinfo->cell_info.lac, (char *)new_ueinfo->cell_info.lac))
        {
            break;
        }

        if(0 != strcmp((char *)old_ueinfo->cell_info.cellid, (char *)new_ueinfo->cell_info.cellid))
        {
            break;
        }

        if(NULL != old_ueinfo->nearbts && NULL != new_ueinfo->nearbts)
        {
            if(0 != strcmp((char *)old_ueinfo->nearbts, (char *)new_ueinfo->nearbts))
            {
                CMLBS_LOGI("nearbts is different");
                break;
            }
        }
        if((NULL == old_ueinfo->nearbts) ^ (NULL == new_ueinfo->nearbts))
        {
            break;
        }

        if(old_ueinfo->cell_info.mcc != new_ueinfo->cell_info.mcc)
        {
            break;
        }
        if(old_ueinfo->cell_info.mnc != new_ueinfo->cell_info.mnc)
        {
            break;
        }
        if(old_ueinfo->cell_info.signal != new_ueinfo->cell_info.signal)
        {
            break;
        }

        ret = CM_LBS_OK;
    } while(0);

    return ret;
}

static void __cm_amap_backup_ueinfo(cm_amap_ue_info_t *new_ueinfo)
{
    if(NULL != s_ue_info_last)
    {
        cm_amap_free_ueinfo(s_ue_info_last);
    }

    cm_amap_ue_info_t *backup_ueinfo = cm_lbs_malloc(sizeof(cm_amap_ue_info_t));
    if(backup_ueinfo == NULL)
    {
        return ;
    }
    __cm_amap_copy_ueinfo(new_ueinfo, backup_ueinfo);

    s_ue_info_last = backup_ueinfo;
}

static cm_lbs_state_e __cm_amap_return_last_location(cm_amap_t *attr)
{
    cm_lbs_state_e ret = CM_LBS_OK;
    if(NULL == s_amap_location_rsp)
    {
        return CM_LBS_ERR;
    }

    /*定位结果有效性     判断*/
    int time_now = time(NULL);
    int time_dif = time_now - s_rsp_time;
    if(time_dif < 0 || time_dif > CM_LBS_AMAP_LOCATION_RSP_VALID_TIME)
    {
        CMLBS_LOGI("Last result failure");
        return CM_LBS_ERR;
    }

    if(NULL != attr->cb)
    {
        cm_lbs_amap_location_rsp_t *old_rsp = s_amap_location_rsp;
        cm_lbs_amap_location_rsp_t *rsp = (cm_lbs_amap_location_rsp_t *)cm_lbs_malloc(sizeof(cm_lbs_amap_location_rsp_t));
        if(NULL == rsp)
        {
            return CM_LBS_ERR;
        }
        rsp->event = old_rsp->event;

        __amap_copy_location_rsp(&rsp->longitude, old_rsp->longitude);
        __amap_copy_location_rsp(&rsp->latitude, old_rsp->latitude);
        __amap_copy_location_rsp(&rsp->radius, old_rsp->radius);
        __amap_copy_location_rsp(&rsp->country, old_rsp->country);
        __amap_copy_location_rsp(&rsp->province, old_rsp->province);
        __amap_copy_location_rsp(&rsp->city, old_rsp->city);
        __amap_copy_location_rsp(&rsp->citycode, old_rsp->citycode);
        __amap_copy_location_rsp(&rsp->adcode, old_rsp->adcode);
        __amap_copy_location_rsp(&rsp->poi, old_rsp->poi);
        __amap_copy_location_rsp(&rsp->location_describe, old_rsp->location_describe);

        if(strlen((char *)rsp->longitude) > 6 && strlen((char *)rsp->latitude) > 6)
        {
            attr->cb(AMAP_LOCATION_SUCCEED, rsp, attr->cb_arg);
        }
        else
        {
            ret = CM_LBS_ERR;
        }
        __amap_location_rsp_free(rsp);

        return ret;
    }
    else
    {
        return CM_LBS_ERR;
    }

}

/*缓存上一次传入的apikey*/
static char *s_amap_last_key = NULL;

/*缓存上一次传入的signkey*/
static char *s_amap_last_signkey = NULL;

static int s_show_fields_enable = 0;

static int s_signen = 0;

static void __cm_amap_backup_lastkey(char *last_key, char *last_signkey, int show_fields, int last_signen)
{
    s_signen = last_signen;
    s_show_fields_enable = show_fields;
    if(NULL != last_key)
    {
        int last_key_len = strlen(last_key);
        if(NULL != s_amap_last_key)
        {
            free(s_amap_last_key);
            s_amap_last_key = NULL;
        }
        s_amap_last_key = (char *)cm_lbs_malloc((last_key_len + 1) * sizeof(char));
        if(NULL == s_amap_last_key)
        {
            return ;
        }
        strncpy(s_amap_last_key, last_key, last_key_len);
    }

    if(NULL != last_signkey)
    {
        int last_signkey_len = strlen(last_signkey);
        if(NULL != s_amap_last_signkey)
        {
            free(s_amap_last_signkey);
            s_amap_last_signkey = NULL;
        }
        s_amap_last_signkey = (char *)cm_lbs_malloc((last_signkey_len + 1) * sizeof(char));
        if(NULL == s_amap_last_signkey)
        {

            return ;
        }
        strncpy(s_amap_last_signkey, last_signkey, last_signkey_len);
    }


}


static cm_lbs_state_e __cm_amap_compare_lastkey(char *last_key, char *last_signkey, int show_fields, int sign_en)
{
    if((NULL == last_key && NULL != s_amap_last_key) || (NULL != last_key && NULL == s_amap_last_key))
    {
        return CM_LBS_ERR;
    }
    if(NULL != last_key && NULL != s_amap_last_key)
    {
        if(0 != strcmp(last_key, s_amap_last_key))
        {
            return CM_LBS_ERR;
        }
    }

    if((NULL == last_signkey && NULL != s_amap_last_signkey) || (NULL != last_signkey && NULL == s_amap_last_signkey))
    {
        return CM_LBS_ERR;
    }
    if(NULL != last_signkey && NULL != s_amap_last_signkey)
    {
        if(0 != strcmp(last_signkey, s_amap_last_signkey))
        {
            return CM_LBS_ERR;
        }
    }

    if(s_show_fields_enable != show_fields)
    {
        return CM_LBS_ERR;
    }
    if(s_signen != sign_en)
    {
        return CM_LBS_ERR;
    }
    return CM_LBS_OK;
}



static cm_amap_client_t __cm_amap_create_by_attr(cm_amap_t *attr)
{
    if(NULL == attr || NULL == attr->auth || NULL == attr->ueinfo)
    {
        CMLBS_LOG("amap_attr is null!");
        return NULL;
    }

    cm_amap_t *amap_client = cm_lbs_malloc(sizeof(cm_amap_t));
    if(amap_client == NULL)
    {
        return NULL;
    }

    cm_amap_ue_info_t *amap_client_ueinfo = cm_lbs_malloc(sizeof(cm_amap_ue_info_t));
    if(amap_client_ueinfo == NULL)
    {
        free(amap_client);
        return NULL;
    }

    cm_amap_authentication_t *amap_client_auth = cm_lbs_malloc(sizeof(cm_amap_authentication_t));
    if(amap_client_auth == NULL)
    {
        free(amap_client) ;
        free(amap_client_ueinfo);
        return NULL;
    }

    /*copy ueinfo*/
    __cm_amap_copy_ueinfo(attr->ueinfo, amap_client_ueinfo);

    /*copy auth*/
    __cm_amap_copy_auth(attr->auth, amap_client_auth);

    amap_client->http_client = s_amap_curl_handle;//amap clien handle use curl handle
    amap_client->cb = attr->cb;
    amap_client->cb_arg = attr->cb_arg;
    amap_client->ueinfo = amap_client_ueinfo;
    amap_client->auth = amap_client_auth;

    return amap_client;
}

cm_lbs_state_e cm_amap_location(cm_amap_t *attr)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
#if (CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV2_SUPPORT)
    if((AMAP_BUSY == __cm_amap_state_get(s_amap_client)) || NULL != s_amap_client)
    {
        CMLBS_LOG("amap busy!");
        return CM_LBS_BUSY;
    }

#if CM_AMAP_REPEAT_REQUEST_FILTE
    //如果是重复请求且鉴权参数没有变化 则返回上一次定位的结果/
    if(CM_LBS_OK == __cm_amap_ueinfo_compare(attr->ueinfo) && CM_LBS_OK == __cm_amap_compare_lastkey(attr->auth->apikey, attr->auth->digital_sign_key, attr->auth->show_fields_enable, attr->auth->digital_sign_enable))
    {
        if(CM_LBS_OK == __cm_amap_return_last_location(attr))
        {
            CMLBS_LOG("return last rsp");
            return CM_LBS_OK;
        }
    }
#endif

    cm_amap_client_t amap_client = __cm_amap_create_by_attr(attr);
    if(NULL == amap_client)
    {
        return CM_LBS_PARAMERR;
    }

    /*赋值给全局变量*/
    s_amap_client = amap_client;

    __cm_amap_state_set(amap_client, AMAP_BUSY);

    /*备份鉴权参数*/
    __cm_amap_backup_lastkey(attr->auth->apikey, attr->auth->digital_sign_key, attr->auth->show_fields_enable, attr->auth->digital_sign_enable);

    /*备份缓存ueinfo*/
    __cm_amap_backup_ueinfo(attr->ueinfo);

    /*执行请求*/
    ret = __cm_amap_http_requst(amap_client);
    if(CM_LBS_OK != ret)
    {
        CMLBS_LOG("http requst fail! err code:%d", ret);
        __cm_lbs_amap_http_release(s_amap_curl_handle);
        __cm_amap_delete(amap_client);
    }
#endif /*(CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV1_SUPPORT)*/
    return ret;
}

cm_lbs_state_e cm_amap_get_ueinfo(cm_amap_ue_info_t **ue_info, int nearbts_enable)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
    
#if (CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV2_SUPPORT)
    ret = CM_LBS_OK;
    cm_amap_ue_info_t  *s_ue_info = (cm_amap_ue_info_t *)cm_lbs_malloc(sizeof(cm_amap_ue_info_t));

    do
    {
        /*
        if(NULL == s_ue_info)
        {
            ret = CM_LBS_MEMROYERR;
            break;
        }
        */
#ifdef NONE
        ret = __cm_lbs_GetImei(s_ue_info->imei);
        CMLBS_LOG("%s s_ue_info->imei=%s",__FUNCTION__,s_ue_info->imei);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetImsi(s_ue_info->imsi);
        if(CM_LBS_OK != ret)
        {
            break;
        }




#if 0 
        ret = __cm_lbs_GetMcc(&s_ue_info->mcc);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetMnc(&s_ue_info->mnc);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetLac(s_ue_info->lac);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetCellid(s_ue_info->cellid);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetSignal(&s_ue_info->signal);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetCage(&s_ue_info->cage);
        if(CM_LBS_OK != ret)
        {
            break;
        }
#endif
        ret = __cm_lbs_GetCell_info(&s_ue_info->cell_info);
        if(CM_LBS_OK != ret)
        {
            break;
        }
#endif
#if 0
    char rsp_test[150] = {0};
    sprintf(rsp_test,"imei=%s, imsi=%s, cellid=%s, lac=%s, mcc=%d, mnc=%d, singal=%d", s_ue_info->imei, s_ue_info->imsi, s_ue_info->cell_info.cellid,
                            s_ue_info->cell_info.lac, s_ue_info->cell_info.mcc, s_ue_info->cell_info.mnc, s_ue_info->cell_info.signal);
    cm_uart_printf(g_lbs_atOpId,(const uint8_t *)rsp_test);
    free((void *)rsp_test);
#endif

#if CM_LBS_UEINFO_SERVERIP_SUPPORT

#endif /*CM_LBS_UEINFO_SERVERIP_SUPPORT*/

#if CM_LBS_UEINFO_NEARBETS_SUPPORT
        if(nearbts_enable)
        {
            uint8_t *nearbts = (uint8_t*)cm_lbs_malloc(1024 * sizeof(uint8_t));

            cm_lbs_state_e op_ret = __cm_lbs_GetNearbts(nearbts);
            if(CM_LBS_OK != op_ret)
            {
                s_ue_info->nearbts = NULL;
                free(nearbts);
                CMLBS_LOG("Nearbts fail!");
            }
            else
            {
                s_ue_info->nearbts = nearbts;
            }
        }
#endif /*CM_LBS_UEINFO_NEARBETS_SUPPORT*/

#if CM_LBS_UEINFO_NETEWORK_TYPE_SUPPORT

#endif /*CM_LBS_UEINFO_NETEWORK_TYPE_SUPPORT*/

    } while(0);

    *ue_info = s_ue_info;

#endif /*(CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV1_SUPPORT)*/
    return ret;
}

void cm_amap_free_ueinfo(cm_amap_ue_info_t *ue_info)
{

#if (CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV1_SUPPORT)
    if(NULL == ue_info)
    {
        return ;
    }

#if CM_LBS_UEINFO_SERVERIP_SUPPORT
    if(NULL != ue_info->server_ip)
    {
        free(ue_info->server_ip);
        ue_info->server_ip = NULL;

    }
#endif /*CM_LBS_UEINFO_SERVERIP_SUPPORT*/

#if CM_LBS_UEINFO_NEARBETS_SUPPORT
    if(NULL != ue_info->nearbts)
    {
        free(ue_info->nearbts);
        ue_info->nearbts = NULL;
    }
#endif /*CM_LBS_UEINFO_NEARBETS_SUPPORT*/

#if CM_LBS_UEINFO_NETEWORK_TYPE_SUPPORT
    if(NULL != ue_info->network_type)
    {
        free(ue_info->network_type);
        ue_info->network_type = NULL;
    }
#endif /*CM_LBS_UEINFO_NETEWORK_TYPE_SUPPORT*/

    free(ue_info);
    ue_info = NULL;
#endif /*#if (CM_LBS_AMAPV1_SUPPORT||CM_LBS_AMAPV1_SUPPORT)*/
}

