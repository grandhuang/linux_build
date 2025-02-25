/**
 * @file        cm_file_util.c
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
/* #include "stdint.h"
#include "stddef.h"
#include <string.h> */
#include "cm_file_util.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
// #define CM_NV_DIR           "online/vendor/"
#define CM_NV_DIR           "./"
#define CM_FACTORY_NVDIR    "productinfo/"

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
int cm_nv_write(const char *filename, char *data, uint32_t datalen)
{
    char real_name[128] = {0};

    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        printf("filename/write data/datalen error or strlen(filename) > 64\n");
        return -1;
    }

    sprintf(real_name, "%s%s", CM_NV_DIR, filename);
    //int fd = open(real_name, O_CREAT | O_RDWR | O_TRUNC);
    FILE *fd = fopen(real_name, "wb+");

    if (fd == NULL)
    {
        printf("open %s fail\n", real_name);
        CM_FILE_LOGE("open %s fail", real_name);
        return -1;
    }

    //int ret = write(fd, data, datalen);
    int ret = fwrite(data, datalen, 1, fd);
    if(ret != 1)
    {
        printf("cm_nv write error");
        CM_FILE_LOGE("cm_nv write error");
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
int cm_nv_read(const char *filename, char *data, uint32_t datalen)
{
    char real_name[128] = {0};

    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        printf("filename/write data/datalen error or strlen(filename) > 64\n");
        return -1;
    }

    sprintf(real_name, "%s%s", CM_NV_DIR, filename);
    FILE *fd = fopen(real_name, "r");

    if (fd == NULL)
    {
        printf("open %s fail\n", real_name);
        CM_FILE_LOGE("open %s fail", real_name);
        return -1;
    }

    int ret = fread(data, datalen, 1, fd);
    if(ret != 1)
    {
        printf("nv_read fread error");
        CM_FILE_LOGE("nv_read fread error");
        return -1;
    }
    else
    {
        ret = datalen;
    }
    printf("nv fread %d", ret);
    CM_FILE_LOGI("nv fread %d", ret);
    fclose(fd);
    return ret;
}

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
int cm_factory_nv_write(const char *filename, char *data, uint32_t datalen)
{
    char real_name[128] = {0};

    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        printf("filename/write data/datalen error or strlen(filename) > 64\n");
        return -1;
    }

    sprintf(real_name, "%s%s", CM_FACTORY_NVDIR, filename);
    //int fd = open(real_name, O_CREAT | O_RDWR | O_TRUNC);
    FILE *fd = fopen(real_name, "wb+");

    if (fd == NULL)
    {
        printf("factory_nv_read open %s fail\n", real_name);
        CM_FILE_LOGE("factory_nv_read open %s fail", real_name);
        return -1;
    }

    //int ret = write(fd, data, datalen);
    int ret = fwrite(data, datalen, 1, fd);
    printf("cm_factory_nv_write fwrite %s", real_name);
    CM_FILE_LOGE("cm_factory_nv_write fwrite %s", real_name);
    if(ret != 1)
    {
        printf("cm_nv write error");
        CM_FILE_LOGE("cm_nv write error");
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
int cm_factory_nv_read(const char *filename, char *data, uint32_t datalen)
{
    char real_name[128] = {0};

    if(filename == NULL || data == NULL || datalen == 0 || strlen(filename) > 64)
    {
        printf("filename/write data/datalen error or strlen(filename) > 64\n");
        return -1;
    }

    sprintf(real_name, "%s%s", CM_FACTORY_NVDIR, filename);
    FILE *fd = fopen(real_name, "r");

    if (fd == NULL)
    {
        printf("factory_nv_read open %s fail\n", real_name);
        CM_FILE_LOGE("factory_nv_read open %s fail", real_name);
        return -1;
    }

    int ret = fread(data, datalen, 1, fd);
    if(ret != 1)
    {
        printf("factory_nv_read fread error");
        CM_FILE_LOGE("factory_nv_read fread error");
        return -1;
    }
    else
    {
        ret = datalen;
    }
    printf("factory fread %d", ret);
    CM_FILE_LOGI("factory fread %d", ret);
    fclose(fd);
    return ret;
}


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
uint32_t insert_openflist(FS_OpenFileInfo_list *list, FS_OpenFileInfo_t *newnode)
{
    FS_OpenFileInfo_t *head = list->node;

    if(head == NULL)
    {
        newnode->fHandle = FS_OPENFILE_HANDLE_START;
        list->node = newnode;
        list->num++;
    }
    else
    {
        while(head->next != NULL && (head->fHandle + 1 == head->next->fHandle))
        {
            head = head->next;
        }

        /*
        FS_OpenFileInfo_t *tmpNode = xy_malloc(sizeof(FS_OpenFileInfo_t));
        memset(tmpNode, 0, sizeof(FS_OpenFileInfo_t));
        memcpy(tmpNode, node, sizeof(FS_OpenFileInfo_t));

        tmpNode->next = head->next;
        head->next = tmpNode;
        tmpNode->fHandle = head->fHandle + 1;
        handle = tmpNode->fHandle;
        list->num++;
        */
        newnode->next = head->next;
        head->next = newnode;
        newnode->fHandle = head->fHandle + 1;
        list->num++;
    }

    return newnode->fHandle;
}

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
FS_OpenFileInfo_t *find_openflist_byHandle(FS_OpenFileInfo_list *list, uint32_t handle)
{
    FS_OpenFileInfo_t *index = list->node;

    if(index == NULL)
    {
        return NULL;
    }
    else
    {
        while(index != NULL)
        {
            if(index->fHandle == handle)
            {
                break;
            }

            index = index->next;
        }
    }

    return index;
}

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
FS_OpenFileInfo_t *close_openflist_byFname(FS_OpenFileInfo_list *list, char *fname)
{
    FS_OpenFileInfo_t *index = list->node;
    FS_OpenFileInfo_t *note = NULL;

    if(index == NULL || fname == NULL)
    {
        return NULL;
    }
    else
    {
        while(index != NULL)
        {
            if(strcmp(index->fname, fname) == 0)
            {
                fclose(index->fp);
                note = index;
                if(index == list->node)
                {
                    list->node = index->next;
                }
                index = index->next;
                free(note);
                list->num--;
            }
            else
            {
                index = index->next;
            }
        }
    }

    return index;
}

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
void remove_openflist(FS_OpenFileInfo_list *list, uint32_t handle)
{
    FS_OpenFileInfo_t *head = list->node;
    FS_OpenFileInfo_t *curNode = NULL;

    if(head != NULL)
    {
        if(head->fHandle == handle)
        {
            list->node = head->next;
            list->num--;
            free(head);
        }
        else
        {
            curNode = head->next;

            while(curNode != NULL)
            {
                if(curNode->fHandle == handle)
                {
                    head->next = curNode->next;
                    list->num--;
                    free(curNode);
                    break;
                }
                else
                {
                    head = curNode;
                    curNode = curNode->next;
                }
            }
        }
    }
}

/**
 * @brief 根据文件指针获取当前文件大小
 *
 * @param [in] fp               文件句柄
 *
 * @return 文件大小
 *
 * @details
 */
uint32_t cm_file_get_size_by_file_point(FILE *fp)
{
    uint32_t pos = ftell(fp); // 获取文件指针的位置
    fseek(fp, 0, SEEK_END); // 将文件指针移动到文件末尾
    uint32_t size = ftell(fp); // 获取文件指针的位置，即文件大小
    fseek(fp, pos, SEEK_SET);

    return size;
}

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
bool is_fs_OverFlow(FILE *fp, const int32_t offset, const uint32_t dir)
{
    uint32_t curPos = 0;
    uint32_t fsize = 0;


    //判断位置是否越界
    fsize = cm_file_get_size_by_file_point(fp);
    curPos = ftell(fp);

    if(dir == 0)
    {
        if(offset < 0 || offset > fsize)
        {
            return true;
        }
    }
    else if(dir == 1)
    {
        if(offset + curPos > fsize || offset + curPos < 0)
        {
            return true;
        }
    }
    else
    {
        if(offset > 0 || (fsize + offset) < 0)
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief 检测结尾是否为/
 *
 * @param [in] fname                 路径
 *
 * @return 
 *
 * @details 检测结尾是否为
 */
bool fs_check_dir(const char *fname)
{
    assert(fname != NULL);
    uint32_t len = strlen(fname);

    if(*(fname + len - 1) == '/')
    {
        return true;
    }
    else
    {
        return false;
    }
}

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
int fs_md5(FILE *fp, unsigned char *output)
{
    int ret = 0;
    uint32_t readLen = 0;
    unsigned char data[16] =  {0};
    char *buff = malloc(FS_READBUFF_LEN);
    mbedtls_md5_context ctx;

    mbedtls_md5_init( &ctx );

    if((ret = mbedtls_md5_starts( &ctx ) ) != 0 )
    {
        goto exit;
    }

    while((readLen = fread(buff, FS_READBUFF_LEN, 1, fp)) > 0)
    {
        if( ( ret = mbedtls_md5_update( &ctx, (const unsigned char *)buff, readLen) ) != 0 )
        {
            goto exit;
        }
    }

    if( ( ret = mbedtls_md5_finish( &ctx, data) ) != 0 )
    {
        goto exit;
    }


    memcpy(output, data, 16);

exit:
    mbedtls_md5_free( &ctx );
    free(buff);
    return( ret );
}
