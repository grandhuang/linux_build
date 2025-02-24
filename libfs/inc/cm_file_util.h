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

#ifndef __CM_FILE_UTIL_H__
#define __CM_FILE_UTIL_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "cm_file_platform.h"
#include "assert.h"
#include "mbedtls/md5.h"
#include "uv.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef enum
{
    FS_CMCC_INFMT_ASC = 0,              //ASCII字符串
    FS_CMCC_INFMT_HEX,                  //HEX字符串
    FS_CMCC_INFMT_ESC,                  //转义字符串escape string
    FS_CMCC_INFMT_MAX
} FS_CMCC_INFMT_E;

typedef enum
{
    FS_CMCC_OUTFMT_ORG = 0,              //原始字符串
    FS_CMCC_OUTFMT_HEX,                  //HEX字符串
    FS_CMCC_OUTFMT_MAX
} FS_CMCC_OUTFMT_E;

typedef struct file_open_info
{
    uint32_t fHandle;               //打开的文件对应的句柄，从2开始
    FILE  *fp;                  //打开的文件指针
    uint32_t fMode;                 //打开文件的模式
    uint32_t fWriteLen;             //文件打开后fwrite的总长度
    //char fname[LFS_NAME_MAX+1];     //文件名
    struct file_open_info *next;
    char fname[0];
} FS_OpenFileInfo_t;

typedef struct{
    uv_timer_t *timerid;
}cm_timer_id_t;

typedef struct fs_passthrough_info
{
    FILE *fp;                   //文件句柄
    int32_t result;             //文件操作结果
    uint32_t waittime;          //等待超时时间
    uint32_t ackmode;           //响应模式
    uint32_t checksum;          //校验值
    uint32_t recvlen;           //已接收的长度
    uint32_t maxlen;            //最大目标长度
    cm_timer_id_t timer;            //定时器
    FS_OpenFileInfo_t *fInfo;
} FS_PassThroughInfo_t;

typedef struct file_open_info_list
{
    FS_OpenFileInfo_t *node;    //数据节点
    uint32_t num;               //现有节点个数
    uint32_t maxNum;            //最大节点个数
} FS_OpenFileInfo_list;

typedef enum
{
    FS_CMCC_VERIFY_MD5 = 0,              //Md5校验
    FS_CMCC_VERIFY_SHA,                  //SHA校验
    FS_CMCC_VERIFY_SHA256,               //SHA256校验
    FS_CMCC_VERIFY_CRC,                  //CRC校验
    FS_CMCC_VERIFY_MAX
} FS_CMCC_VERIFYALG_E;
/* struct lfs_info {
    // Type of the file, either LFS_TYPE_REG or LFS_TYPE_DIR
    uint8_t type;

    // Size of the file, only valid for REG files. Limited to 32-bits.
    lfs_size_t size;

    // Name of the file stored as a null-terminated string. Limited to
    // LFS_NAME_MAX+1, which can be changed by redefining LFS_NAME_MAX to
    // reduce RAM. LFS_NAME_MAX is stored in superblock and must be
    // respected by other littlefs drivers.
    char name[LFS_NAME_MAX+1];
};

typedef struct lfs_info fs_info_t;

typedef struct file_list{
    fs_info_t finfo;
    struct file_list* next;
} FILE_LIST_NODE;

typedef struct {
    FILE_LIST_NODE *node;
    uint32_t count;
} FILE_LIST; */
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
 * @brief 写NV
 *
 * @param [in] filename                 文件名
 * @param [in] data                     数据
 * @param [in] datalen                  数据长度
 *
 * @return <0 失败；其他 实际写入长度
 *
 * @details 用于保存配置参数
 */
int cm_nv_write(const char *filename, char *data, uint32_t datalen);

/**
 * @brief 读NV
 *
 * @param [in] filename                 文件名
 * @param [in] data                     缓存
 * @param [in] datalen                  缓存长度
 *
 * @return <0 失败；其他 实际读取长度
 *
 * @details 用于读取配置参数
 */
int cm_nv_read(const char *filename, char *data, uint32_t datalen);

/**
 * @brief 写NV
 *
 * @param [in] filename                 文件名
 * @param [in] data                     数据
 * @param [in] datalen                  数据长度
 *
 * @return <0 失败；其他 实际写入长度
 *
 * @details 用于保存配置参数(factory,仅可以存放2M)
 */
int cm_factory_nv_write(const char *filename, char *data, uint32_t datalen);

/**
 * @brief 读NV
 *
 * @param [in] filename                 文件名
 * @param [in] data                     缓存
 * @param [in] datalen                  缓存长度
 *
 * @return <0 失败；其他 实际读取长度
 *
 * @details 用于读取配置参数(factory,仅可以存放2M)
 */
int cm_factory_nv_read(const char *filename, char *data, uint32_t datalen);


/**
 * @brief 打开的文件插入链表
 *
 * @param [in] list                 链表
 * @param [in] newnode              节点
 *
 * @return 节点下标
 *
 * @details 打开的文件插入链表
 */
uint32_t insert_openflist(FS_OpenFileInfo_list *list, FS_OpenFileInfo_t *newnode);

/**
 * @brief 获取文件节点
 *
 * @param [in] list                 链表
 * @param [in] handle               下标
 *
 * @return 文件节点
 *
 * @details 获取文件节点
 */
FS_OpenFileInfo_t *find_openflist_byHandle(FS_OpenFileInfo_list *list, uint32_t handle);

/**
 * @brief 关闭已打开的文件
 *
 * @param [in] list                 链表
 * @param [in] fname                文件名
 *
 * @return 文件节点
 *
 * @details 关闭已打开的文件
 */
FS_OpenFileInfo_t *close_openflist_byFname(FS_OpenFileInfo_list *list, char *fname);

/**
 * @brief 删除文件节点
 *
 * @param [in] list                 链表
 * @param [in] handle               下标
 *
 * @return 
 *
 * @details 删除文件节点
 */
void remove_openflist(FS_OpenFileInfo_list *list, uint32_t handle);

/**
 * @brief 根据文件指针获取当前文件大小
 *
 * @param [in] fp               文件句柄
 *
 * @return 文件大小
 *
 * @details
 */
uint32_t cm_file_get_size_by_file_point(FILE *fp);

/**
 * @brief 判断当前文件位置指针移动后是否越界
 *
 * @param [in] fp               文件句柄
 * @param [in] offset           偏移量
 * @param [in] dir              起始位置
 *
 * @return true/false
 *
 * @details 判断当前文件位置指针移动后是否越界
 */
bool is_fs_OverFlow(FILE *fp, const int32_t offset, const uint32_t dir);

/**
 * @brief 检测结尾是否为/
 *
 * @param [in] fname                 路径
 *
 * @return 
 *
 * @details 检测结尾是否为
 */
bool fs_check_dir(const char *fname);

/**
 * @brief md5校验
 *
 * @param [in] fp               文件句柄
 * @param [out] output          校验码
 *
 * @return 0 成功
 *
 * @details md5校验
 */
int fs_md5(FILE *fp, unsigned char *output);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif