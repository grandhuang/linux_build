/**
 * @file        cm_file_platform.c
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
# include <sys/statvfs.h>

# include "cm_file_platform.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Functions
 ****************************************************************************/

/**
 * @brief 获取文件系统总大小和当前可用大小
 *
 * @param [in] diskpath               文件系统路径
 *
 * @return 小于0 失败；  0 成功
 *
 * @details 文件系统总大小和当前可用大小的结构体
 */
int32_t cm_fsize_info(const char *diskpath, fs_size_info_t *file_size)
{
    int ret = 0;

    struct statvfs stat;

    if (statvfs(diskpath, &stat) == -1)
    {
        return -1;
    }

    CM_FILE_LOG("stat.f_frsize = %ld", stat.f_frsize);

    //获取总大小
    file_size->file_system_total_size = stat.f_frsize * (unsigned long long)stat.f_blocks;

    // 获取可用大小
    file_size->file_system_remain_size = stat.f_frsize * (unsigned long long)stat.f_bavail;

    return ret;

}