/**
 * @file        cm_file_platform.h
 * @brief       文件系统适配平台接口
 * @copyright   Copyright © 2021 China Mobile IOT. All rights reserved.
 * @author
 * @date
 *
 * @defgroup
 * @ingroup
 * @{
 */

#ifndef __CM_FILE_PLATFORM_H__
#define __CM_FILE_PLATFORM_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define FS_FILENAME_MAX_LEN         128          //输入最大文件名字,含路径
#define FS_DEFAULT_WAITTIME         10           //默认超时时间
#define FS_DEFAULT_FRAG_SIZE        8192         //默认分片大小
#define FS_DEFAULT_FRAG_INTERVAL    200          //默认分片间隔时间ms
#define FS_READBUFF_LEN             8

#define USER_DISK_SYM                       "online/vendor/filesys"//文件系统存储的路径
#define CM_FILE_FD_ERROR  NULL

#define FS_OPENFILE_ARRAY_LEN       8       //记录打开文件信息的数组长度
#define FS_OPENFILE_HANDLE_START    1       //打开的文件对应的句柄起始值

/**适配平台log打印函数*/
#define CM_FILE_LOG(fmt, args...)       printf("[CM_FILE]%s(%d) "fmt"\r\n",__func__, __LINE__, ##args)
#define CM_FILE_LOGI(fmt, args...)      printf("[CM_FILE_I] "fmt"\r\n", ##args)
#define CM_FILE_LOGW(fmt, args...)      printf("[CM_FILE_W] "fmt"\r\n", ##args)
#define CM_FILE_LOGE(fmt, args...)      printf("[CM_FILE_E] "fmt"\r\n", ##args)
/****************************************************************************
 * Public Types
 ****************************************************************************/

/**文件系统结果码*/
typedef enum
{
    ATERR_FS_CMCC_OPERATION_NOT_ALLOWED = 3,        //3操作不允许
    ATERR_FS_CMCC_PARAM_INVALID = 50,               //50非法参数
    ATERR_FS_CMCC_VALUE_INVALID = 1050,             //1050文件系统错误
    ATERR_FS_CMCC_FILE_NOEXIST,                     //1051文件不存在
    ATERR_FS_CMCC_FILE_EXIST,                       //1052文件已存在
    ATERR_FS_CMCC_FILE_TOO_LARGE,                   //1053文件过大
    ATERR_FS_CMCC_MAXNUM_TO_OPEN,                   //1054文件达到允许打开的最大数
    ATERR_FS_CMCC_FAIL_TO_OPEN,                     //1055文件打开失败
    ATERR_FS_CMCC_FAIL_REOPEN,                      //1056文件已被打开
    ATERR_FS_CMCC_FAIL_TO_WRITE,                    //1057文件写入失败
    ATERR_FS_CMCC_FAIL_TO_READ,                     //1058文件读取失败
    ATERR_FS_CMCC_READ_ONLY,                        //1059文件只允许读
    ATERR_FS_CMCC_DESCRIPTOR_INVALID,               //1060文件描述符无效
    ATERR_FS_CMCC_FAIL_TO_DELETE,                   //1061文件删除失败
    ATERR_FS_CMCC_FAIL_TO_LIST,                     //1062文件列举失败
    ATERR_FS_CMCC_NO_SPACE,                         //1063文件系统空间不足
    ATERR_FS_CMCC_TIMEOUT,                          //1064文件操作超时
    ATERR_FS_CMCC_MAX_ERR
} AT_FS_CMCC_ERRNO_E;

typedef struct  FS_SIZE_INFO
{
    uint32_t file_system_total_size;
    uint32_t file_system_remain_size;

} fs_size_info_t;
/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 * @brief 获取文件系统总大小和当前可用大小
 *
 * @param [in] diskpath               文件系统路径
 *
 * @return 小于0 失败
 *
 * @details 文件系统总大小和当前可用大小的结构体
 */
int32_t cm_fsize_info(const char *diskpath, fs_size_info_t *file_size);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif