#include "cm_lbs_oneospos.h"

#define ONEOSPOS_HTTP_CONTENT_LEN (1024)
#define CM_LBS_ONEOSPOS_RSP_LENMAX (64)

#define ONEOSPOS_HTTP_HOST "https://os.iot.10086.cn/api/ops/cms/lbs/report"
#define ONEOSPOS_HTTP_PATH "/api/ops/cms/lbs/report"

#define ONEOSPOS_REPEAT_REQUEST_FILTE (1) /*过滤重复请求*/
#define ONEOSPOS_DEBUG (0) /*调试模式*/

/*ONEOSPOS 状态*/
typedef enum {
	ONEOSPOS_STATE_FREE = 0, /*闲置*/
	ONEOSPOS_STATE_BUSY /*忙*/
} oneospos_state_e;

typedef struct {
	cm_oneospos_callback_event_e event;
	char *code_str;
} oneospos_receive_event_t;

/*ONEOSPOS 状态*/
static int s_oneospos_state = 0;

/*CURL实现HTTP全局句柄*/
static CURL *s_oneospos_curl_handle = NULL;

/*CURLM全局句柄*/
static CURLM *s_curl_multi_handle = NULL;

/*oneospos 定位相关参数 句柄*/
static cm_oneospos_attr_t *s_oneospos_client = NULL;

/*请求数据*/
static char *s_http_content = NULL;

/*Http请求中 conten数据的长度*/
static uint32_t s_http_conten_len = 0;

/*定位结果缓存*/
static cm_oneospos_location_rsp_t *s_rsp_buf = NULL;

/*定位结果缓存的时间*/
static int s_rsp_time = 0;

static oneospos_receive_event_t receive_event[] = { { CM_ONEOSPOS_EVENT_FAIL, "10000" },
						    { CM_ONEOSPOS_EVENT_OVER_RESTRICT, "11000" },
						    { CM_ONEOSPOS_EVENT_UNSET_PLATFORM, "12000" } };

static cm_oneospos_callback_event_e __cm_oneospos_find_callback_event_by_receive(char *code)
{
	int count = sizeof(receive_event) / sizeof(oneospos_receive_event_t);
	int i = 0;
	for (i = 0; i < count; i++) {
		if (!strcmp(receive_event[i].code_str, code)) {
			return receive_event[i].event;
		}
	}
	return CM_ONEOSPOS_EVENT_UNKNOW;
}

/*
*@brief 释放定位相关参数内存
*/
static void __cm_oneospos_oneospos_client_free(cm_oneospos_attr_t *attr)
{
	if (NULL != attr) {
		if (NULL != attr->pid) {
			free(attr->pid);
			attr->pid = NULL;
		}
		cm_oneospos_free_ueinfo(attr->ueinfo);
		free(attr);
		attr = NULL;
	}
	if (NULL != s_http_content) {
		free(s_http_content);
		s_http_content = NULL;
	}

	s_oneospos_state = ONEOSPOS_STATE_FREE;
	s_http_conten_len = 0;
}

/*
*@brief 释放http相关资源
*/
static void __cm_oneospos_http_release(CURL *client_handle)
{
	if (client_handle != NULL) {
		curl_multi_remove_handle(s_curl_multi_handle, client_handle);
		curl_easy_cleanup(client_handle);
		client_handle = NULL;
	}
}

/*
*@brief 将经纬度字符串拆分为精度和纬度字符串
*/
static cm_lbs_state_e __cm_location_string_parse_to_latitude_longitude(char *la_longitude, char *latitude, char *longitude)
{
	if (NULL == la_longitude) {
		return CM_LBS_PARAMERR;
	}

	int len = strlen(la_longitude);

	char *p_head = la_longitude;
	char *p_tai = la_longitude + len;
	char *p_doc = strstr(la_longitude, ",");

	if (NULL != p_doc) {
		memcpy(latitude, p_head, p_doc - p_head);
		memcpy(longitude, p_doc + 1, p_tai - p_doc);
		return CM_LBS_OK;
	}

	return CM_LBS_ERR;
}

static cm_lbs_state_e __oneospos_copy_location_rsp(char **location_rsp, char *json_vue)
{
	*location_rsp = NULL;
	if (NULL == json_vue) {
		return CM_LBS_ERR;
	}
	char *location_vue = NULL;
	int len = strlen(json_vue);

	if (len > 0 && len < CM_LBS_ONEOSPOS_RSP_LENMAX) {
		location_vue = cm_lbs_malloc((len + 1) * sizeof(char));
		if (NULL == location_vue) {
			return CM_LBS_MEMROYERR;
		}
	} else {
		//CMLBS_LOG("loc_vue is long :%d", len);
		return CM_LBS_MEMROYERR;
	}

	memcpy(location_vue, json_vue, len);
	*location_rsp = location_vue;
	return CM_LBS_OK;
}

static cm_oneospos_callback_event_e __cm_oneospos_http_location_parse(cm_oneospos_location_rsp_t *rsp, const uint8_t *param)
{
	cm_oneospos_callback_event_e ret = CM_ONEOSPOS_EVENT_SUCCESS;

	cJSON *location_json = NULL;
	location_json = cJSON_Parse((char *)param);
	do {
		if (NULL == location_json) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		cJSON *success = cJSON_GetObjectItem(location_json, "success");
		//unused
		//        cJSON *message = cJSON_GetObjectItem(location_json,  "message");
		cJSON *code = cJSON_GetObjectItem(location_json, "code");
		cJSON *data = cJSON_GetObjectItem(location_json, "data");

		if (NULL == success) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		//       if(!cJSON_IsTrue(success) && NULL != code)
		if (!((success->type & 0xff) == cJSON_True) && NULL != code) {
			ret = __cm_oneospos_find_callback_event_by_receive(code->valuestring);
			break;
		}

		//        if(!cJSON_IsTrue(success) || NULL == data)
		if (!((success->type & 0xff) == cJSON_True) || NULL == data) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		cJSON *pos = cJSON_GetObjectItem(data, "pos");
		if (NULL == pos) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		char latitude_temp[24] = { 0 };
		char longitude_temp[24] = { 0 };

		if (CM_LBS_OK !=
		    __cm_location_string_parse_to_latitude_longitude(pos->valuestring, latitude_temp, longitude_temp)) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		int cp_ret = __oneospos_copy_location_rsp(&rsp->latitude, latitude_temp);
		if (CM_LBS_OK != cp_ret) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

		cp_ret = __oneospos_copy_location_rsp(&rsp->longitude, longitude_temp);
		if (CM_LBS_OK != cp_ret) {
			ret = CM_ONEOSPOS_EVENT_UNKNOW;
			break;
		}

	} while (0);
	cJSON_Delete(location_json);
	rsp->event = ret;

#if ONEOSPOS_DEBUG
	//CMLBS_LOG("rev:%s", param);
	//CMLBS_LOG("rev:%s", param + 64);
	//CMLBS_LOG("rev:%s", param + 128);
	//CMLBS_LOG("rev:%s", param + 196);
	//CMLBS_LOG("rev:%s", param + 256);
#endif

	return ret;
}

static void __cm_oneospos_location_rsp_free(cm_oneospos_location_rsp_t *rsp)
{
	if (NULL == rsp) {
		return;
	}
	if (NULL != rsp->latitude && strlen(rsp->latitude) > 2) {
		free(rsp->latitude);
		rsp->latitude = NULL;
	}

	if (NULL != rsp->longitude && strlen(rsp->longitude) > 2) {
		free(rsp->longitude);
		rsp->longitude = NULL;
	}
	free(rsp);
	rsp = NULL;
}

static cm_lbs_state_e __cm_oneospos_http_send_contant(void)
{
	CURLcode ret;
	if (NULL == s_http_content || NULL == s_oneospos_curl_handle) {
		return CM_LBS_ERR;
	}
	curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_POSTFIELDS, s_http_content);
	curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_POSTFIELDSIZE, s_http_conten_len);
	CURLMcode res_m = curl_multi_add_handle(s_curl_multi_handle, s_oneospos_curl_handle);
	if (CURLM_OK != res_m) {
		return CM_LBS_ERR;
	}
	int still_running = 0;
	res_m = curl_multi_perform(s_curl_multi_handle, &still_running);
	if (CURLM_OK != res_m) {
		return CM_LBS_ERR;
	}
}

static void __cm_oneospos_loaction_rsp_backup(cm_oneospos_location_rsp_t *location_rsp)
{
	if (CM_ONEOSPOS_EVENT_SUCCESS != location_rsp->event) {
		__cm_oneospos_location_rsp_free(s_rsp_buf);
		s_rsp_buf = NULL;
		return;
	}

	cm_oneospos_location_rsp_t *l_rsp = (cm_oneospos_location_rsp_t *)cm_lbs_malloc(sizeof(cm_oneospos_location_rsp_t));
	if (NULL == l_rsp) {
		return;
	}
	l_rsp->event = location_rsp->event;
	__oneospos_copy_location_rsp(&l_rsp->longitude, location_rsp->longitude);
	__oneospos_copy_location_rsp(&l_rsp->latitude, location_rsp->latitude);
	if (NULL != s_rsp_buf) {
		__cm_oneospos_location_rsp_free(s_rsp_buf);
	}
	s_rsp_buf = l_rsp;
	s_rsp_time = time(NULL);
}

#if 0
static size_t write_callback_curl(char *ptr, size_t size, size_t nmemb, void *rsp_content)
{
	size_t realsize = size * nmemb;
	cm_httpclient_callback_rsp_content_param_t *data = (cm_httpclient_callback_rsp_content_param_t *)rsp_content;

	// Reallocate buffer to fit new data
	uint8_t *temp = realloc(data->response_content, data->current_len + realsize);
	if (temp == NULL) 
    {
		CMLBS_LOGE("oneos curl callback realloc rsp_content NULL");
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
    CMLBS_LOG("oneos curl callback rsp_content info :\ntotal_len = %d, sum_len = %d, current_len = %d", data->total_len, data->sum_len, data->current_len);
	return realsize;
}
#endif


typedef struct curl_rsp_st
{
    uint8_t *data;
    uint32_t data_len;
}curl_rsp_data_t;

static size_t write_callback_curl(void *ptr, size_t size, size_t nmemb, void *rsp_content)
{
	size_t realsize = size * nmemb;

    curl_rsp_data_t *curl_rsp_data = (curl_rsp_data_t *)malloc(sizeof(curl_rsp_data_t));

    curl_rsp_data->data_len = realsize;
    curl_rsp_data->data = (uint8_t *)ptr;

	return realsize;
}

static void cm_lbs_http_callback(CURL *client_handle, cm_httpclient_callback_event_e event, void *param)
{
	static int callback_state = 0; //避免接收HTTP消息重复触发。
	cm_oneospos_attr_t *oneospos_client = s_oneospos_client;

	if (client_handle != s_oneospos_curl_handle) {
		return;
	}

	//CMLBS_LOG("cm_lbs_http_callback:%d,%d",event,(int)param);

	void *cb_arg = oneospos_client->cb_arg;
	switch (event) {
	case CM_HTTP_CALLBACK_EVENT_REQ_START_SUCC_IND: {
		if (CM_LBS_OK != __cm_oneospos_http_send_contant()) {
			////CMLBS_LOGI("send contant fail");
		}
		break;
	}
	case CM_HTTP_CALLBACK_EVENT_ERROR_IND: {
		cm_httpclient_error_event_e err_event = (cm_httpclient_error_event_e)(int)param;
		if (CM_HTTP_EVENT_CODE_CONNECT_TIMEOUT == err_event) {
			oneospos_client->cb(CM_ONEOSPOS_EVENT_TIMEOUT, NULL, cb_arg);
		} else {
			oneospos_client->cb(CM_ONEOSPOS_EVENT_CONERR, NULL, cb_arg);
		}

		/*释放HTTP*/
		__cm_oneospos_http_release(s_oneospos_curl_handle);

		/*释放请求参数*/
		__cm_oneospos_oneospos_client_free(oneospos_client);

		break;
	}
	case CM_HTTP_CALLBACK_EVENT_RSP_HEADER_IND: {
		cm_httpclient_callback_rsp_header_param_t *callback_param = (cm_httpclient_callback_rsp_header_param_t *)param;

		if (200 != callback_param->response_code) {
			////CMLBS_LOGI("http response_code:%d", callback_param->response_code);
			oneospos_client->cb(CM_ONEOSPOS_EVENT_UNKNOW, NULL, cb_arg);

			/*释放HTTP*/
			__cm_oneospos_http_release(s_oneospos_curl_handle);

			/*释放请求参数*/
			__cm_oneospos_oneospos_client_free(oneospos_client);
		}
		break;
	}
	case CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND: {
		if (2 == callback_state) {
			return;
		}
		callback_state = 2;

		cm_httpclient_callback_rsp_content_param_t *callback_param = (cm_httpclient_callback_rsp_content_param_t *)param;
		cm_oneospos_location_rsp_t *location_rsp =
			(cm_oneospos_location_rsp_t *)cm_lbs_malloc(sizeof(cm_oneospos_location_rsp_t));
		if (NULL == location_rsp) {
			oneospos_client->cb(CM_ONEOSPOS_EVENT_UNKNOW, NULL, cb_arg);
		} else {
			cm_oneospos_callback_event_e event_ret =
				__cm_oneospos_http_location_parse(location_rsp, callback_param->response_content);
			location_rsp->event = event_ret;
			oneospos_client->cb(event_ret, location_rsp, cb_arg);
		}

		/*备份定位结果*/
		__cm_oneospos_loaction_rsp_backup(location_rsp);
		/*释放定位结果*/
		__cm_oneospos_location_rsp_free(location_rsp);
		/*释放HTTP*/
		__cm_oneospos_http_release(s_oneospos_curl_handle);
		break;
	}
	case CM_HTTP_CALLBACK_EVENT_RSP_END_IND: {
		/*释放HTTP*/
		__cm_oneospos_http_release(s_oneospos_curl_handle);

		/*释放请求参数*/
		__cm_oneospos_oneospos_client_free(oneospos_client);

		callback_state = 0;
		break;
	}
	default:
		break;
	}
}

/*上一次ueinfo缓存*/
static cm_oneospos_ueinfo_t *s_ue_info_last = NULL;

static cm_lbs_state_e __cm_oneospos_ueinfo_compare(cm_oneospos_ueinfo_t *new_ueinfo)
{
	if (NULL == s_ue_info_last) {
		return CM_LBS_ERR;
	}

	if (NULL == new_ueinfo) {
		return CM_LBS_ERR;
	}

	cm_oneospos_ueinfo_t *old_ueinfo = s_ue_info_last;

	cm_lbs_state_e ret = CM_LBS_ERR;
	do {
		if (0 != strcmp((char *)old_ueinfo->imei, (char *)new_ueinfo->imei)) {
			break;
		}

		if (0 != strcmp((char *)old_ueinfo->imsi, (char *)new_ueinfo->imsi)) {
			break;
		}

		if (0 != strcmp((char *)old_ueinfo->cell_info.lac, (char *)new_ueinfo->cell_info.lac)) {
			break;
		}

		if (0 != strcmp((char *)old_ueinfo->cell_info.cellid, (char *)new_ueinfo->cell_info.cellid)) {
			break;
		}

		if (NULL != old_ueinfo->nearbts && NULL != new_ueinfo->nearbts) {
			if (0 != strcmp((char *)old_ueinfo->nearbts, (char *)new_ueinfo->nearbts)) {
				break;
			}
		}

		if (NULL == old_ueinfo->nearbts && NULL != new_ueinfo->nearbts) {
			break;
		}

		if (NULL != old_ueinfo->nearbts && NULL == new_ueinfo->nearbts) {
			break;
		}

		if (old_ueinfo->cell_info.mcc != new_ueinfo->cell_info.mcc) {
			break;
		}
		if (old_ueinfo->cell_info.mnc != new_ueinfo->cell_info.mnc) {
			break;
		}
		if (old_ueinfo->cell_info.signal != new_ueinfo->cell_info.signal) {
			break;
		}
		ret = CM_LBS_OK;
	} while (0);

	return ret;
}

static cm_oneospos_ueinfo_t *__cm_oneospos_create_ueinfo(cm_oneospos_ueinfo_t *src_ueinfo)
{
	if (NULL == src_ueinfo) {
		return NULL;
	}
	char *nearbts = NULL;
	cm_oneospos_ueinfo_t *ueinfo = (cm_oneospos_ueinfo_t *)cm_lbs_malloc(sizeof(cm_oneospos_ueinfo_t));
	if (NULL == ueinfo) {
		return NULL;
	}
	memcpy(ueinfo, src_ueinfo, sizeof(cm_oneospos_ueinfo_t));

	if (NULL != src_ueinfo->nearbts) {
		int nearbts_len = strlen((char *)src_ueinfo->nearbts);
		if (nearbts_len > 0) {
			nearbts = (char *)cm_lbs_malloc((nearbts_len + 1) * sizeof(char));
			if (NULL != nearbts) {
				strncpy(nearbts, (char *)src_ueinfo->nearbts, nearbts_len);
			}
		}
		ueinfo->nearbts = (uint8_t *)nearbts;
	}

	return ueinfo;
}

static void __cm_oneospos_backup_ueinfo(cm_oneospos_ueinfo_t *new_ueinfo)
{
	if (NULL != s_ue_info_last) {
		cm_oneospos_free_ueinfo(s_ue_info_last);
	}

	cm_oneospos_ueinfo_t *backup_ueinfo = __cm_oneospos_create_ueinfo(new_ueinfo);
	if (backup_ueinfo == NULL) {
		return;
	}
	s_ue_info_last = backup_ueinfo;
}

static cm_lbs_state_e __cm_oneospos_return_last_location(cm_oneospos_attr_t *attr)
{
	cm_lbs_state_e ret = CM_LBS_OK;
	if (NULL == s_rsp_buf) {
		return CM_LBS_ERR;
	}

	/*定位结果有效性     判断*/
	int time_now = time(NULL);
	int time_dif = time_now - s_rsp_time;

	if (time_dif < 0 || time_dif > CM_LBS_ONEOSPOS_LOCATION_RSP_VALID_TIME) {
		////CMLBS_LOGI("Last result failure");
		return CM_LBS_ERR;
	}

	if (NULL != attr->cb) {
		cm_oneospos_location_rsp_t *old_rsp = s_rsp_buf;
		cm_oneospos_location_rsp_t *rsp = (cm_oneospos_location_rsp_t *)cm_lbs_malloc(sizeof(cm_oneospos_location_rsp_t));
		if (NULL == rsp) {
			return CM_LBS_ERR;
		}
		rsp->event = old_rsp->event;

		__oneospos_copy_location_rsp(&rsp->longitude, old_rsp->longitude);
		__oneospos_copy_location_rsp(&rsp->latitude, old_rsp->latitude);

		if (strlen((char *)rsp->longitude) > 6 && strlen((char *)rsp->latitude) > 6) {
			attr->cb(CM_ONEOSPOS_EVENT_SUCCESS, rsp, attr->cb_arg);
		} else {
			ret = CM_LBS_ERR;
		}

		__cm_oneospos_location_rsp_free(rsp);
		return ret;
	} else {
		return CM_LBS_ERR;
	}
}

/*缓存上一次的pid*/
static char *s_oneospos_lastpid = NULL;

static void __cm_oneospos_backup_lastpid(char *last_pid)
{
	if (NULL != last_pid) {
		int last_key_len = strlen(last_pid);
		if (NULL != s_oneospos_lastpid) {
			free(s_oneospos_lastpid);
			s_oneospos_lastpid = NULL;
		}
		s_oneospos_lastpid = (char *)cm_lbs_malloc((last_key_len + 1) * sizeof(char));
		if (NULL == s_oneospos_lastpid) {
			return;
		}
		strncpy(s_oneospos_lastpid, last_pid, last_key_len);
	}
}

static cm_lbs_state_e __cm_oneospos_compare_lastpid(char *last_pid)
{
	if ((NULL == last_pid && NULL != s_oneospos_lastpid) || (NULL != last_pid && NULL == s_oneospos_lastpid)) {
		return CM_LBS_ERR;
	}
	if (NULL != last_pid && NULL != s_oneospos_lastpid) {
		if (0 != strcmp(last_pid, s_oneospos_lastpid)) {
			return CM_LBS_ERR;
		}
	}
	return CM_LBS_OK;
}

static uint32_t __cm_oneospos_create_http_content(cm_oneospos_attr_t *attr, char *http_content)
{
	uint32_t _C_conten_len = -1;
	if (NULL == http_content || NULL == attr->ueinfo || NULL == attr->pid) {
		//CMLBS_LOG("input param fail!");
		return _C_conten_len;
	}
	cm_oneospos_ueinfo_t *ue_info = attr->ueinfo;
	char *bts = (char *)cm_lbs_malloc(512 * sizeof(char));
	if (NULL == bts) {
		return _C_conten_len;
	}

	if (NULL != ue_info->nearbts && strlen((char *)ue_info->nearbts) > 3) {
		sprintf(bts, "%d,%d,%s,%s,%d|%s", ue_info->cell_info.mcc, ue_info->cell_info.mnc, (char *)ue_info->cell_info.lac,
			(char *)ue_info->cell_info.cellid, ue_info->cell_info.signal, ue_info->nearbts);
	} else {
		sprintf(bts, "%d,%d,%s,%s,%d", ue_info->cell_info.mcc, ue_info->cell_info.mnc, (char *)ue_info->cell_info.lac,
			(char *)ue_info->cell_info.cellid, ue_info->cell_info.signal);
	}

	//  sprintf(http_content, "{\"pid\":\"%s\",\"cellList\":\"%s\",\"batchId\":\"%s\"}", attr->pid, bts, ue_info->imei);

	strcat(http_content, "{\"pid\":\"");
	strcat(http_content, attr->pid);
	strcat(http_content, "\",");

	strcat(http_content, "\"cellList\":\"");
	strcat(http_content, bts);
	strcat(http_content, "\",");

	strcat(http_content, "\"batchId\":\"");
	strcat(http_content, (char *)ue_info->imei);
	strcat(http_content, "\"}");

	free(bts);
	bts = NULL;

#if ONEOSPOS_DEBUG
	//CMLBS_LOG("%s", http_content);
	//CMLBS_LOG("%s", http_content + 64);
	//CMLBS_LOG("%s", http_content + 128);
	//CMLBS_LOG("%s", http_content + 192);
#endif
	_C_conten_len = (uint32_t)strlen(http_content);

	return _C_conten_len;
}

static cm_lbs_state_e __cm_oneospos_http_post(cm_oneospos_attr_t *attr, char *http_content)
{
    curl_rsp_data_t *rsp_data = (curl_rsp_data_t *)malloc(sizeof(curl_rsp_data_t));
	cm_lbs_state_e ret = CM_LBS_OK;
	uint32_t index = -1;

	do {
		CURLcode res;
		/*初始化oneos curl handle and client*/
		if (NULL == s_oneospos_curl_handle && NULL == s_curl_multi_handle) {
			s_oneospos_curl_handle = curl_easy_init();
			s_curl_multi_handle = curl_multi_init();
			if (NULL == s_oneospos_curl_handle && NULL == s_curl_multi_handle) {
				return CM_LBS_ERR;
			}
		}
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_URL, (char *)ONEOSPOS_HTTP_HOST);
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_SSL_VERIFYPEER, 1L); //验证证书 true：验证；false：不验证
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_SSL_VERIFYHOST, 1L); //验证主机 true：验证；false：不验证

		/*添加HTTP头 */
		char *head = "Content-Type:application/json";
		struct curl_slist *slist = NULL;
		char *header_str = (char *)head;
		int header_len = strlen(head) + 1;
		header_str[header_len] = '\0';

		slist = curl_slist_append(slist, (const char *)header_str);
		if (NULL == slist) {
			return CM_LBS_ERR;
		}
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_HTTPHEADER, slist);
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_WRITEFUNCTION, write_callback_curl);
		curl_easy_setopt(s_oneospos_curl_handle, CURLOPT_WRITEDATA, (void *)rsp_data);

		CURLMcode res_m = curl_multi_add_handle(s_curl_multi_handle, s_oneospos_curl_handle);
		if (CURLM_OK != res_m) {
			return CM_LBS_ERR;
		}
		int still_running = 0;
		res = curl_multi_perform(s_curl_multi_handle, &still_running);
		if (CURLM_OK != res) {
			return CM_LBS_ERR;
		}

        cm_httpclient_callback_rsp_content_param_t rsp_content = {0};
        rsp_content.current_len = rsp_data->data_len;
        if(rsp_content.total_len == 0)
        {
            rsp_content.total_len = rsp_content.current_len;
        }
        rsp_content.sum_len += rsp_data->data_len;
        rsp_content.response_content = (uint8_t*)rsp_data->data;
        
		cm_lbs_http_callback(s_oneospos_curl_handle, CM_HTTP_CALLBACK_EVENT_RSP_CONTENT_IND, (void *)&rsp_content);

	} while (0);
	return ret;
}

static cm_lbs_state_e cm_oneospos_asynlocation(cm_oneospos_attr_t *attr)
{
	cm_lbs_state_e ret = CM_LBS_OK;
	char *http_content = NULL;
	do {
		http_content = (char *)cm_lbs_malloc(ONEOSPOS_HTTP_CONTENT_LEN * sizeof(char));
		memset(http_content, '\0', ONEOSPOS_HTTP_CONTENT_LEN);
		if (NULL == http_content) {
			ret = CM_LBS_MEMROYERR;
			break;
		}

		uint32_t l_content_len = __cm_oneospos_create_http_content(attr, http_content);
		if (l_content_len < 1) {
			ret = CM_LBS_REPEERR;
			break;
		}

		s_http_content = http_content;
		s_http_conten_len = l_content_len;

		ret = __cm_oneospos_http_post(attr, http_content);
	} while (0);

	if (CM_LBS_OK != ret) {
		__cm_oneospos_http_release(s_oneospos_curl_handle);
		/*若HTTP 请求成功 http_content 在http回调中释放*/
		free(http_content);
		http_content = NULL;
	}
	return ret;
}

void cm_oneospos_create_attr(cm_oneospos_attr_t *src_attr, cm_oneospos_attr_t **dec_attr)
{
	cm_oneospos_attr_t *attr = (cm_oneospos_attr_t *)cm_lbs_malloc(sizeof(cm_oneospos_attr_t));
	if (NULL == attr) {
		*dec_attr = NULL;
		return;
	}
	memcpy(attr, src_attr, sizeof(cm_oneospos_attr_t));
	attr->ueinfo = __cm_oneospos_create_ueinfo(src_attr->ueinfo);

	int pid_len = strlen(src_attr->pid);
	char *internal_pid = (char *)cm_lbs_malloc((pid_len + 1) * sizeof(char));

	if (NULL == internal_pid) {
		free(attr);
		*dec_attr = NULL;
		return;
	}
	strncpy(internal_pid, src_attr->pid, pid_len);
	attr->pid = internal_pid;

	*dec_attr = attr;
}

int cm_oneospos_location(cm_oneospos_attr_t *attr)
{
	if (NULL == attr) {
		return CM_LBS_PARAMERR;
	}
	if (NULL == attr->ueinfo || NULL == attr->cb) {
		return CM_LBS_PARAMERR;
	}

	if (ONEOSPOS_STATE_BUSY == s_oneospos_state) {
		return CM_LBS_BUSY;
	}

	s_oneospos_state = ONEOSPOS_STATE_BUSY;
#if ONEOSPOS_REPEAT_REQUEST_FILTE
	/*如果是重复请求且pid没有变化 直接返回上一次定位结果*/
	if (CM_LBS_OK == __cm_oneospos_ueinfo_compare(attr->ueinfo) && CM_LBS_OK == __cm_oneospos_compare_lastpid(attr->pid)) {
		if (CM_LBS_OK == __cm_oneospos_return_last_location(attr)) {
			//CMLBS_LOG("return last rsp");
			s_oneospos_state = ONEOSPOS_STATE_FREE;
			return CM_LBS_OK;
		}
	}
#endif

	cm_oneospos_attr_t *internal_atrr = NULL;
	cm_oneospos_create_attr(attr, &internal_atrr);
	if (NULL == internal_atrr) {
		s_oneospos_state = ONEOSPOS_STATE_FREE;
		return CM_LBS_MEMROYERR;
	}
	s_oneospos_client = internal_atrr;

	/*备份pid*/
	__cm_oneospos_backup_lastpid(attr->pid);

	/*备份UEINFO*/
	__cm_oneospos_backup_ueinfo(attr->ueinfo);

	cm_lbs_state_e ret = cm_oneospos_asynlocation(attr);
	if (CM_LBS_OK != ret) {
		s_oneospos_state = ONEOSPOS_STATE_FREE;
		__cm_oneospos_oneospos_client_free(internal_atrr);
	}
	return ret;
}

int cm_oneospos_get_ueinfo(cm_oneospos_ueinfo_t **ue_info, int nearbts_enable)
{
	cm_lbs_state_e ret = CM_LBS_OK;
	cm_oneospos_ueinfo_t *s_ue_info = (cm_oneospos_ueinfo_t *)cm_lbs_malloc(sizeof(cm_oneospos_ueinfo_t));

	do {
		if (NULL == s_ue_info) {
			ret = CM_LBS_MEMROYERR;
			break;
		}

#if 0 
        ret = __cm_lbs_GetImei(s_ue_info->imei);
        if(CM_LBS_OK != ret)
        {
            break;
        }

        ret = __cm_lbs_GetImsi(s_ue_info->imsi);
        if(CM_LBS_OK != ret)
        {
            break;
        }


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

        ret = __cm_lbs_GetCell_info(&s_ue_info->cell_info);
        if(CM_LBS_OK != ret)
        {
            break;
        }
#endif

#if CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT
		if (nearbts_enable) {
			uint8_t *nearbts = (uint8_t *)cm_lbs_malloc(1024 * sizeof(uint8_t));
			cm_lbs_state_e op_ret = __cm_lbs_GetNearbts(nearbts);
			if (CM_LBS_OK != op_ret) {
				s_ue_info->nearbts = NULL;
				free(nearbts);
				////CMLBS_LOGI("get Nearbts fail!");
			} else {
				s_ue_info->nearbts = nearbts;
			}
		}

#endif /*CM_LBS_PLATFORM_UEINFO_NEARBETS_SUPPORT*/

	} while (0);

	*ue_info = s_ue_info;
	return ret;
}

void cm_oneospos_free_ueinfo(cm_oneospos_ueinfo_t *ue_info)
{
	if (NULL != ue_info) {
		if (NULL != ue_info->nearbts) {
			free(ue_info->nearbts);
			ue_info->nearbts = NULL;
		}
		free(ue_info);
		ue_info = NULL;
	}
}
