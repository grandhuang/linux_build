/**
 * @file        cm_file_at_cmd.c
 * @brief       文件系统AT命令
 * @copyright   Copyright © 2023 China Mobile IOT. All rights reserved.
 * @author      By
 * @date        2023/11/17
 *
 * @defgroup
 * @ingroup
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "cm_file_at_cmd.h"
#include "cm_file_platform.h"
#include "cm_file_util.h"

#include <string.h>

#include <dirent.h>
#include <ftw.h>
#include <unistd.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/
//文件系统通用配置的结构体参数
typedef struct
{
    uint16_t timeout; //单位为秒，默认值10s
    uint16_t inputfmt; //默认值0
    uint16_t outputfmt;//默认值0
    uint16_t  fragsize;//默认值8192
    uint8_t  interval; //单位为毫秒，默认值200ms
} FS_CmccCfg_t;

typedef enum
{
    FS_CMCC_RDONLY  = 0,       // r以只读方式打开文件，若文件不存在则报错
    FS_CMCC_WRONLY,            // w以只写方式打开文件，若文件不存在则创建该文件；如果文件存在，则清除并覆盖原文件
    FS_CMCC_WR_APPEND,         // a以只写方式打开文件，附加APPEND模式
    FS_CMCC_RDWR,              // w+以读/写方式打开文件，若文件不存在则创建该文件；如果文件存在，则清除并覆盖原文件
    FS_CMCC_RDWR_APPEND        // a+以读/写方式打开文件，附加APPEND模式
} AT_FS_CMCC_MODE;

typedef struct
{
    AT_FS_CMCC_MODE mode;
    char *value;
} fs_mode_t;
#if 0
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int _at_MFLIST_req(unsigned short atOpId, char *fullPath, char *Path);
static char *get_fs_mode(AT_FS_CMCC_MODE mode);
static int32_t cm_file_get_size(const uint8_t *local_file);	
static int cm_remove_directory(const char *path, const struct stat *stat_buf, int typeflag, struct FTW *ftw_buf);
/****************************************************************************
 * Private Data
 ****************************************************************************/
FS_OpenFileInfo_list g_cmcc_openfile_list = {0};
FS_CmccCfg_t g_cmcc_cfg = {FS_DEFAULT_WAITTIME, FS_CMCC_INFMT_ASC, FS_CMCC_OUTFMT_ORG, 0, 200};

const fs_mode_t fs_mode_tbl[] =
{
    {FS_CMCC_RDONLY,        "r"},
    {FS_CMCC_WRONLY,        "w"},
    {FS_CMCC_WR_APPEND,     "a"},
    {FS_CMCC_RDWR,          "w+"},
    {FS_CMCC_RDWR_APPEND,   "a+"}
};

static uint32_t s_file_datamode_data_length = 0; //数据模式需要接收的长度
static uint32_t s_file_datamode_recv_length = 0; //数据模式已接收的长度

/****************************************************************************
 * Functions
 ****************************************************************************/
/**
 * @brief 获取文件操作
 *
 * @param [in] mode               操作模式
 *
 * @return NULL 失败；其他 操作字符
 *
 * @details
 */
static char *get_fs_mode(AT_FS_CMCC_MODE mode)
{
    for (int i = 0; i < (int)(sizeof(fs_mode_tbl) / sizeof(fs_mode_tbl[0])); ++i)
    {
        if (fs_mode_tbl[i].mode == mode)
        {
            return fs_mode_tbl[i].value;
        }
    }

    return NULL;
}


/**
 * @brief 获取文件大小
 *
 * @param [in] local_file               文件路径
 *
 * @return 小于0 失败/其他 文件大小
 *
 * @details
 */
static int32_t cm_file_get_size(const uint8_t *local_file)
{
    struct stat file_stat;

    if (stat((char *)local_file, &file_stat) == 0)
    {
        return file_stat.st_size;
    }
    else
    {
        return -1;
    }

}

/**
 * @brief 目录删除回调
 *
 * @param [in] path               文件名
 * @param [in] stat_buf           不关注
 * @param [in] typeflag           不关注
 * @param [in] ftw_buf            不关注
 *
 * @return 小于0 失败/0 成功
 *
 * @details 删除目录时遍历删除目录中文件
 */
static int cm_remove_directory(const char *path, const struct stat *stat_buf, int typeflag, struct FTW *ftw_buf)
{
    //未关闭时先关闭
    close_openflist_byFname(&g_cmcc_openfile_list, (char *)path);

    int rv = remove(path);

    if (rv == 0)
    {
        printf("Deleted: %s\n", path);
    }
    else
    {
        perror("Error deleting file");
    }

    return rv;
}

//设置文件系统通用设置
unsigned int at_MFCFG_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *param = NULL;
    int32_t param1 = -1;
    int32_t param2 = -1;

    int param_cnt = paraList->paraNum;
    uint8_t string[80] = {0};

    switch (cmdType)
    {
        //2023-11-24，不支持AT+MFCFG="fragment"的参数配置
        case CMD_TYPE_SET:
        {
            if(param_cnt < 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            uint32_t data_string_len = 0;
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &param, &data_string_len, 0);

            if(parsing == false || param == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(strcmp("timeout", (const char *)param) == 0)
            {
                if(param_cnt != 1 && param_cnt != 2)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }

                if(param_cnt == 1)//查询timeout值
                {
                    sprintf((char *)string, "\r\n+MFCFG: \"timeout\",%d\r\n", g_cmcc_cfg.timeout);
                    cm_uart_printf_withoutline(atOpId, string);
                    break;
                }
                else//设置timeout值
                {
                    parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, (uint32_t *)&param1, 1, 120);

                    if(parsing == false)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    g_cmcc_cfg.timeout = (uint16_t)param1;
                }
            }
            else if(strcmp("encoding", (const char *)param) == 0)
            {
                if(param_cnt < 1 || param_cnt > 3 )
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }

                if(param_cnt == 1)//查询encoding值
                {
                    sprintf((char *)string, "\r\n+MFCFG: \"encoding\",%d,%d\r\n", g_cmcc_cfg.inputfmt, g_cmcc_cfg.outputfmt);
                    cm_uart_printf_withoutline(atOpId, string);
                    break;
                }

                if(paraList->para[1].len)
                {
#ifdef FILE_SUPPORT_ESCAPE_CHARACTER_ENABLE                   
                    parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, (uint32_t *)&param1, FS_CMCC_INFMT_ASC, FS_CMCC_INFMT_ESC);
#else
                    parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, (uint32_t *)&param1, FS_CMCC_INFMT_ASC, FS_CMCC_INFMT_HEX);
#endif
                    if(parsing == false)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }
                }

                if (param_cnt == 3 && paraList->para[2].len)
                {
                    parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[2].buff, paraList->para[2].len, (uint32_t *)&param2, FS_CMCC_OUTFMT_ORG, FS_CMCC_OUTFMT_HEX);

                    if(parsing == false)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    g_cmcc_cfg.outputfmt = (uint16_t)param2;
                }

                g_cmcc_cfg.inputfmt = (uint16_t)param1;

            }
            else
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            break;
        }

        case CMD_TYPE_TEST:
        {
#ifdef FILE_SUPPORT_ESCAPE_CHARACTER_ENABLE            
            snprintf((char *)string, 80, "\r\n+MFCFG: \"timeout\",(1-120)\r\n+MFCFG: \"encoding\",(0-2),(0-1)\r\n");
#else
            snprintf((char *)string, 80, "\r\n+MFCFG: \"timeout\",(1-120)\r\n+MFCFG: \"encoding\",(0-1),(0-1)\r\n");
#endif                   
            cm_uart_printf_withoutline(atOpId, string);
            break;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


//获取文件系统总大小和当前可用大小
unsigned int at_MFSINFO_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    switch (cmdType)
    {
        case CMD_TYPE_EXE:
        {
            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + 2;
            char real_path[real_path_len];
            if(cm_file_get_real_path("/", real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            fs_size_info_t fs_info = {0};

            if(cm_fsize_info(real_path, &fs_info) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }
            else
            {
                uint8_t string[64] = {0};
                snprintf((char *)string, 64, "\r\n+MFSINFO: %d,%d\r\n", fs_info.file_system_total_size, fs_info.file_system_remain_size);
                cm_uart_printf_withoutline(atOpId, string);
                break;
            }
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}

static int _at_MFLIST_req(unsigned short atOpId, char *fullPath, char *Path)
{
    uint32_t len = 0;
    char buff[FS_FILENAME_MAX_LEN + 256] = {0};
    int ret = RET_OK;
    uint32_t current_file_size = 0;

    struct dirent *pDent = NULL;
    DIR *pDir = opendir(fullPath);
    bool is_first = true;
    bool need_ = true;
    if(pDir)
    {
        len = strlen(Path);
        if(Path[len - 1] == '/')
        {
            need_ = false;
        }
        while ((pDent = readdir(pDir)) != NULL)
        {
            //过滤掉"."和".."
            if (0 == strcmp(pDent->d_name, ".") || 0 == strcmp(pDent->d_name, ".."))
            {
                continue;
            }
            if(pDent->d_type == DT_DIR)//为目录时
            {
                if(is_first)
                {
                    is_first = false;
                    len = sprintf(buff, "\r\n");
                }
                if(need_)
                {
                    len += sprintf(buff + len, "+MFLIST: \"%s/%s\"\r\n", Path, pDent->d_name);
                }
                else
                {
                    len += sprintf(buff + len, "+MFLIST: \"%s%s\"\r\n", Path, pDent->d_name);
                }
            }
            else // 为普通文件时
            {
                sprintf((char *)buff, "%s/%s", fullPath, pDent->d_name);

                current_file_size = cm_file_get_size((uint8_t *)buff);

                if(current_file_size >= 0)
                {
                    if(is_first)
                    {
                        is_first = false;
                        len = sprintf(buff, "\r\n");
                    }
                    if(need_)
                    {
                        len += sprintf(buff + len, "+MFLIST: \"%s/%s\",%d\r\n", Path, pDent->d_name, current_file_size);
                    }
                    else
                    {
                        len += sprintf(buff + len, "+MFLIST: \"%s%s\",%d\r\n", Path, pDent->d_name, current_file_size);
                    }
                }
                else
                {
                    ret = ATERR_FS_CMCC_VALUE_INVALID;
                    return ret;
                }
            }
            cm_uart_printf_data_withoutline(atOpId, (uint8_t *)buff, len);
            len = 0;
        }

        closedir(pDir);
    }

    return ret;
}

//列举已存在的文件
unsigned int at_MFLIST_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *fName = NULL;
    int ret = 0;

    uint32_t param_cnt = paraList->paraNum;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            uint32_t data_string_len = 0;
            if(param_cnt != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            ret = _at_MFLIST_req(atOpId, real_path, (char *)fName);

            break;
        }

        case CMD_TYPE_EXE:
        {
            ret = _at_MFLIST_req(atOpId, CM_FILE_SYS_PATH, "/");
            break;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, ret);
}

//获取文件大小
unsigned int at_MFSIZE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint32_t data_string_len = 0;
    uint8_t *fName = NULL;
    int file_len = 0;
    uint8_t string[32] = {0};

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + 128;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            file_len = cm_file_get_size((const uint8_t *)real_path);

            if(file_len > 0)
            {
                snprintf((char *)string, 32, "\r\n+MFSIZE: %d\r\n", file_len);
                cm_uart_printf_withoutline(atOpId, string);
            }
            else
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }

            break;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}

static void cm_file_mfput_datamode_cb(uint32_t atOpId, void *param, const void *data, uint32_t msg_len)
{
    FILE *fp = (FILE *)param;
    int ret = 0;
    uint32_t len = 0;
    if(msg_len == 0)  // 超时退出
    {
        fclose(fp);
        uint8_t string[32] = {0};
        snprintf((char *)string, 32, "\r\n+MFPUT: %d\r\n", s_file_datamode_recv_length);
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, string);
        return;
    }
    if(s_file_datamode_data_length > 0)
    {
        len = ((s_file_datamode_recv_length + msg_len) > s_file_datamode_data_length) 
                ? (s_file_datamode_data_length - s_file_datamode_recv_length) : (msg_len);
    }
    else
    {
        ret = cm_at_cmd_find_quit_symbol((char *)data, msg_len, &len);
    }
    if(ret == 2) //ESC取消
    {
        fseek(fp, 0, 0);
        if(ftruncate(fileno(fp), 0) == -1)
        {
            printf("Error truncating file\n");
        }
        fclose(fp);
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, NULL);
        return;
    }
    if(len)
    {
        if(fwrite(data, len, 1, fp) != 1)
        {
            fclose(fp);
            cm_at_cmd_set_atmode(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE, NULL);
            return;
        }
        s_file_datamode_recv_length = s_file_datamode_recv_length + len;
    }
    if(s_file_datamode_recv_length == s_file_datamode_data_length || ret == 1)
    {
        fclose(fp);
        uint8_t string[32] = {0};
        snprintf((char *)string, 32, "\r\n+MFPUT: %d\r\n", s_file_datamode_recv_length);
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, string);
    }
}

//直接写入数据到文件，文件不存在时直接创建，文件存在时将删除后创建
unsigned int at_MFPUT_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint32_t fsize = 0;
    uint8_t *fName = NULL;
    uint32_t data_string_len = 0;
    bool parsing = false;
    uint8_t *srcData = NULL;
    uint32_t write_size = 0;
    uint8_t *writeBuff = NULL;
    int param_cnt = paraList->paraNum;
    FILE *fp = NULL;
    uint8_t string[32] = {0};

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(param_cnt < 2 || param_cnt > 3)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, true) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, &fsize, 0, 0xffffffff);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            fp = fopen(real_path, "wb+");

            if(fp == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_OPEN);
            }

            if(param_cnt == 2) //进入数据模式
            {
                s_file_datamode_data_length = fsize;
                if(cm_at_cmd_set_datamode(atOpId, 2, 0, g_cmcc_cfg.timeout*1000, cm_file_mfput_datamode_cb, (void *)fp) < 0)
                {
                    fclose(fp);
                    CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
                }
                return 0;
            }
            else
            {
                parsing = cm_at_param_parsing_to_string(paraList->para[2].buff, paraList->para[2].len, &srcData, &data_string_len, 65535);

                if(parsing == false || srcData == NULL)
                {
                    fclose(fp);
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }

                if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_ASC)
                {
                    if(fsize == 0)
                    {
                        fsize = strlen((char *)srcData);
                    }
                    if(fsize != strlen((char *)srcData))
                    {
                        fclose(fp);
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = fsize;
                    writeBuff = srcData;
                }
                else if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_HEX)
                {
                    if(fsize == 0)
                    {
                        fsize = (strlen((char *)srcData) / 2);
                    }
                    if(strlen((char *)srcData) % 2 != 0 || fsize != (strlen((char *)srcData) / 2))
                    {
                        fclose(fp);
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    bool suc = cm_util_str2hex((char *)srcData, (char *)srcData, (int)strlen((char *)srcData));

                    if(suc != true)
                    {
                        fclose(fp);
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = fsize;
                    writeBuff = srcData;
                }
#ifdef FILE_SUPPORT_ESCAPE_CHARACTER_ENABLE               
                else if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_ESC)
                {
                    if(fsize != strlen((char *)srcData))
                    {
                        fclose(fp);
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    int escs_datalen = 0;
                    uint8_t *destData = (uint8_t *)cm_at_parse_escstring((char *)srcData, &escs_datalen);

                    if(destData == NULL)
                    {
                        fclose(fp);
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = escs_datalen;
                    writeBuff = destData;
                }
#endif
                if(fwrite(writeBuff, write_size, 1, fp) != 1)
                {
                    fclose(fp);
                    CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE);
                }

                fclose(fp);


                snprintf((char *)string, 32, "\r\n+MFPUT: %d\r\n", write_size);

                cm_uart_printf_withoutline(atOpId, string);

            }

            break;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}

void cm_file_mfget_datamode_cb(uint32_t atOpId, void *param, const void *data, uint32_t msg_len)
{
    //接收的数据不做处理
}

//从文件读取数据
unsigned int at_MFGET_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *fName = NULL;
    uint32_t data_string_len = 0;
    FILE *fp = NULL;
    char *fs_mfget_rcvbuff = NULL;

    int param_cnt = paraList->paraNum;
    int flag = 0;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(param_cnt != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            fp = fopen(real_path, "rb");

            if(fp == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_OPEN);
            }

            uint32_t fsize = cm_file_get_size_by_file_point(fp);

            if(fsize == 0)
            {
                sprintf(real_path, "\r\n+MFGET: 0,\r\n");
                cm_uart_printf_withoutline(atOpId, (uint8_t *)real_path);
                fclose(fp);
                break;
            }
            if(cm_at_cmd_set_datamode(atOpId, 2, 0, 0, cm_file_mfget_datamode_cb, (void *)fp) < 0)
            {
                fclose(fp);
                CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
            }

            uint32_t buffLen = 0;
            uint32_t read_size = 0;
            uint32_t read_total_len = 0;
            buffLen = (g_cmcc_cfg.fragsize > 0) ? g_cmcc_cfg.fragsize : FS_DEFAULT_FRAG_SIZE;
            if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
            {
                buffLen = ((fsize > buffLen / 2) ? buffLen / 2 : fsize)*2;
            }
            else
            {
                buffLen = (fsize > buffLen) ? buffLen : fsize;
            }

            fs_mfget_rcvbuff = malloc(buffLen + 32);
            if(fs_mfget_rcvbuff == NULL)
            {
                fclose(fp);
                cm_at_cmd_set_atmode(CM_WrapperGetPortID(atOpId), ATERR_FS_CMCC_VALUE_INVALID, NULL);
                return 0;
            }

            while ( read_total_len < fsize)
            {
                if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
                {
                    read_size = ((fsize - read_total_len) > (buffLen/2)) ? (buffLen/2) : (fsize - read_total_len);
                }
                else
                {
                    read_size = ((fsize - read_total_len) > buffLen) ? buffLen : (fsize - read_total_len);
                }
                if(flag == 0)
                {
                    flag = 1;
                    data_string_len = sprintf(fs_mfget_rcvbuff, "\r\n+MFGET: %d,", fsize);
                }
                else
                {
                    if(g_cmcc_cfg.fragsize > 0 && g_cmcc_cfg.interval)
                    {
                        usleep(g_cmcc_cfg.interval * 1000);
                    }
                }
                //读取文件内容
                if(fread(fs_mfget_rcvbuff + data_string_len, read_size, 1, fp) == 1)
                {
                    //输出文件内容
                    if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
                    {
                        if(cm_util_hex2str((char *)fs_mfget_rcvbuff + data_string_len, (uint8_t *)fs_mfget_rcvbuff + data_string_len, read_size) != true)
                        {
                            cm_free(fs_mfget_rcvbuff);
                            fclose(fp);
                            cm_at_cmd_set_atmode(CM_WrapperGetPortID(atOpId), ATERR_FS_CMCC_VALUE_INVALID, NULL);
                            return 0;
                        }
                        data_string_len = data_string_len + read_size*2;
                    }
                    else
                    {
                        data_string_len = data_string_len + read_size;
                    }
                    read_total_len += read_size;
                }
                else
                {
                    cm_free(fs_mfget_rcvbuff);
                    fclose(fp);
                    cm_at_cmd_set_atmode(CM_WrapperGetPortID(atOpId), ATERR_FS_CMCC_FAIL_TO_READ, NULL);
                    return 0;
                }
                if(read_total_len == fsize)
                {
                    data_string_len += sprintf(fs_mfget_rcvbuff + data_string_len, "\r\n");
                }
                uint32_t print_len = 0;
                if(data_string_len > 8192)
                {
                    print_len = 8192;
                    cm_at_cmd_send_data_bypass(CM_WrapperGetPortID(atOpId), fs_mfget_rcvbuff, print_len);
                }
                cm_at_cmd_send_data_bypass(CM_WrapperGetPortID(atOpId), fs_mfget_rcvbuff + print_len, data_string_len - print_len);
                data_string_len = 0;
            }
            cm_free(fs_mfget_rcvbuff);
            fclose(fp);
            cm_at_cmd_set_atmode(CM_WrapperGetPortID(atOpId), 0, NULL);
            return 0;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


//打开或创建文件
unsigned int at_MFOPEN_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *fName = NULL;
    uint32_t data_string_len = 0;
    uint32_t fMode = 0;

    uint8_t string[64] = {0};

    FS_OpenFileInfo_t *infoNode = NULL;
    uint32_t fHandle = 0;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 2)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];

            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, &fMode, 0, 4);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, (fMode > 0)) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            FILE *fd = NULL;
            fd = fopen((char *)real_path, get_fs_mode(fMode));

            if(fd == CM_FILE_FD_ERROR)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_OPEN);
            }

            infoNode = malloc(sizeof(FS_OpenFileInfo_t) + strlen(real_path) + 1);
            memset(infoNode, 0, sizeof(FS_OpenFileInfo_t) + strlen(real_path) + 1);
            infoNode->fMode = fMode;
            infoNode->fp = fd;
            memcpy(infoNode->fname, real_path, strlen(real_path));
            fHandle = insert_openflist(&g_cmcc_openfile_list, infoNode);

            snprintf((char *)string, 64, "\r\n+MFOPEN: %d\r\n", fHandle);
            cm_uart_printf_withoutline(atOpId, string);
            break;

        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


//读取已打开的文件
unsigned int at_MFREAD_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint32_t fhandle = 0;
    uint32_t  readLen = 0;
    uint32_t fsize = 0;
    uint32_t curPos = 0;

    uint32_t ret = RET_OK;
    bool parsing = false;
    bool first_flag = false;
    uint32_t string_len = 0;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            uint8_t param_cnt = paraList->paraNum;
            if(param_cnt < 1 || param_cnt > 2)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, &fhandle, 0, 65535);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL || fileInfo->fMode == FS_CMCC_WRONLY || fileInfo->fMode == FS_CMCC_WR_APPEND)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }

            //获取文件大小和文件的当前位置
            curPos = ftell(fileInfo->fp);

            fsize = cm_file_get_size_by_file_point(fileInfo->fp);

            //计算实际可读取的最大数据长度（从当前文件位置到文件结尾）
            if(param_cnt == 1)
            {
                readLen = fsize - curPos;
            }
            else
            {
                parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, &readLen, 0, 65535);

                if(parsing == false)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }
                readLen = (readLen > (fsize - curPos)) ? (fsize - curPos) : readLen;
            }

            //文件位置指针指向文件结尾，无法再读，直接返回ERROR
            if(readLen == 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_READ);
            }

            int read_length = 0;
            int fs_frag_size = (g_cmcc_cfg.fragsize > 0) ? g_cmcc_cfg.fragsize : FS_DEFAULT_FRAG_SIZE;
            if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
            {
                fs_frag_size = ((readLen > fs_frag_size / 2) ? fs_frag_size / 2 : readLen)*2;
            }
            else
            {
                fs_frag_size = (readLen > fs_frag_size) ? fs_frag_size : readLen;
            }

            int read_size = 0;

            char *data = malloc(fs_frag_size + 56);
            if(data == NULL)
            {
                ret = ATERR_FS_CMCC_VALUE_INVALID;
                break;
            }

            while(read_length < readLen)
            {
                if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
                {
                    read_size = ((readLen - read_length) > (fs_frag_size/2)) ? (fs_frag_size/2) : (readLen - read_length);
                }
                else
                {
                    read_size = ((readLen - read_length) > fs_frag_size) ? fs_frag_size : (readLen - read_length);
                }
                if(first_flag == false)
                {
                    first_flag = true;
                    string_len = sprintf((char *)data, "\r\n+MFREAD: %d,%d,%d,%d,", fhandle, readLen, (read_length + read_size), read_size);
                }
                else
                {
                    if(g_cmcc_cfg.fragsize > 0 && g_cmcc_cfg.interval)
                    {
                        usleep(g_cmcc_cfg.interval * 1000);
                    }
                    string_len = sprintf((char *)data, "+MFREAD: %d,%d,%d,%d,", fhandle, readLen, read_length + read_size, read_size);
                }
                //读取错误时跳出
                if (fread(data + string_len, read_size, 1, fileInfo->fp) != 1)
                {
                    free(data);
                    CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_READ);
                }
                if(g_cmcc_cfg.outputfmt == FS_CMCC_OUTFMT_HEX)
                {
                    if(cm_util_hex2str((char *)data + string_len, (uint8_t *)data + string_len, read_size) != true)
                    {
                        free(data);
                        CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
                    }
                    string_len = string_len + read_size*2;
                }
                else
                {
                    string_len = string_len + read_size;
                }

                read_length += read_size;
                memcpy(data + string_len, "\r\n", 2);
                string_len += 2;

                cm_uart_printf_data_withoutline(atOpId, (uint8_t *)data, string_len);
            }
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, ret);
}

void cm_file_mfwrite_datamode_cb(uint32_t atOpId, void *param, const void *data, uint32_t msg_len)
{
    FS_OpenFileInfo_t *fileInfo = (FS_OpenFileInfo_t *)param;
    FILE *fp = fileInfo->fp;
    int ret = 0;
    uint32_t len = 0;
    if(msg_len == 0)  // 超时退出
    {
        uint8_t string[32] = {0};
        snprintf((char *)string, 32, "\r\n+MFWRITE: %d,%d\r\n", fileInfo->fHandle, s_file_datamode_recv_length);
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, string);
        return;
    }
    if(s_file_datamode_data_length > 0)
    {
        len = ((s_file_datamode_recv_length + msg_len) > s_file_datamode_data_length) 
                ? (s_file_datamode_data_length - s_file_datamode_recv_length) : (msg_len);
    }
    else
    {
        ret = cm_at_cmd_find_quit_symbol((char *)data, msg_len, &len);
    }
    if(ret == 2) //ESC取消
    {
        len = ftell(fp) - s_file_datamode_recv_length;
        fseek(fp, 0, 0);
        if(ftruncate(fileno(fp), len) == -1) // 清除已写入长度
        {
            printf("Error truncating file\n");
        }
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, NULL);
        return;
    }
    if(len)
    {
        if(fwrite(data, len, 1, fp) != 1)
        {
            cm_at_cmd_set_atmode(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE, NULL);
            return;
        }
        s_file_datamode_recv_length = s_file_datamode_recv_length + len;
    }
    if(s_file_datamode_recv_length == s_file_datamode_data_length || ret == 1)
    {
        uint8_t string[32] = {0};
        snprintf((char *)string, 32, "\r\n+MFWRITE: %d,%d\r\n", fileInfo->fHandle, s_file_datamode_recv_length);
        s_file_datamode_recv_length = 0;
        cm_at_cmd_set_atmode(atOpId, RET_OK, string);
    }
}

//向已打开的文件中写入数据
unsigned int at_MFWRITE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint32_t fhandle = 0;
    uint32_t fsize = 0;
    uint8_t *srcData = NULL;
    uint8_t *writeBuff = NULL;

    bool parsing = false;
    uint32_t data_string_len = 0;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            int param_cnt = paraList->paraNum;
            if(param_cnt < 2 || param_cnt > 3)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, &fhandle, 0, 65535);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL || fileInfo->fMode == FS_CMCC_RDONLY)//只读方式打开，不允许写操作
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE);
            }

            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, &fsize, 0, 0xffffffff);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(param_cnt == 2) //进入数据模式
            {
                s_file_datamode_data_length = fsize;
                if(cm_at_cmd_set_datamode(atOpId, 2, 0, g_cmcc_cfg.timeout*1000, cm_file_mfwrite_datamode_cb, (void *)fileInfo) < 0)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
                }
                return 0;

            }
            else
            {
                uint32_t write_size = 0;

                parsing = cm_at_param_parsing_to_string(paraList->para[2].buff, paraList->para[2].len, &srcData, &data_string_len, 0);

                if(parsing == false || srcData == NULL)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }

                if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_ASC)
                {
                    if(fsize == 0)
                    {
                        fsize = data_string_len;
                    }
                    if(data_string_len != fsize)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = fsize;
                    writeBuff = srcData;
                }
                else if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_HEX)
                {
                    if(fsize == 0)
                    {
                        fsize = data_string_len / 2;
                    }
                    if((data_string_len % 2) || (data_string_len != (fsize * 2)))
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    bool suc = cm_util_str2hex((char *)srcData, (char *)srcData, (int)strlen((char *)srcData));

                    if(suc != true)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = fsize;
                    writeBuff = srcData;
                }
#ifdef FILE_SUPPORT_ESCAPE_CHARACTER_ENABLE                
                else if(g_cmcc_cfg.inputfmt == FS_CMCC_INFMT_ESC)
                {
                    if(data_string_len < fsize)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    int escs_datalen = 0;
                    uint8_t *destData = (uint8_t *)cm_at_parse_escstring((char *)srcData, &escs_datalen);

                    if(destData == NULL)
                    {
                        CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                    }

                    write_size = escs_datalen;
                    writeBuff = destData;
                }
#endif
                if(fwrite(writeBuff, write_size, 1, fileInfo->fp) != 1)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE);
                }

                uint8_t string[64] = {0};
                snprintf((char *)string, 64, "\r\n+MFWRITE: %d,%d\r\n", fhandle, write_size);
                cm_uart_printf_withoutline(atOpId, string);
                break;
            }

        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}

//20231122 命令不支持
unsigned int at_MFSYNC_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            uint32_t fhandle = 0;
            bool parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, &fhandle, 0, 65535);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_DESCRIPTOR_INVALID);
            }

            if(fsync(fileno(fileInfo->fp)) == -1)
            {
                printf("Error fsync file\n");
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }
    CMD_RET(atOpId, RET_OK);
}


unsigned int at_MFSEEK_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    int32_t fhandle = -1;
    int32_t offset = 0;
    uint32_t direction = 0;

    bool parsing = false;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            int param_cnt = paraList->paraNum;

            if(param_cnt != 1 && param_cnt != 3)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }


            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, (uint32_t *)&fhandle, 0, 65535);

            if(parsing == false || (param_cnt < 1 && param_cnt > 2))
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_WRITE);
            }

            if(param_cnt == 1)
            {
                int32_t pos = ftell(fileInfo->fp);

                if(pos < 0)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
                }
                else
                {
                    uint8_t string[64] = {0};
                    snprintf((char *)string, 64, "\r\n+MFSEEK: %d,%d\r\n", fhandle, pos);
                    cm_uart_printf_withoutline(atOpId, string);
                    break;
                }
            }

            parsing = cm_at_param_parsing_to_int32_with_range(paraList->para[1].buff, paraList->para[1].len, &offset, -0x7fffffff, 0x7fffffff);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[2].buff, paraList->para[2].len, &direction, 0, 2);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(is_fs_OverFlow(fileInfo->fp, offset, direction))
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }

            //设置文件指针到指定的位置
            if(fseek(fileInfo->fp, offset, direction) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}

//20231122 命令不支持
unsigned int at_MFTRUNC_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 2)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            uint32_t fhandle = 0;
            uint32_t length = 0;
            bool parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, &fhandle, 0, 65535);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, &length, 0, 0xffffffff);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_DESCRIPTOR_INVALID);
            }

            fseek(fileInfo->fp, 0, 0);
            if(ftruncate(fileno(fileInfo->fp), length) == -1) // 清除
            {
                printf("Error truncating file\n");
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }
    CMD_RET(atOpId, RET_OK);
}

