#include "cm_lbs_api.h"

#define CM_LBS_PLAT_AMAP10_SUPPORT  CM_LBS_AMAPV1_SUPPORT                  /*高德1.0平台是否支持*/
#define CM_LBS_PLAT_AMAP20_SUPPORT  CM_LBS_AMAPV1_SUPPORT                  /*高德2.0平台是否支持*/
#define CM_LBS_PLAT_MANTU10_SUPPORT (0)                                    /*漫图定位平台是否支持*/
#define CM_LBS_PLAT_BAIDU10_SUPPORT (0)                                    /*百度定位平台是否支持*/
#define CM_LBS_PLAT_ONEOSPOS_SUPPORT (1)                                   /*oneospos平台是否支持*/

#define cm_lbs_isdigit(c) ((c)>='0'&&(c)<='9')

/*定位结果返回状态码*/
typedef enum
{
    LBS_ATCMD_STATE_SUCCED = 100,
    LBS_ATCMD_STATE_TIMEOUT = 120,
    LBS_ATCMD_STATE_KEYERR = 121,
    LBS_ATCMD_STATE_PARAERR = 122,
    LBS_ATCMD_STATE_OVERDAYQUTA = 123,
    LBS_ATCMD_STATE_OVERQPS = 124,
    LBS_ATCMD_STATE_OVERQUTA = 125,
    LBS_ATCMD_STATE_UNKNOWNERR = 126
} cm_lbs_atcmd_statecode_e;



/*配置参数枚举选项*/
typedef enum
{
    LBS_LBS_ATCMD_NULL_OP = 0,
    LBS_ATCMD_METHOD_OP = 1,
    LBS_ATCMD_APIKEY_OP,
    LBS_ATCMD_PRECISION,
    LBS_ATCMD_DESCR_MODE,
    LBS_ATCMD_SIGN_ENABLE,
    LBS_ATCMD_SIGN_KEY,
    LBS_ATCMD_PID,
    LBS_ATCMD_NEARBTS_ENABLE,
    LBS_ATCMD_ONEOS_INTERNAL_PID,
    LBS_ATCMD_ONEOS_CHANGE_COORDINATE
} cm_lbs_atcmd_option_e;

/*LBS AT命令配置*/
typedef struct
{
    cm_lbs_location_platform_e platform;
} cm_lbs_atcmd_config_t;



/*高德平台配置*/
typedef struct
{
    uint8_t apikey[CM_LBS_AMAP_APIKET_LEN];
    uint8_t signature_key[CM_LBS_AMAP_SIGNATUREKEY_LEN];
    int signal_enable;
    int precision;
    int descr_mode;
    int nearbts_enable;
} cm_lbs_atcmd_amap_config_t;

/*oneospos 平台配置*/
typedef struct
{
    uint8_t pid[CM_LBS_ONEOSPOS_PID_LEN];
    int precision;
    int nearbts_enable;
    int coordinate_enable;
} cm_lbs_atcmd_oneospos_config_t;



/*配置参数与选项的映射*/
typedef struct
{
    cm_lbs_atcmd_option_e option;
    char *sub_cmd;
} cm_lbs_atcmd_option_info_t;

typedef enum
{
    LBS_ATCMD_NVCFG_ONEOSPOSPID = 1,
    LBS_ATCMD_NVCFG_AMAPV1KEY,
    LBS_ATCMD_NVCFG_AMAPV1SIGNA,
    LBS_ATCMD_NVCFG_AMAPV2KEY,
    LBS_ATCMD_NVCFG_AMAPV2SIGNA,
    LBS_ATCMD_NVCFG_AMAPV1SIGNA_EN,
    LBS_ATCMD_NVCFG_AMAPV2SIGNA_EN,
    LBS_ATCMD_NVCFG_CLEARNALL,
    LBS_ATCMD_NVCFG_NULL
} cm_lbs_atcmd_nvcfg_type_t;

typedef struct
{
    cm_lbs_atcmd_nvcfg_type_t type;
    char *subcmd;
} cm_lbs_atcmd_nvcfg_op_t;




/*at 输入配置与选项的映射*/
static cm_lbs_atcmd_option_info_t option_info[] =
{
    {LBS_ATCMD_METHOD_OP, "method"},
    {LBS_ATCMD_APIKEY_OP, "apikey"},
    {LBS_ATCMD_APIKEY_OP, "key"},
    {LBS_ATCMD_PRECISION, "precision"},
    {LBS_ATCMD_DESCR_MODE, "format"},
    {LBS_ATCMD_SIGN_ENABLE, "signen"},
    {LBS_ATCMD_SIGN_KEY, "signkey"},
    {LBS_ATCMD_PID, "pid"},
    {LBS_ATCMD_NEARBTS_ENABLE, "nearbtsen"},
    {LBS_ATCMD_ONEOS_INTERNAL_PID, "oneosinternalpid"},
    {LBS_ATCMD_ONEOS_CHANGE_COORDINATE, "oneoscoordinate"}
};

static cm_lbs_atcmd_nvcfg_op_t nvcfg_option_info[] =
{
    {LBS_ATCMD_NVCFG_ONEOSPOSPID, "oneospospid"},
    {LBS_ATCMD_NVCFG_AMAPV1KEY, "amapv1key"},
    {LBS_ATCMD_NVCFG_AMAPV1SIGNA, "amapv1signa"},
    {LBS_ATCMD_NVCFG_AMAPV2KEY, "amapv2key"},
    {LBS_ATCMD_NVCFG_AMAPV2SIGNA, "amapv2signa"},
    {LBS_ATCMD_NVCFG_AMAPV1SIGNA_EN, "amapv1sinaen"},
    {LBS_ATCMD_NVCFG_AMAPV2SIGNA_EN, "amapv2sinaen"},
    {LBS_ATCMD_NVCFG_CLEARNALL, "clearnall"}
};


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
void *cm_lbs_malloc(uint32_t size)
{
    void *mem = NULL;

    if(size)
    {
        mem = malloc(size);

        if(mem == NULL)
        {
            return NULL;
        }

        memset(mem, 0, size);
    }

    return mem;
}

/**
 * @brief LBS释放内存空间
 *
 * @param [in] mem     分配的内存地址
 *
 * @return
 *
 * @details 仅用于LBS free接口
 */
void cm_lbs_free(void *mem)
{
    if(mem != NULL)
    {
        free(mem);
    }
}

/*
*@brief 将二进制数据转换为十六进制字符串
*
*@param[in] bin_ptr 指向待转换的二进制数据的指针
*@param[out] hex_ptr 指向存储转换后的十六进制字符串的缓冲区的指针
*@param[in] length 要转换的二进制数据的字节长度
*
*@note 该函数遍历二进制数据，每次处理一个字节。
	   对于每个字节，它首先提取高4位（即半字节或尼布尔），
	   将其转换为对应的	   十六进制字符（'0'-'9'或'A'-'F'），然后存储到输出缓冲区。
	   接着提取低4位，同样转换为十六进制字符并存储。每个输入字节都被转换为两个十六进制字符。
*/
static void __cm_lbs_atcmd_data_to_hex(char *hex_ptr, char *bin_ptr, int length)
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

/*
*@brief 计算输入字符串的MD5哈希值，并将结果转换为十六进制字符串
*
*@param[in] input 输入的字符串，用于计算MD5哈希值
*@param[out] output 存储MD5哈希值的十六进制字符串的缓冲区
*
*@note 该函数使用mbedtls库来计算MD5哈希值，并将结果通过__cm_lbs_atcmd_data_to_hex函数转换为十六进制表示的字符串。
*/
static void cm_lbs_atcmd_md5(char *input, char *output)
{
    int len = strlen(input);
    mbedtls_md5_context  md5;

    mbedtls_md5_init(&md5);
    mbedtls_md5_starts(&md5);
    mbedtls_md5_update(&md5, (void *)input, len);
    mbedtls_md5_finish(&md5, (void *)input);

    __cm_lbs_atcmd_data_to_hex(output, input, 16);
}

/*
*@brief 判断字符串内是否全为数字
*
*@param[in] str 需要判断的字符串
*
*@return CM_LBS_OK：全为数字       其他：不全为数字
*/
static cm_lbs_state_e cm_lbs_atcmd_string_is_digital(char *str)
{
    int len = strlen(str);
    int i = 0;
    for(i = 0; i < len; i++)
    {
        if(!cm_lbs_isdigit(str[i]))
        {
            return CM_LBS_ERR;
        }
    }
    return CM_LBS_OK;
}

/*
*@brief 为数字字符串中小数部分指定长度，原始数据位数不足时在末尾补0
*
*@param[in] src_str 原始字符串
*@param[in] out_str 转换后的字符串
*@param[in] count 需要指定的小数位数
*/
static void cm_lbs_atcmd_string_assign_decimal(char *src_str, char *out_str, int count)
{
    if(NULL == src_str || NULL == out_str || count > 8)
    {
        return ;
    }

    int srclen = strlen(src_str);
    memcpy(out_str, src_str, srclen);

    int i = 0;
    char *p_doc = NULL;
    for(i = 0; i < srclen; i++)
    {
        if('.' == out_str[i])
        {
            p_doc = (out_str + i);
            break;
        }
    }
    int decimal_len = srclen - i - 1;

    if(decimal_len == CM_LBS_PRECISION_MAX)
    {
        CMLBS_LOGE("decimal_len long");
        return ;
    }

    if(decimal_len > count)
    {
        p_doc[count + 1] = '\0';
    }

    if(decimal_len < count)
    {
        int patch_len = count - decimal_len;
        if(patch_len > CM_LBS_PRECISION_MAX)
        {
            CMLBS_LOGI("patch_len long:%d", patch_len);
            return;
        }

        while(patch_len--)
        {
            strcat(out_str, "0");
        }
    }
}

/*
*@brief 创建文件夹
*
*@param[in] path 创建文件夹路径
*
*@return 0 表示成功，-1 表示失败
*/
int create_directory_path(const char *path) 
{
    char temp_path[256];
    char *p = NULL;
    size_t len;
 
    snprintf(temp_path, sizeof(temp_path),"%s",path);
    len = strlen(temp_path);
 
    // Iterate through the path and create directories if they don't exist
    if(temp_path[len - 1] == '/') 
    {
        temp_path[len - 1] = 0; // Remove trailing '/'
    }
    for(p = temp_path + 1; *p; p++) 
    {
        if(*p == '/') 
        {
            *p = 0;
            if(access(temp_path, F_OK) != 0) 
            {
                if(mkdir(temp_path, 0755) != 0) 
                {
                    CMLBS_LOGE("mkdir %s failed", temp_path);
                    return -1;
                }
            }
            *p = '/';
        }
    }
 
    // Create the final directory if it doesn't exist
    if(access(temp_path, F_OK) != 0) 
    {
        if(mkdir(temp_path, 0755) != 0) 
        {
            CMLBS_LOGE("mkdir %s failed", temp_path);
            return -1;
        }
    }
    return 0;
}


/**
 * @brief 写配置文件
 *
 * @param [in] filename                 文件名
 * @param [in] data                     数据
 * @param [in] datalen                  数据长度
 *
 * @return <0 失败；其他 实际写入长度
 *
 * @details 用于保存配置参数
 */
int cm_nv_write(const char *filename, char *data, uint32_t datalen)
{
    CMLBS_LOG("");

    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        return -1;
    }

    if(access(CM_LBS_CONFIG_FILE_PATH, F_OK) != 0)
    {
        CMLBS_LOG("%s not exist, create it", CM_LBS_CONFIG_FILE_PATH);
        if(create_directory_path(CM_LBS_CONFIG_FILE_PATH) < 0)
        {
            CMLBS_LOGE("create path  %s failed", CM_LBS_CONFIG_FILE_PATH);
            return -1;
        }
    }

    FILE *fd = fopen(filename, "wb+");
    if (fd == NULL)
    {
        CMLBS_LOGE("open %s fail", filename);
        return -1;
    }

    int ret = fwrite(data, datalen, 1, fd);
    if(ret != 1)
    {
        CMLBS_LOGE("cm_nv write error");
        ret = -1;
    }
    else
    {
        ret = datalen;
    }
    fclose(fd);
    return ret;
}

/**
 * @brief 读配置文件
 *
 * @param [in] filename                 文件名
 * @param [in] data                     缓存
 * @param [in] datalen                  缓存长度
 *
 * @return <=0 失败；其他 实际读取长度
 *
 * @details 用于读取配置参数
 */
int cm_nv_read(const char *filename, char *data, uint32_t datalen)
{
    CMLBS_LOG("");
    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        return -1;
    }

    if(access(CM_LBS_CONFIG_FILE_PATH, F_OK) != 0)
    {
        CMLBS_LOG("%s not exist, create it", CM_LBS_CONFIG_FILE_PATH);
        if(create_directory_path(CM_LBS_CONFIG_FILE_PATH) < 0)
        {
            CMLBS_LOGE("create path  %s failed", CM_LBS_CONFIG_FILE_PATH);
            return -1;
        }
    }

    FILE *fd = fopen(filename, "r");
    if (fd == NULL)
    {
        CMLBS_LOGE("open %s fail", filename);
        return -1;
    }

    int ret = fread(data, datalen, 1, fd);
    if(ret != 1)
    {
        CMLBS_LOGE("nv_read fread error");
        return -1;
    }
    else
    {
        ret = ret;
    }
    CMLBS_LOGI("nv fread %d", ret);
    fclose(fd);
    return ret;
}

/*
*@brief 写入文件(必选)
*
*@param[in] file_path 文件路径
*@param[in] file_data 需要写入数据
*@param[in] size  需要写入数据长度
*
*@return 成功:CM_LBS_OK 失败：CM_LBS_ERR
*/
cm_lbs_state_e __cm_lbs_FileWrite(const char *file_path, void *file_data, uint32_t size)
{
    // if(access(file_path, F_OK) != 0)
    // {
    //     CMLBS_LOG("%s not exist, create it", CM_LBS_CONFIG_FILE_PATH);
    //     if(create_directory_path(CM_LBS_CONFIG_FILE_PATH) < 0)
    //     {
    //         CMLBS_LOGE("create path  %s failed", CM_LBS_CONFIG_FILE_PATH);
    //         return CM_LBS_ERR;
    //     }
    // }

#if 1
    cm_lbs_state_e ret = CM_LBS_OK;

    int fd = cm_nv_write(file_path, (char *)file_data, size);
    if(fd < 0)
    {
        CMLBS_LOGE("error:%d fd = %d", errno, fd);
        ret = CM_LBS_ERR;
    }
    return ret;
#else
    cm_lbs_state_e ret = CM_LBS_OK;
    if(file_path == NULL || file_data == NULL || size == 0 || strlen(file_path) > 64)
    {
        CMLBS_LOGE("error:file_path or file_data is null");
        return CM_LBS_ERR;
    }

    FILE *fd = fopen(file_path, "wb+");
    if(fd == NULL)
    {
        CMLBS_LOGE("fopen %s fail", file_path);
        return CM_LBS_ERR;
    }

    int res = fwrite(file_data, size, 1, fd);
    if(res != 1)
    {
        CMLBS_LOGE("fwrite to %s error", file_path);
        return CM_LBS_ERR;
    }
    else
    {
        ret = size;
    }
    
    fclose(fd);
    return ret;
#endif
}

/*
*@brief 读取文件(必选)
*
*@param[in] file_path 文件路径
*@param[in] file_data 读取缓存区
*@param[in] size  读取长度
*
*@return 成功:CM_LBS_OK 失败：CM_LBS_ERR
*/
cm_lbs_state_e __cm_lbs_FileRead(const char *file_path, void *file_rdata, uint32_t read_size)
{
    // if(access(file_path, F_OK) != 0)
    // {
    //     CMLBS_LOG("%s not exist, create it", CM_LBS_CONFIG_FILE_PATH);
    //     if(create_directory_path(CM_LBS_CONFIG_FILE_PATH) < 0)
    //     {
    //         CMLBS_LOGE("create path  %s failed", CM_LBS_CONFIG_FILE_PATH);
    //         return CM_LBS_ERR;
    //     }
    // }

#if 1
    cm_lbs_state_e ret = CM_LBS_OK;

    int fd = cm_nv_read(file_path, (char *)file_rdata, read_size);
    if(fd <= 0)//读到0个数据，说明文件不存在或权限不足
    {
        CMLBS_LOG("error:%d fd = %d", errno, fd);
        ret = CM_LBS_ERR;
    }
    return ret;
#else
    cm_lbs_state_e ret = CM_LBS_OK;

    if(file_path == NULL || file_rdata == NULL || read_size == 0 ||strlen(file_path) > 64)
    {
        CMLBS_LOGE("error:file_path or file_data is null");
        return CM_LBS_ERR;
    }

    FILE *fd = fopen(file_path, "r");
    if(fd  == NULL)
    {
        CMLBS_LOGE("fopen %s fail", file_path);
        return CM_LBS_ERR;
    }
    int res = fread(file_rdata, read_size, 1, fd);
    if(res != 1)
    {
        CMLBS_LOGE("fread from %s fail", file_path);
        return CM_LBS_ERR;
    }
    else
    {
        ret = res;
    }
    fclose(fd);
    return ret;
#endif
}

/*
*@brief 根据AT+MLBSCFG输入的参数找到配置选项
*
*@param[in] subcmd 输入的配置参数
*
*return 找到的配置选项(详见cm_lbs_atcmd_option_e)
*/
static cm_lbs_atcmd_option_e cm_lbs_atcmd_subcmd_to_option(char *subcmd)
{
    cm_lbs_atcmd_option_e ret = LBS_LBS_ATCMD_NULL_OP;

    int count = sizeof(option_info) / sizeof(cm_lbs_atcmd_option_info_t);
    int i = 0;

    for(i = 0; i < count; i++)
    {
        if(!strcmp(subcmd, option_info[i].sub_cmd))
        {
            ret = option_info[i].option;
            break;
        }
    }
    return ret;
}

static cm_lbs_state_e cm_lbs_atcmd_read_atcmdcfg(cm_lbs_atcmd_config_t *atcmd_cfg)
{
    if(NULL == atcmd_cfg)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_OK;

    ret = __cm_lbs_FileRead(CM_LBS_ATCMD_CONFIG_FILE_PATH, (void *)atcmd_cfg, sizeof(cm_lbs_atcmd_config_t));
    return ret;
}

static cm_lbs_state_e cm_lbs_atcmd_write_atcmdcfg(cm_lbs_atcmd_config_t *atcmd_cfg)
{
    cm_lbs_state_e ret = CM_LBS_OK;
    ret = __cm_lbs_FileWrite(CM_LBS_ATCMD_CONFIG_FILE_PATH, (void *)atcmd_cfg, sizeof(cm_lbs_atcmd_config_t));
    return ret;
}

static cm_lbs_state_e cm_lbs_atcmd_write_amapcfg(cm_lbs_location_platform_e platform, cm_lbs_atcmd_amap_config_t *amap_cfg)
{
    if(NULL == amap_cfg)
    {
        return CM_LBS_ERR;
    }

    char *path = (platform == CM_LBS_PLAT_AMAP10) ? CM_LBS_ATCMD_AMAPV10_FILE_PATH : CM_LBS_ATCMD_AMAPV20_FILE_PATH ;

    cm_lbs_state_e ret = CM_LBS_OK;
    ret = __cm_lbs_FileWrite(path, (void *)amap_cfg, sizeof(cm_lbs_atcmd_amap_config_t));
    return ret;
}

static cm_lbs_state_e cm_lbs_atcmd_read_amapcfg(cm_lbs_location_platform_e platform, cm_lbs_atcmd_amap_config_t *amap_cfg)
{
    if(NULL == amap_cfg)
    {
        return CM_LBS_ERR;
    }
    char *path = (platform == CM_LBS_PLAT_AMAP10) ? CM_LBS_ATCMD_AMAPV10_FILE_PATH : CM_LBS_ATCMD_AMAPV20_FILE_PATH ;

    cm_lbs_state_e ret = CM_LBS_OK;
    ret = __cm_lbs_FileRead(path, (void *)amap_cfg, sizeof(cm_lbs_atcmd_amap_config_t));
    return ret;
}


static cm_lbs_state_e cm_lbs_atcmd_write_oneosposcfg(cm_lbs_location_platform_e platform, cm_lbs_atcmd_oneospos_config_t *oneospos_cfg)
{
    (void)platform;
    if(NULL == oneospos_cfg)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_OK;
    ret = __cm_lbs_FileWrite(CM_LBS_ATCMD_ONEOSPOS_FILE_PATH, (void *)oneospos_cfg, sizeof(cm_lbs_atcmd_oneospos_config_t));
    return ret;
}

static cm_lbs_state_e cm_lbs_atcmd_Read_oneosposcfg(cm_lbs_location_platform_e platform, cm_lbs_atcmd_oneospos_config_t *oneospos_cfg)
{
    (void)platform;
    if(NULL == oneospos_cfg)
    {
        return CM_LBS_ERR;
    }
    cm_lbs_state_e ret = CM_LBS_OK;
    ret = __cm_lbs_FileRead(CM_LBS_ATCMD_ONEOSPOS_FILE_PATH, (void *)oneospos_cfg, sizeof(cm_lbs_atcmd_oneospos_config_t));
    return ret;
}




static cm_lbs_location_platform_e cm_lbs_atcmd_getplatform(void)
{
    cm_lbs_location_platform_e ret = CM_LBS_METHOD_DEFAULT;

    cm_lbs_atcmd_config_t  atcmd_cfg = {0};
    if(CM_LBS_OK == cm_lbs_atcmd_read_atcmdcfg(&atcmd_cfg))
    {
        ret = atcmd_cfg.platform;
    }
    return ret;
}

static cm_lbs_state_e __cm_lbs_atcmd_amap_location_urc_rsp(cm_lbs_location_rsp_t *location, cm_lbs_location_platform_e platform, char *at_urcbuf)
{
    cm_lbs_atcmd_amap_config_t *attr = cm_lbs_malloc(sizeof(cm_lbs_atcmd_amap_config_t));
    if(NULL == attr)
    {
        return  CM_LBS_ERR;

    }

    if(CM_LBS_OK != cm_lbs_atcmd_read_amapcfg(platform, attr))
    {
        attr->descr_mode = CM_LBS_DESCR_MODE_DEFAULT;
        attr->precision = CM_LBS_PRECISION_DEFAULT;
    }
    int descr_mode = attr->descr_mode;

    char latitude[CM_LBS_PRECISION_MAX + 8] = {0};
    char longitude[CM_LBS_PRECISION_MAX + 8] = {0};
    cm_lbs_atcmd_string_assign_decimal(location->latitude, latitude, attr->precision);
    cm_lbs_atcmd_string_assign_decimal(location->longitude, longitude, attr->precision);

    sprintf(at_urcbuf, "\r\n+MLBSLOC: %d,%s,%s,%s", (int)LBS_ATCMD_STATE_SUCCED, longitude, latitude, location->radius);     //880A会自动丢弃+MLBLOC前的\r\n
    // sprintf(at_urcbuf, "+MLBSLOC: %d,%s,%s,%s", (int)LBS_ATCMD_STATE_SUCCED, longitude, latitude, location->radius);
    free(attr);
    if(descr_mode)
    {
        int descr_len = strlen(location->location_describe);
        if(descr_len > (CM_LBS_ATCMD_URCDESCRLEN_MAX - 2))
        {
            CMLBS_LOGI("descr_len long:%d ", descr_len);
            return  CM_LBS_ERR;
        }

        char *location_desrc = (char *)cm_lbs_malloc(CM_LBS_ATCMD_URCDESCRLEN_MAX * sizeof(char));
        if(NULL == location_desrc)
        {
            return  CM_LBS_ERR;
        }
        sprintf(location_desrc, ",\"%s\"", location->location_describe);
        strcat(at_urcbuf, location_desrc);
        free(location_desrc);
    }

    return CM_LBS_OK;
}

static cm_lbs_state_e __cm_lbs_atcmd_oneospos_location_urc_rsp(cm_lbs_location_rsp_t *location, cm_lbs_location_platform_e platform, char *at_urcbuf)
{
    char latitude[CM_LBS_PRECISION_MAX + 8] = {0};
    char longitude[CM_LBS_PRECISION_MAX + 8] = {0};
    int precision_count = 0;

    cm_lbs_atcmd_oneospos_config_t oneospos_attr_cfg = {0};
    if(CM_LBS_OK != cm_lbs_atcmd_Read_oneosposcfg(platform, &oneospos_attr_cfg))
    {
        oneospos_attr_cfg.precision = CM_LBS_PRECISION_DEFAULT;
        oneospos_attr_cfg.coordinate_enable = CM_LBS_ONEOS_COORDINATE;
    }
    precision_count = oneospos_attr_cfg.precision;
    int coordinate = oneospos_attr_cfg.coordinate_enable;

    char s_latitude[24] = {0};
    char s_longitude[24] = {0};

    if(NULL != location->latitude && NULL != location->longitude)
    {
        int la_length = strlen(location->latitude);
        int lon_length =  strlen(location->longitude);
        if(la_length < 24 && lon_length < 24)
        {
            memcpy(s_latitude, location->latitude, la_length);
            memcpy(s_longitude, location->longitude, lon_length);
        }
    }
    if(1 == coordinate)
    {
        double f_latitude = atof(s_latitude) ;
        double f_longitude = atof(s_longitude);
        Wgs84ToGcj02(&f_latitude, &f_longitude);
        
        gcvt(f_latitude,sizeof(s_latitude),s_latitude);
        gcvt(f_longitude,sizeof(s_longitude),s_longitude);
        //sprintf(s_latitude, "%f", f_latitude);
        //sprintf(s_longitude, "%f", f_longitude);
        
    }

    cm_lbs_atcmd_string_assign_decimal(s_latitude, latitude, precision_count);
    cm_lbs_atcmd_string_assign_decimal(s_longitude, longitude, precision_count);

    sprintf(at_urcbuf, "\r\n+MLBSLOC: %d,%s,%s", (int)LBS_ATCMD_STATE_SUCCED, longitude, latitude);

    return CM_LBS_OK;
}
/*
*@brief URC上报
*
*@param[in]   cmd_event
*@param[in] location 定位结果(定位失败时可为空)
*@param[in] atHandle AT句柄
*
*/
static void cm_lbs_atcmd_location_urc_rsp(cm_lbs_atcmd_statecode_e cmd_event, cm_lbs_location_rsp_t *location, void *arg)
{
    char *at_urcbuf = (char *)cm_lbs_malloc(CM_LBS_ATCMD_URCLEN_MAX * sizeof(char));
    if(NULL == at_urcbuf)
    {
        cmd_event = LBS_ATCMD_STATE_UNKNOWNERR;
    }
    memset(at_urcbuf, '\0', CM_LBS_ATCMD_URCLEN_MAX);

    cm_lbs_state_e ret = CM_LBS_OK;

    switch(cmd_event)
    {
        case LBS_ATCMD_STATE_SUCCED:
            {
                cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();
                if(CM_LBS_PLAT_ONEOSPOS == platform)
                {
                    ret =  __cm_lbs_atcmd_oneospos_location_urc_rsp(location, platform, at_urcbuf);
                }
                else
                {
                    ret = __cm_lbs_atcmd_amap_location_urc_rsp(location, platform, at_urcbuf);
                }
                break;
            }

        default :
            {
                sprintf(at_urcbuf, "\r\n+MLBSLOC: %d", (int)cmd_event);
                break;
            }

    }
    if(CM_LBS_OK == ret)
    {
        // strcat(at_urcbuf, "\r\n");       //去掉尾部\r\n
        cm_lbs_urc_rsp(arg, at_urcbuf); //urc上报
    }
    else //URC上报 解析时发生错误
    {
        char lbs_err_urc[64] = {0};
        sprintf(lbs_err_urc, "\r\n+MLBSLOC: %d\r\n", LBS_ATCMD_STATE_UNKNOWNERR);
        cm_lbs_urc_rsp(arg, lbs_err_urc);
    }

    free(at_urcbuf);
}


/*定位回调*/
static void cm_lbs_atcmd_callback(cm_lbs_callback_event_e event, cm_lbs_location_rsp_t *location, void *cb_arg)
{
    CMLBS_LOGI("cm_lbs_atcmd_cb event:%d", event);

    cm_lbs_atcmd_statecode_e atcmd_ret = LBS_ATCMD_STATE_SUCCED;

    switch(event)
    {
        case CM_LBS_LOCATION_OK :
            {
                atcmd_ret = LBS_ATCMD_STATE_SUCCED;
                break;
            }
        case CM_LBS_NET_ERR :
            {
                atcmd_ret = LBS_ATCMD_STATE_UNKNOWNERR;
                break;
            }
        case CM_LBS_TIMEOUT :
            {
                atcmd_ret = LBS_ATCMD_STATE_TIMEOUT;
                break;
            }
        case CM_LBS_KEY_ERR  :
            {
                atcmd_ret = LBS_ATCMD_STATE_KEYERR;
                break;
            }
        case CM_LBS_PARAM_INVALID  :
            {
                atcmd_ret = LBS_ATCMD_STATE_PARAERR;
                break;
            }
        case CM_LBS_OVER_DAYQUTA   :
            {
                atcmd_ret = LBS_ATCMD_STATE_OVERDAYQUTA;
                break;
            }
        case CM_LBS_OVER_QUOTA   :
            {
                atcmd_ret = LBS_ATCMD_STATE_OVERQUTA;
                break;
            }
        case CM_LBS_OVER_QPS   :
            {
                atcmd_ret = LBS_ATCMD_STATE_OVERQPS;
                break;
            }
        case CM_LBS_UNKNOW_ERR    :
            {
                atcmd_ret = LBS_ATCMD_STATE_UNKNOWNERR;
                break;
            }

        default :
            atcmd_ret = LBS_ATCMD_STATE_UNKNOWNERR ;
            break;
    }

    /*URC 上报*/
    cm_lbs_atcmd_location_urc_rsp(atcmd_ret, location, cb_arg);

    /*释放资源*/
    cm_lbs_deinit();
}


static  cm_lbs_state_e __amap_cm_lbs_atcmd_setoption(cm_lbs_atcmd_option_e option, void *value)
{
    // CMLBS_LOG("function:%s", __FUNCTION__);
    cm_lbs_state_e ret = CM_LBS_OK;
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();

    cm_lbs_atcmd_amap_config_t *amap_attr = cm_lbs_malloc(sizeof(cm_lbs_atcmd_amap_config_t));
    if(NULL == amap_attr)
    {
        return CM_LBS_MEMROYERR;
    }

    if(LBS_ATCMD_METHOD_OP != option)
    {
        if(CM_LBS_OK != cm_lbs_atcmd_read_amapcfg(platform, amap_attr))
        {
            amap_attr->precision = CM_LBS_PRECISION_DEFAULT;
            amap_attr->descr_mode = CM_LBS_DESCR_MODE_DEFAULT;
            amap_attr->signal_enable = CM_LBS_SIGNATURE_EN_DEFAULT;
        }
    }

    switch(option)
    {
        case LBS_ATCMD_METHOD_OP:
            {
                cm_lbs_atcmd_config_t *atcmd_cfg = cm_lbs_malloc(sizeof(cm_lbs_atcmd_config_t));
                if(NULL == atcmd_cfg)
                {
                    CMLBS_LOGE("memrory error");
                    ret = CM_LBS_MEMROYERR;
                    break;
                }
                cm_lbs_location_platform_e s_platform = *(cm_lbs_location_platform_e *)value;
                atcmd_cfg->platform = s_platform;

                ret =  cm_lbs_atcmd_write_atcmdcfg(atcmd_cfg);
                if(CM_LBS_OK != ret)
                {
                    CMLBS_LOGI("write atcmdcfg fail:%d", (int)ret);
                }
                free(atcmd_cfg);
                break;
            }
        case LBS_ATCMD_APIKEY_OP:
            {
                // CMLBS_LOG("function:%s LBS_ATCMD_APIKEY_OP ENTRY ", __FUNCTION__);    
                int key_len = strlen((char *)value);                
                CMLBS_LOGI("__amap_cm_lbs_atcmd_setoption apikey_value:%s key_len=%d", (char *)value, key_len);
                if(key_len <= CM_LBS_AMAP_APIKET_LEN)
                {
                    memset(amap_attr->apikey, 0, CM_LBS_AMAP_APIKET_LEN);
                    memcpy(amap_attr->apikey, (uint8_t*)value, key_len);
                    CMLBS_LOGI("function:%s value=%s  attr=%s ",__FUNCTION__, (char *)value, (char *)amap_attr->apikey); 
                }
                else
                {
                    ret = CM_LBS_PARAMERR;
                }
                break;
            }

        case LBS_ATCMD_SIGN_KEY:
            {
                int key_len = strlen((char *)value);
                if(key_len <= CM_LBS_AMAP_SIGNATUREKEY_LEN)
                {
                    memset(amap_attr->signature_key, 0, CM_LBS_AMAP_SIGNATUREKEY_LEN);
                    memcpy(amap_attr->signature_key, (uint8_t*)value, key_len);
                }
                else
                {
                    ret = CM_LBS_PARAMERR;
                }
                break;
            }

        case LBS_ATCMD_PRECISION:
            {
                amap_attr->precision = *(int *)value;
                break;
            }

        case LBS_ATCMD_DESCR_MODE:
            {
                amap_attr->descr_mode = *(int *)value;
                break;
            }
        case LBS_ATCMD_SIGN_ENABLE:
            {
                amap_attr->signal_enable = *(int *)value;
                break;
            }
        case LBS_ATCMD_NEARBTS_ENABLE:
            {
                amap_attr->nearbts_enable = *(int *)value;
                break;
            }
        default :
            ret = CM_LBS_PARAMERR;
            break;
    }

    if(LBS_ATCMD_METHOD_OP != option && CM_LBS_OK == ret)
    {
        if(CM_LBS_PLAT_AMAP10 == platform || CM_LBS_PLAT_AMAP20 == platform)
        {
            if(CM_LBS_PLAT_AMAP10 == platform)
            {
#if CM_LBS_PLAT_AMAP10_SUPPORT
                // memcpy(&amapv10_attr, amap_attr, sizeof(cm_lbs_atcmd_amap_config_t));
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT*/
            }
            if(CM_LBS_PLAT_AMAP20 == platform)
            {
#if CM_LBS_PLAT_AMAP20_SUPPORT
                //  memcpy(&amapv20_attr, amap_attr, sizeof(cm_lbs_atcmd_amap_config_t));
#endif /*CM_LBS_PLAT_AMAP20_SUPPORT*/
            }

            ret = cm_lbs_atcmd_write_amapcfg(platform, amap_attr);
        }
    }
    free(amap_attr);
    return ret;


}

static cm_lbs_state_e __oneospos_cm_lbs_atcmd_setoption(cm_lbs_atcmd_option_e option, void *value)
{
    cm_lbs_state_e ret = CM_LBS_OK;
    cm_lbs_atcmd_oneospos_config_t *oneospos_attr = (cm_lbs_atcmd_oneospos_config_t *)cm_lbs_malloc(sizeof(cm_lbs_atcmd_oneospos_config_t));
    if(NULL == oneospos_attr)
    {
        return CM_LBS_PARAMERR;
    }

    if(CM_LBS_OK != cm_lbs_atcmd_Read_oneosposcfg(CM_LBS_PLAT_ONEOSPOS, oneospos_attr))
    {
        CMLBS_LOGE("read oneoscfg fail");
        oneospos_attr->precision = CM_LBS_PRECISION_DEFAULT;
        oneospos_attr->coordinate_enable=CM_LBS_ONEOS_COORDINATE;
    }
    switch(option)
    {
        case LBS_ATCMD_PID:
            {

                int pid_len = strlen((char *)value);
                if(pid_len <= CM_LBS_ONEOSPOS_PID_LEN)
                {
                    memset(oneospos_attr->pid, 0, CM_LBS_ONEOSPOS_PID_LEN);
                    memcpy(oneospos_attr->pid, (uint8_t*)value, pid_len);
                }
                else
                {
                    ret = CM_LBS_PARAMERR;
                }
                break;
            }
        case LBS_ATCMD_PRECISION:
            {
                oneospos_attr->precision = *(int *)value;
                break;
            }
        case LBS_ATCMD_NEARBTS_ENABLE:
            {
                oneospos_attr->nearbts_enable = *(int *)value;
                break;
            }
        case LBS_ATCMD_ONEOS_CHANGE_COORDINATE:
            {
                oneospos_attr->coordinate_enable = *(int *)value;
                break;
            }

        default :
            ret = CM_LBS_PARAMERR;
            break;
    }

    if(CM_LBS_OK == ret)
    {
        // memcpy(&s_oneospos_attr, oneospos_attr, sizeof(cm_lbs_atcmd_oneospos_config_t));
        ret = cm_lbs_atcmd_write_oneosposcfg(CM_LBS_PLAT_ONEOSPOS, oneospos_attr);
    }

    free(oneospos_attr);
    return ret;
}

cm_lbs_state_e cm_lbs_atcmd_setoption(cm_lbs_atcmd_option_e option, void *value)
{
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();

    if(LBS_ATCMD_METHOD_OP == option)
    {
        return __amap_cm_lbs_atcmd_setoption(option, value);
    }

    if(CM_LBS_PLAT_ONEOSPOS == platform)
    {
        return __oneospos_cm_lbs_atcmd_setoption(option, value);
    }

    /*此函数内有通用设置 只能放在此函数的最后    */
    return __amap_cm_lbs_atcmd_setoption(option, value);
}

void cm_lbs_atcmd_MLBSCFG_test(char *at_send)
{
    if(NULL == at_send)
    {
        return ;
    }
    char method_send[128] = {0};
    char format_send[64] = {0};
    sprintf(method_send, "+MLBSCFG: \"method\",(");
#if CM_LBS_PLAT_AMAP10_SUPPORT
    strcat(method_send, "10");
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT*/

#if CM_LBS_PLAT_AMAP20_SUPPORT
    strcat(method_send, ",11");
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT*/

#if CM_LBS_PLAT_MANTU10_SUPPORT
    strcat(method_send, ",20");
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT*/

#if CM_LBS_PLAT_BAIDU10_SUPPORT
    strcat(method_send, ",30");
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT*/
#if CM_LBS_PLAT_ONEOSPOS_SUPPORT

#if (!CM_LBS_PLAT_AMAP10_SUPPORT&&!CM_LBS_PLAT_AMAP20_SUPPORT)
    strcat(method_send, "40");
#else
    strcat(method_send, ",40");
#endif /*(CM_LBS_PLAT_AMAP10_SUPPORT&&CM_LBS_PLAT_AMAP20_SUPPORT)*/

#endif /*CM_LBS_PLAT_ONEOSPOS_SUPPORT*/
    strcat(method_send, ")\r\n");

#if (CM_LBS_PLAT_AMAP10_SUPPORT&&CM_LBS_PLAT_AMAP20_SUPPORT)
    strcat(method_send, "+MLBSCFG: \"apikey\",<apikey>\r\n+MLBSCFG: \"signen\",(0,1)\r\n+MLBSCFG: \"signkey\",<signkey>\r\n");

    sprintf(format_send, "+MLBSCFG: \"format\",(%d,%d)\r\n", CM_LBS_DESCR_MODE_MIN, CM_LBS_DESCR_MODE_MAX);
#endif /*(CM_LBS_PLAT_AMAP10_SUPPORT&&CM_LBS_PLAT_AMAP20_SUPPORT)*/
    sprintf(at_send, "%s+MLBSCFG: \"precision\",(%d-%d)\r\n%s+MLBSCFG: \"pid\",<pid>\r\n+MLBSCFG: \"nearbtsen\",(0,1)\r\n", method_send, \
            CM_LBS_PRECISION_MIN, CM_LBS_PRECISION_MAX, format_send);

}

cm_lbs_state_e cm_lbs_atcmd_MLBSCFG_set(char *subcmd, void *value)
{
    CMLBS_LOG("Enter cm_lbs_atcmd_MLBSCFG_set");
    cm_lbs_atcmd_option_e op = cm_lbs_atcmd_subcmd_to_option(subcmd);
    if(LBS_LBS_ATCMD_NULL_OP == op || NULL == value)
    {
        CMLBS_LOGE("subcmd and vlaue is null");
        return CM_LBS_PARAMERR;
    }
#if (!CM_LBS_PLAT_AMAP10_SUPPORT&&!CM_LBS_PLAT_AMAP20_SUPPORT)
    if(op < LBS_ATCMD_PID && op != LBS_ATCMD_PRECISION)
    {
        /*非高德平台下，不支持apikey、signkey、signen、format配置*/
        return CM_LBS_PARAMERR;
    }
#endif /*(!CM_LBS_PLAT_AMAP10_SUPPORT&&!CM_LBS_PLAT_AMAP20_SUPPORT)*/

    cm_lbs_state_e ret = CM_LBS_OK;

    switch(op)
    {
        case LBS_ATCMD_METHOD_OP:
            {
                CMLBS_LOGI("LBS_ATCMD_METHOD_OP");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGI("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }
                cm_lbs_location_platform_e platform = atoi((char *)value);
                if(!(CM_LBS_PLAT_AMAP10 == platform || CM_LBS_PLAT_AMAP20 == platform || CM_LBS_PLAT_ONEOSPOS == platform))
                {
                    CMLBS_LOGE("platform value(%d) error", platform);
                    return CM_LBS_PARAMERR;
                }
                ret = cm_lbs_atcmd_setoption(op, (void *)&platform);
                break;
            }

        case LBS_ATCMD_APIKEY_OP:
            {
                CMLBS_LOGI("LBS_ATCMD_APIKEY_OP");
                ret = cm_lbs_atcmd_setoption(op, value);
                break;
            }
        case LBS_ATCMD_PRECISION:
            {
                CMLBS_LOGI("LBS_ATCMD_PRECISION");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGE("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }

                int precision = atoi(value);
                if(precision < CM_LBS_PRECISION_MIN || precision > CM_LBS_PRECISION_MAX)
                {
                    CMLBS_LOGE("precision value(%s) error", (char)value);
                    return CM_LBS_PARAMERR;
                }

                ret = cm_lbs_atcmd_setoption(op, (void *)&precision);
                break;
            }
        case LBS_ATCMD_DESCR_MODE:
            {
                CMLBS_LOGI("LBS_ATCMD_DESCR_MODE");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGE("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }
                int descr_mode = atoi(value);

                if(descr_mode < CM_LBS_DESCR_MODE_MIN || descr_mode > CM_LBS_DESCR_MODE_MAX)
                {
                    CMLBS_LOGE("descr_mode value(%s) error", (char)value);
                    return CM_LBS_PARAMERR;
                }
                ret = cm_lbs_atcmd_setoption(op, (void *)&descr_mode);
                break;
            }

        case LBS_ATCMD_SIGN_ENABLE:
            {
                CMLBS_LOGI("LBS_ATCMD_SIGN_ENABLE");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGE("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }
                int sign_en = atoi(value);

                if(sign_en < 0 || sign_en > 1)
                {
                    CMLBS_LOGE("sign_en value(%s) error", (char)value);
                    return CM_LBS_PARAMERR;
                }
                ret = cm_lbs_atcmd_setoption(op, (void *)&sign_en);
                break;
            }
        case LBS_ATCMD_SIGN_KEY:
            {
                CMLBS_LOGI("LBS_ATCMD_SIGN_KEY");
                ret = cm_lbs_atcmd_setoption(op, value);
                break;
            }

        case LBS_ATCMD_PID:
            {
                CMLBS_LOGI("LBS_ATCMD_PID");
                ret = cm_lbs_atcmd_setoption(op, value);
                break;
            }
        case LBS_ATCMD_NEARBTS_ENABLE:
            {
                CMLBS_LOGI("LBS_ATCMD_NEARBTS_ENABLE");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGE("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }
                int nearbts_en = atoi(value);

                if(nearbts_en < 0 || nearbts_en > 1)
                {
                    return CM_LBS_PARAMERR;
                }
                ret = cm_lbs_atcmd_setoption(op, (void *)&nearbts_en);
                break;
            }
        case LBS_ATCMD_ONEOS_CHANGE_COORDINATE:
            {
                CMLBS_LOGI("LBS_ATCMD_ONEOS_CHANGE_COORDINATE");
                if(CM_LBS_OK != cm_lbs_atcmd_string_is_digital((char *)value))
                {
                    CMLBS_LOGE("value(%s) is not al digtal", (char)value);
                    return CM_LBS_PARAMERR;
                }
                int coordinate = atoi(value);

                if(coordinate < 0 || coordinate > 1)
                {
                    CMLBS_LOGE("coordinate value(%s) error", (char)value);
                    return CM_LBS_PARAMERR;
                }
                ret = cm_lbs_atcmd_setoption(op, (void *)&coordinate);
                break;
            }

        default:
            return CM_LBS_PARAMERR;
    }

    return ret ;
}

static cm_lbs_state_e __cm_lbs_atcmd_amap_config_get(char *subcmd, char *at_send)
{
    cm_lbs_state_e ret = CM_LBS_ERR;
#if  CM_LBS_PLAT_AMAP10_SUPPORT||CM_LBS_PLAT_AMAP20_SUPPORT
    cm_lbs_atcmd_option_e op = cm_lbs_atcmd_subcmd_to_option(subcmd);
    if(LBS_LBS_ATCMD_NULL_OP == op)
    {
        return CM_LBS_PARAMERR;
    }


    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();

    /*read amap config*/
    cm_lbs_atcmd_amap_config_t *attr = cm_lbs_malloc(sizeof(cm_lbs_atcmd_amap_config_t));
    if(NULL == attr)
    {
        return CM_LBS_MEMROYERR;
    }

    if(CM_LBS_OK != cm_lbs_atcmd_read_amapcfg(platform, attr))
    {
        attr->descr_mode = CM_LBS_DESCR_MODE_DEFAULT;
        attr->precision = CM_LBS_PRECISION_DEFAULT;
        attr->signal_enable = CM_LBS_SIGNATURE_EN_DEFAULT;
    }

    ret = CM_LBS_OK;
    sprintf(at_send, "+MLBSCFG: \"%s\",", subcmd);
    switch(op)
    {
        case LBS_ATCMD_METHOD_OP:
            {
                char s_platform[4] = {0};
                sprintf(s_platform, "%d", (int)platform);
                strcat(at_send, s_platform);
                break;
            }

        case LBS_ATCMD_APIKEY_OP:
            {
                char s_apikey_md5[64] = {0};
                cm_lbs_atcmd_md5((char *)attr->apikey, s_apikey_md5);
                strcat(at_send, s_apikey_md5);
                break;
            }
        case LBS_ATCMD_PRECISION:
            {
                char s_precision[4] = {0};
                sprintf(s_precision, "%d", attr->precision);
                strcat(at_send, s_precision);
                break;
            }
        case LBS_ATCMD_DESCR_MODE:
            {
                char s_descr_mode[4] = {0};
                sprintf(s_descr_mode, "%d", attr->descr_mode);
                strcat(at_send, s_descr_mode);
                break;
            }

        case LBS_ATCMD_SIGN_ENABLE:
            {
                char s_sign_en[4] = {0};
                sprintf(s_sign_en, "%d", attr->signal_enable);
                strcat(at_send, s_sign_en);
                break;
            }

        case LBS_ATCMD_SIGN_KEY:
            {
                char s_signkey_md5[64] = {0};
                cm_lbs_atcmd_md5((char *)attr->signature_key, s_signkey_md5);
                strcat(at_send, s_signkey_md5);
                break;
            }
        case LBS_ATCMD_NEARBTS_ENABLE:
            {
                char s_nearbts_en[4] = {0};
                sprintf(s_nearbts_en, "%d", attr->nearbts_enable);
                strcat(at_send, s_nearbts_en);
                break;
            }
        default:
            ret = CM_LBS_PARAMERR;
            break;
    }

    free(attr);
#endif /*CM_LBS_PLAT_AMAP10_SUPPORT||CM_LBS_PLAT_AMAP20_SUPPORT*/
    return ret;

}

static cm_lbs_state_e __cm_lbs_atcmd_oneospos_config_get(char *subcmd, char *at_send)
{
    cm_lbs_atcmd_option_e op = cm_lbs_atcmd_subcmd_to_option(subcmd);
    if(LBS_LBS_ATCMD_NULL_OP == op)
    {
        return CM_LBS_PARAMERR;
    }

    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();
    CMLBS_LOGI("platform is:%d", platform);

    cm_lbs_atcmd_oneospos_config_t *oneospos_attr = (cm_lbs_atcmd_oneospos_config_t *)cm_lbs_malloc(sizeof(cm_lbs_atcmd_oneospos_config_t));
    if(NULL == oneospos_attr)
    {
        return CM_LBS_MEMROYERR;
    }
    if(CM_LBS_OK != cm_lbs_atcmd_Read_oneosposcfg(platform, oneospos_attr))
    {
        oneospos_attr->precision = CM_LBS_PRECISION_DEFAULT;
        oneospos_attr->coordinate_enable = CM_LBS_ONEOS_COORDINATE;
        memcpy(oneospos_attr->pid, CM_LBS_ONEOSPOS_INTERNAL_PID, strlen(CM_LBS_ONEOSPOS_INTERNAL_PID));//未读取到配置文件，使用代码内置pid
    }

    cm_lbs_state_e ret = CM_LBS_OK;
    sprintf(at_send, "+MLBSCFG: \"%s\",", subcmd);
    switch(op)
    {
        case LBS_ATCMD_PID:
            {
                char s_pid_md5[64] = {0};
                cm_lbs_atcmd_md5((char *)oneospos_attr->pid, s_pid_md5);
                strcat(at_send, s_pid_md5);
                break;
            }
        case LBS_ATCMD_PRECISION:
            {
                char s_precision[4] = {0};
                sprintf(s_precision, "%d", oneospos_attr->precision);
                strcat(at_send, s_precision);
                break;
            }
        case LBS_ATCMD_NEARBTS_ENABLE:
            {
                char s_nearbts_en[4] = {0};
                sprintf(s_nearbts_en, "%d", oneospos_attr->nearbts_enable);
                strcat(at_send, s_nearbts_en);
                break;
            }
        case LBS_ATCMD_ONEOS_INTERNAL_PID:
            {
                char s_internal_pid_md5[64] = {0};
                char internal_pid[64] = {0};
                memcpy(internal_pid, CM_LBS_ONEOSPOS_INTERNAL_PID, strlen(CM_LBS_ONEOSPOS_INTERNAL_PID));
                cm_lbs_atcmd_md5(internal_pid, s_internal_pid_md5);
                strcat(at_send, s_internal_pid_md5);
                break;
            }
        case LBS_ATCMD_ONEOS_CHANGE_COORDINATE:
            {
                char coordinate[4] = {0};
                sprintf(coordinate, "%d", oneospos_attr->coordinate_enable);
                strcat(at_send, coordinate);
                break;
            }
        default:
            ret = CM_LBS_PARAMERR;
            break;
    }

    free(oneospos_attr);
    return ret;
}

cm_lbs_state_e cm_lbs_atcmd_MLBSCFG_get(char *subcmd, char *at_send)
{
    
    // CMLBS_LOG("function:%s",__FUNCTION__);
    if(NULL == at_send)
    {
        return CM_LBS_PARAMERR;
    }
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();
    CMLBS_LOGI("get platform=%d",platform);

    cm_lbs_atcmd_option_e op = cm_lbs_atcmd_subcmd_to_option(subcmd);

    if(LBS_ATCMD_METHOD_OP == op)
    {
        sprintf(at_send, "+MLBSCFG: \"%s\",", subcmd);
        char s_platform[4] = {0};
        sprintf(s_platform, "%d", (int)platform);
        strcat(at_send, s_platform);
        return  CM_LBS_OK;
    }

    if(CM_LBS_PLAT_ONEOSPOS == platform)
    {
        return __cm_lbs_atcmd_oneospos_config_get(subcmd, at_send);
    }
        // CMLBS_LOG("function:%s has returned",__FUNCTION__);

    return  __cm_lbs_atcmd_amap_config_get(subcmd, at_send);
}


static cm_lbs_state_e __cm_lbs_atcmd_amap_setnv_config(int platform, cm_lbs_atcmd_amap_config_t *amap_cfg)
{
    // CMLBS_LOG("function:%s",__FUNCTION__);
    cm_lbs_nvconfig_t *nvconfig = (cm_lbs_nvconfig_t *)cm_lbs_malloc(sizeof(cm_lbs_nvconfig_t));
    if(NULL==nvconfig)
    {
      return CM_LBS_MEMROYERR;
    }

    
    if(CM_LBS_OK != cm_lbs_read_nv_data(nvconfig))
    {
        free(nvconfig);
        return CM_LBS_ERR;
    }

    if(platform == CM_LBS_PLAT_AMAP10)
    {
        int len = strlen(nvconfig->amapv1key);
        if(len < 1 || 0 == strcmp(nvconfig->amapv1key, "0"))
        {
            free(nvconfig);
            return CM_LBS_ERR;
        }

        strncpy((char *)amap_cfg->apikey, nvconfig->amapv1key, len);
        amap_cfg->signal_enable = nvconfig->amapv1signa_enable;

        if(1 == nvconfig->amapv1signa_enable)
        {
            if(0 == strcmp(nvconfig->amapv1signa, "0") || strlen(nvconfig->amapv1signa) < 1)
            {
                free(nvconfig);
                return CM_LBS_ERR;
            }
        }
        strncpy((char *)amap_cfg->signature_key, nvconfig->amapv1signa, strlen(nvconfig->amapv1signa));

        free(nvconfig);
        return CM_LBS_OK;
    }
    if(platform == CM_LBS_PLAT_AMAP20)
    {
        int len = strlen(nvconfig->amapv2key);
        if(len < 1 || 0 == strcmp(nvconfig->amapv2key, "0"))
        {
            free(nvconfig);
            return CM_LBS_ERR;
        }

        strncpy((char *)amap_cfg->apikey, nvconfig->amapv2key, len);
        amap_cfg->signal_enable = nvconfig->amapv2signa_enable;

        if(1 == nvconfig->amapv2signa_enable)
        {
            if(0 == strcmp(nvconfig->amapv2signa, "0") || strlen(nvconfig->amapv2signa) < 1)
            {
                free(nvconfig);
                return CM_LBS_ERR;
            }
        }
        strncpy((char *)amap_cfg->signature_key, nvconfig->amapv2signa, strlen(nvconfig->amapv2signa));
        free(nvconfig);
        return CM_LBS_OK;
    }
    free(nvconfig);
    return CM_LBS_ERR;
}

static cm_lbs_state_e __cm_lbs_atcmd_oneospos_setnv_config(cm_lbs_atcmd_oneospos_config_t *oneospos_attr)
{
    cm_lbs_nvconfig_t *nvconfig = (cm_lbs_nvconfig_t *)cm_lbs_malloc(sizeof(cm_lbs_nvconfig_t));
    if(NULL == nvconfig)
    {
        return CM_LBS_MEMROYERR;
    }

    if(CM_LBS_OK != cm_lbs_read_nv_data(nvconfig))
    {
        free(nvconfig);
        return CM_LBS_ERR;
    }

    if(0 == strcmp(nvconfig->oneospospid, "0") || strlen(nvconfig->oneospospid) < 1)
    {
        free(nvconfig);
        return CM_LBS_ERR;
    }
    strncpy((char *)oneospos_attr->pid, nvconfig->oneospospid, strlen(nvconfig->oneospospid));

    free(nvconfig);
    return CM_LBS_OK;
}


static cm_lbs_atcmd_errcode_e __cm_lbs_atcmd_amap_mlbsloc(void *cb_arg)
{
    // CMLBS_LOG("%s",__FUNCTION__);    
    cm_lbs_atcmd_errcode_e ret = LBS_ATCMD_SUCCESS;
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();
    cm_lbs_atcmd_amap_config_t *amap_cfg = cm_lbs_malloc(sizeof(cm_lbs_atcmd_amap_config_t));
    if(NULL == amap_cfg)
    {
        return LBS_ATCMD_UNKNOWERR;
    }

    do
    {

        if(CM_LBS_OK != __cm_lbs_get_netstate(NULL))
        {
            CMLBS_LOGE("net error!");
            ret = LBS_ATCMD_NETERR;
            break;
        }

        cm_lbs_state_e res = 0;
        res = cm_lbs_atcmd_read_amapcfg(platform, amap_cfg);
        if(strlen((char *)amap_cfg->apikey) < 1 || 0 == strcmp((char *)amap_cfg->apikey, "0"))
        {
            if(CM_LBS_OK != __cm_lbs_atcmd_amap_setnv_config(platform, amap_cfg))
            {
                ret = LBS_ATCMD_APIKEYNULL;
                break;
            }
        }

        if(CM_LBS_OK != res)
        {
            amap_cfg->descr_mode = CM_LBS_DESCR_MODE_DEFAULT;
            amap_cfg->precision = CM_LBS_PRECISION_DEFAULT;
            amap_cfg->nearbts_enable = CM_LBS_NEARBTS_ENABLE;
        }

        cm_lbs_amap_location_attr_t attr = {0};
        attr.api_key = amap_cfg->apikey;
        attr.time_out = CM_LBS_LOATION_TIMEOUT;
        attr.show_fields_enable = amap_cfg->descr_mode;
        attr.digital_sign_enable = amap_cfg->signal_enable;
        attr.digital_key = amap_cfg->signature_key;
        attr.nearbts_enable = amap_cfg->nearbts_enable;

        /*初始化LBS任务 及参数*/
        int32_t init_ret = cm_lbs_init(platform, (void *)&attr);
        if(0 != init_ret)
        {
            if(-2 == init_ret)
            {
                ret = LBS_ATCMD_BUSY;
            }
            else
            {
                ret = LBS_ATCMD_UNKNOWERR;
            }
            break;
        }

        res = cm_lbs_location(cm_lbs_atcmd_callback, cb_arg);
        if(0 == res)
        {
            ret = LBS_ATCMD_SUCCESS;
        }
        else if(-3 == res)
        {
            ret = LBS_ATCMD_BUSY;
        }
        else
        {
            ret = LBS_ATCMD_UNKNOWERR;
        }

    } while(0);

    free(amap_cfg);
    return ret;
}

static cm_lbs_atcmd_errcode_e __cm_lbs_atcmd_oneospos_mlbsloc(void *cb_arg)
{
    cm_lbs_atcmd_oneospos_config_t *oneospos_attr = (cm_lbs_atcmd_oneospos_config_t *)cm_lbs_malloc(sizeof(cm_lbs_atcmd_oneospos_config_t));
    if(NULL == oneospos_attr)
    {
        return CM_LBS_MEMROYERR;
    }
    cm_lbs_atcmd_errcode_e ret = LBS_ATCMD_SUCCESS;

    do
    {
        if(CM_LBS_OK != __cm_lbs_get_netstate(NULL))
        {
            CMLBS_LOGE("net error!");
            ret = LBS_ATCMD_NETERR;
            break;
        }

        if(CM_LBS_OK != cm_lbs_atcmd_Read_oneosposcfg(CM_LBS_PLAT_ONEOSPOS, oneospos_attr))
        {
            oneospos_attr->precision = CM_LBS_PRECISION_DEFAULT;
            oneospos_attr->nearbts_enable = CM_LBS_NEARBTS_ENABLE;
            oneospos_attr->coordinate_enable = CM_LBS_ONEOS_COORDINATE;
            if(CM_LBS_OK != __cm_lbs_atcmd_oneospos_setnv_config(oneospos_attr))
            {
                memset(oneospos_attr->pid, 0, CM_LBS_ONEOSPOS_PID_LEN);
                memcpy(oneospos_attr->pid, CM_LBS_ONEOSPOS_INTERNAL_PID, strlen(CM_LBS_ONEOSPOS_INTERNAL_PID));
            }

        }

        if(strlen((char *)oneospos_attr->pid) < 1 || 0 == strcmp((char *)oneospos_attr->pid, "0"))
        {
            if(CM_LBS_OK != __cm_lbs_atcmd_oneospos_setnv_config(oneospos_attr))
            {
                memset(oneospos_attr->pid, 0, CM_LBS_ONEOSPOS_PID_LEN);
                memcpy(oneospos_attr->pid, CM_LBS_ONEOSPOS_INTERNAL_PID, strlen(CM_LBS_ONEOSPOS_INTERNAL_PID));
            }
        }

        // s_oneospos_attr.precision = oneospos_attr->precision;

        cm_lbs_oneospos_attr_t attr = {0};
        attr.pid = (char *)oneospos_attr->pid;
        //attr.pid = NULL;
        attr.nearbts_enable = oneospos_attr->nearbts_enable;
        attr.time_out = 20;

        int32_t init_ret = cm_lbs_init(CM_LBS_PLAT_ONEOSPOS, (void *)&attr);
        if(0 != init_ret)
        {
            if(-2 == init_ret)
            {
                CMLBS_LOGE("LBS ATCMD busy");
                ret = LBS_ATCMD_BUSY;
            }
            else
            {
                CMLBS_LOGE("LBS ATCMD unknown error");
                ret = LBS_ATCMD_UNKNOWERR;
            }
            break;
        }

        int res = cm_lbs_location(cm_lbs_atcmd_callback, cb_arg);
        if(0 == res)
        {
            ret = LBS_ATCMD_SUCCESS;
        }
        else if(-3 == res)
        {
            ret = LBS_ATCMD_BUSY;
        }
        else
        {
            ret = LBS_ATCMD_UNKNOWERR;
        }

    } while(0);

    free(oneospos_attr);
    return ret;
}


cm_lbs_atcmd_errcode_e cm_lbs_atcmd_MLBSLOC(void *cb_arg)
{
    CMLBS_LOGI("[cm_lbs_atcmd_MLBSLOC] begin\n");
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();

    if(CM_LBS_PLAT_ONEOSPOS == platform)
    {
        return __cm_lbs_atcmd_oneospos_mlbsloc(cb_arg);
    }

    return  __cm_lbs_atcmd_amap_mlbsloc(cb_arg);
}


/*获取位置信息回调 兼容送样版本指令 AT+MLBS*/
static void cm_lbs_atcmd_at_mlbs_callback(cm_lbs_callback_event_e event, cm_lbs_location_rsp_t *location, void *cb_arg)
{
      //unused
//    cm_lbs_atcmd_statecode_e atcmd_ret = LBS_ATCMD_STATE_SUCCED;
    char urc_buf[64] = {0};
    strcat(urc_buf, "\r\n+MLBS :");
    switch(event)
    {
        case CM_LBS_LOCATION_OK :
            {
                char location_ll[32] = {0};
                sprintf(location_ll, "0,%s,%s", location->longitude, location->latitude);
                strcat(urc_buf, location_ll);
                //atcmd_ret = LBS_ATCMD_STATE_SUCCED;
                break;
            }

        default :
            strcat(urc_buf, "1");
            break;
    }
    strcat(urc_buf, "\r\n");
    cm_lbs_urc_rsp(cb_arg, urc_buf);

    /*释放资源*/
    cm_lbs_deinit();

    return ;
}


/*获取位置信息 兼容送样版本指令 AT+MLBS*/
cm_lbs_atcmd_errcode_e cm_lbs_atcmd_MLBS(void *cb_arg)
{
    cm_lbs_atcmd_errcode_e ret = LBS_ATCMD_SUCCESS;
    cm_lbs_location_platform_e platform = cm_lbs_atcmd_getplatform();
    cm_lbs_atcmd_amap_config_t *amap_cfg = cm_lbs_malloc(sizeof(cm_lbs_atcmd_amap_config_t));
    if(NULL == amap_cfg)
    {
        return LBS_ATCMD_UNKNOWERR;
    }
    do
    {
        int res = 0;
        res = cm_lbs_atcmd_read_amapcfg(platform, amap_cfg);
        if(CM_LBS_OK != res || strlen((char *)amap_cfg->apikey) < 1)
        {
            ret = LBS_ATCMD_APIKEYNULL;
            break;
        }

        cm_lbs_amap_location_attr_t attr = {0};
        attr.api_key = amap_cfg->apikey;
        attr.time_out = CM_LBS_LOATION_TIMEOUT;
        attr.show_fields_enable = amap_cfg->descr_mode;
        attr.digital_sign_enable = amap_cfg->signal_enable;
        attr.digital_key = amap_cfg->signature_key;
        attr.nearbts_enable = amap_cfg->nearbts_enable;

        /*初始化LBS任务 及参数*/
        int32_t init_ret = cm_lbs_init(platform, (void *)&attr);
        if(0 != init_ret)
        {
            ret = LBS_ATCMD_UNKNOWERR;
            break;
        }

        /*异步执行定位*/
        res = cm_lbs_location(cm_lbs_atcmd_at_mlbs_callback, cb_arg);
        if(CM_LBS_OK == res)
        {
            ret = LBS_ATCMD_SUCCESS;
        }
        else if(CM_LBS_BUSY == res)
        {
            ret = LBS_ATCMD_BUSY;
        }
        else if(CM_LBS_PARAMERR == res)
        {
            ret = LBS_ATCMD_PARAERR;
        }
        else if(CM_LBS_NET_ERR == res)
        {
            ret = LBS_ATCMD_NETERR;
        }
        else
        {
            ret = LBS_ATCMD_UNKNOWERR;
        }

    } while(0);

    free(amap_cfg);
    return ret;
}

cm_lbs_atcmd_nvcfg_type_t cm_lbs_atcmd_mlbsfact_typefind(char *subcmd)
{
    int count = sizeof(nvcfg_option_info) / sizeof(cm_lbs_atcmd_nvcfg_op_t);
    int i = 0;
    for(i = 0; i < count; i++)
    {
        if(0 == strcmp(nvcfg_option_info[i].subcmd, subcmd))
        {
            return nvcfg_option_info[i].type;
        }
    }
    return LBS_ATCMD_NVCFG_NULL;
}

cm_lbs_state_e cm_lbs_atcmd_mlbsfact_stringfind(int type, char *str)
{
    int count = sizeof(nvcfg_option_info) / sizeof(cm_lbs_atcmd_nvcfg_op_t);
    int i = 0;
    for(i = 0; i < count; i++)
    {
        if(type == nvcfg_option_info[i].type)
        {
            strncpy(str, nvcfg_option_info[i].subcmd, strlen(nvcfg_option_info[i].subcmd));
            return CM_LBS_OK;
        }
    }
    return CM_LBS_ERR;;
}

cm_lbs_atcmd_errcode_e cm_lbs_atcmd_mlbsfact_write(int type, char *param)
{
    cm_lbs_atcmd_errcode_e ret = LBS_ATCMD_SUCCESS;
    cm_lbs_nvconfig_t *nvconfig = (cm_lbs_nvconfig_t *)cm_lbs_malloc(sizeof(cm_lbs_nvconfig_t));
    if(NULL == nvconfig)
    {
        return LBS_ATCMD_UNKNOWERR;
    }

    cm_lbs_read_nv_data(nvconfig);

    int param_len = strlen(param);
    switch(type)
    {
        case LBS_ATCMD_NVCFG_ONEOSPOSPID :
            {
                if(param_len > CM_LBS_NVCONFIG_ONEOSPOS_PIDLEN)
                {
                    ret = LBS_ATCMD_PARAERR;
                    break;
                }
                memset(nvconfig->oneospospid, 0, CM_LBS_NVCONFIG_ONEOSPOS_PIDLEN);
                strncpy(nvconfig->oneospospid, param, param_len);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV1KEY :
            {
                if(param_len >  CM_LBS_NVCONFIG_AMAP_KEYLEN)
                {
                    ret = LBS_ATCMD_PARAERR;
                    break;
                }
                memset(nvconfig->amapv1key, 0, CM_LBS_NVCONFIG_AMAP_KEYLEN);
                strncpy(nvconfig->amapv1key, param, param_len);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV1SIGNA :
            {
                if(param_len > CM_LBS_NVCONFIG_AMAP_SIGNALEN)
                {
                    ret = LBS_ATCMD_PARAERR;
                    break;
                }
                memset(nvconfig->amapv1signa, 0, CM_LBS_NVCONFIG_AMAP_SIGNALEN);
                strncpy(nvconfig->amapv1signa, param, param_len);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV2KEY :
            {
                if(param_len > CM_LBS_NVCONFIG_AMAP_KEYLEN)
                {
                    ret = LBS_ATCMD_PARAERR;
                    break;
                }
                memset(nvconfig->amapv2key, 0, CM_LBS_NVCONFIG_AMAP_KEYLEN);
                strncpy(nvconfig->amapv2key, param, param_len);
                break;

            }

        case LBS_ATCMD_NVCFG_AMAPV2SIGNA :
            {
                if(param_len > CM_LBS_NVCONFIG_AMAP_SIGNALEN)
                {
                    ret = LBS_ATCMD_PARAERR;
                    break;
                }
                memset(nvconfig->amapv2signa, 0, CM_LBS_NVCONFIG_AMAP_SIGNALEN);
                strncpy(nvconfig->amapv2signa, param, param_len);
                break;
            }
        case LBS_ATCMD_NVCFG_AMAPV1SIGNA_EN :
            {
                int amapv1singna_en = atoi(param);
                if(1 == amapv1singna_en || 0 == amapv1singna_en)
                {
                    nvconfig->amapv1signa_enable = amapv1singna_en;
                }
                else
                {
                    ret = LBS_ATCMD_PARAERR;
                }
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV2SIGNA_EN :
            {
                int amapv2singna_en = atoi(param);
                if(1 == amapv2singna_en || 0 == amapv2singna_en)
                {
                    nvconfig->amapv2signa_enable = amapv2singna_en;
                }
                else
                {
                    ret = LBS_ATCMD_PARAERR;
                }
                break;
            }

        default :
            ret = LBS_ATCMD_PARAERR;
            break;
    }

    if(LBS_ATCMD_SUCCESS == ret)
    {
        if(CM_LBS_OK != cm_lbs_write_nv_data(nvconfig))
        {
            ret = LBS_ATCMD_UNKNOWERR;
        }
    }
    free(nvconfig);
    return ret;

}

cm_lbs_atcmd_errcode_e cm_lbs_atcmd_mlbsfact_read(int type, char *at_rsp)
{
    if(NULL == at_rsp)
    {
        return LBS_ATCMD_UNKNOWERR;
    }

    cm_lbs_nvconfig_t *nvconfig = (cm_lbs_nvconfig_t *)cm_lbs_malloc(sizeof(cm_lbs_nvconfig_t));
    if(CM_LBS_OK != cm_lbs_read_nv_data(nvconfig))
    {
        return LBS_ATCMD_UNKNOWERR;
    }

    char sub_cmd[64] = {0};
    if(CM_LBS_OK != cm_lbs_atcmd_mlbsfact_stringfind(type, sub_cmd))
    {
        return LBS_ATCMD_UNKNOWERR;
    }
    char *rsp_buf = at_rsp;
    char param[128] = {0};

    strcat(rsp_buf, "\"");
    strcat(rsp_buf, sub_cmd);
    strcat(rsp_buf, "\"");
    strcat(rsp_buf, ",");

    switch(type)
    {
        case LBS_ATCMD_NVCFG_ONEOSPOSPID :
            {
                strcat(param, nvconfig->oneospospid);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV1KEY :
            {
                strcat(param, nvconfig->amapv1key);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV1SIGNA :
            {
                strcat(param, nvconfig->amapv1signa);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV2KEY :
            {
                strcat(param, nvconfig->amapv2key);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV2SIGNA :
            {
                strcat(param, nvconfig->amapv2signa);
                break;
            }
        case LBS_ATCMD_NVCFG_AMAPV1SIGNA_EN :
            {
                char en_buf[2] = {0};
                sprintf(en_buf, "%d", nvconfig->amapv1signa_enable);
                strcat(param, en_buf);
                break;
            }

        case LBS_ATCMD_NVCFG_AMAPV2SIGNA_EN :
            {
                char en_buf[2] = {0};
                sprintf(en_buf, "%d", nvconfig->amapv2signa_enable);
                strcat(param, en_buf);
                break;
            }

        default :
            break;
    }

    free(nvconfig);
    if(LBS_ATCMD_NVCFG_AMAPV1SIGNA_EN != type && LBS_ATCMD_NVCFG_AMAPV2SIGNA_EN != type)
    {
        if(1 == strlen(param) && 0 == strcmp(param, "0"))
        {
            strcat(rsp_buf, "none");
        }
        else
        {
            char md5_vue[64] = {0};
            cm_lbs_md5(param, md5_vue);
            strcat(rsp_buf, md5_vue);
        }
    }
    else
    {
        strcat(rsp_buf, param);
    }

    return LBS_ATCMD_SUCCESS;
}

cm_lbs_atcmd_errcode_e cm_lbs_atcmd_mlbsfact_clearn(int type)
{
    /*暂时不支持*/
    return LBS_ATCMD_UNKNOWERR;
}

cm_lbs_atcmd_errcode_e cm_lbs_atcmd_mlbsfact(char *subcmd, void *arg, char *at_rsp)
{
    cm_lbs_atcmd_nvcfg_type_t nvcfg_type = cm_lbs_atcmd_mlbsfact_typefind(subcmd);
    if(LBS_ATCMD_NVCFG_NULL == nvcfg_type)
    {
        return LBS_ATCMD_PARAERR;
    }

    /*清除NV配置*/
    if(LBS_ATCMD_NVCFG_CLEARNALL == nvcfg_type)
    {
        return cm_lbs_atcmd_mlbsfact_clearn(nvcfg_type);
    }

    char *cmd_param = (char *)arg;
    if(NULL == arg || strlen(cmd_param) <= 0)
    {
        return cm_lbs_atcmd_mlbsfact_read(nvcfg_type, at_rsp);
    }
    else
    {
        return cm_lbs_atcmd_mlbsfact_write(nvcfg_type, cmd_param);
    }

}