//关闭已打开的文件
unsigned int at_MFCLOSE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            uint32_t fhandle = 0;
            bool parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[0].buff, paraList->para[0].len, &fhandle, 0, 65535);

            if(parsing == false)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //查找文件句柄是否已经打开
            FS_OpenFileInfo_t *fileInfo = find_openflist_byHandle(&g_cmcc_openfile_list, fhandle);

            if(fileInfo == NULL)//只读方式打开，不允许写操作
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_DESCRIPTOR_INVALID);
            }

            //关闭文件
            if(fclose(fileInfo->fp) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FILE_NOEXIST);
            }

            //删除open文件列表对应节点
            remove_openflist(&g_cmcc_openfile_list, fhandle);
            break;

        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


unsigned int at_MFDELETE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *fName  = NULL;
    uint32_t data_string_len = 0;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(paraList->paraNum != 1)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(fs_check_dir((char *)fName))
            {
                //递归删除指定存储媒介中的文件夹及所有子文件夹与子文件
                if(nftw((char *)real_path, cm_remove_directory, 64, FTW_DEPTH | FTW_PHYS) != 0)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_DELETE);
                }
            }
            else
            {
                //未关闭时先关闭
                close_openflist_byFname(&g_cmcc_openfile_list, real_path);

                //删除指定存储媒介中的单个文件
                if(remove((char *)real_path) < 0)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_DELETE);
                }
            }
            break;
        }

        case CMD_TYPE_EXE:
        {
            //递归删除指定存储媒介中的文件夹及所有子文件夹与子文件
            if(nftw(CM_FILE_SYS_PATH, cm_remove_directory, 64, FTW_DEPTH | FTW_PHYS) != 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_DELETE);
            }

            break;
        }

        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


unsigned int at_MFMOVE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *old_name = NULL;
    uint8_t *new_name = NULL;

    int param_cnt = paraList->paraNum;

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            if(param_cnt != 2)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            uint32_t data_string_len = 0;

            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &old_name, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || old_name == NULL || data_string_len == 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            parsing = cm_at_param_parsing_to_string(paraList->para[1].buff, paraList->para[1].len, &new_name, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || new_name == NULL || data_string_len == 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }


            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char old_real_path[real_path_len];
            char new_real_path[real_path_len];
            if(cm_file_get_real_path((char *)old_name, old_real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }
            if(cm_file_get_real_path((char *)new_name, new_real_path, real_path_len, true) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //检查可移动或者重命名的文件是否存在
            if(access(old_real_path, F_OK) != 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FILE_NOEXIST);
            }
            //移到文件
            if(rename(old_real_path, new_real_path) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FILE_NOEXIST);
            }
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }

    CMD_RET(atOpId, RET_OK);
}


//校验文件
unsigned int at_MFCHECK_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList)
{
    uint8_t *fName = NULL;
    FILE *fp = NULL;
    unsigned char value[16] = {0};

    switch (cmdType)
    {
        case CMD_TYPE_SET:
        {
            uint8_t param_cnt = paraList->paraNum;

            if(param_cnt != 1 && param_cnt != 2)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            uint32_t fMode = FS_CMCC_VERIFY_MD5;
            uint32_t data_string_len = 0;

            bool parsing = cm_at_param_parsing_to_string(paraList->para[0].buff, paraList->para[0].len, &fName, &data_string_len, FS_FILENAME_MAX_LEN);

            if(parsing == false || fName == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            //获取文件系统中的绝对文件路径
            uint16_t real_path_len = CM_FILE_SYS_PATH_LEN + FS_FILENAME_MAX_LEN;
            char real_path[real_path_len];
            if(cm_file_get_real_path((char *)fName, real_path, real_path_len, false) < 0)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
            }

            if(param_cnt == 2)
            {
                //仅支持MD5
                parsing = cm_at_param_parsing_to_uint32_with_range(paraList->para[1].buff, paraList->para[1].len, (uint32_t *)&fMode, 0, 0);

                if(parsing == false)
                {
                    CMD_RET(atOpId, ATERR_FS_CMCC_PARAM_INVALID);
                }

            }
            fp = fopen(real_path, "r");

            if(fp == NULL)
            {
                CMD_RET(atOpId, ATERR_FS_CMCC_FAIL_TO_OPEN);
            }

            if(fs_md5(fp, value) != 0)
            {
                fclose(fp);
                fp = NULL;
                CMD_RET(atOpId, ATERR_FS_CMCC_VALUE_INVALID);
            }
            fclose(fp);

            uint8_t string[128] = {0};
            snprintf((char *)string, 128, "\r\n+MFCHECK: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                     value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7],
                     value[8], value[9], value[10], value[11], value[12], value[13], value[14], value[15]);

            cm_uart_printf_withoutline(atOpId, string);
            break;
        }
        default:
        {
            CMD_RET(atOpId, ATERR_FS_CMCC_OPERATION_NOT_ALLOWED);
        }
    }
    CMD_RET(atOpId, RET_OK);
}


#endif