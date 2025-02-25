#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "test.h"
#include "mbedtls/md5.h"
#include "uv.h"
#include "cm_file_util.h"

// 用于保存HTTP响应体的缓冲区
struct MemoryStruct
{
    char *memory;
    size_t size;
};

// 写入响应体的回调函数
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        // 内存分配失败，返回0表示失败
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}
void write_to_file(char *filename, const char *data)
{
    // 打开文件以写入头部信息
    FILE *fd = fopen(filename, "w");
    if (fd == NULL)
    {
        perror("Failed to open file for writing");
    }
    else
    {
        // 将头部信息写入文件
        fputs(data, fd);

        // 注意：在真实情况下，你应该从curl的头部回调中收集头部信息，
        // 并在这里格式化它们。由于我们没有实际的头部回调示例，
        // 所以我们在这里只是写入了一个模拟的头部字符串。

        // 关闭文件
        fclose(fd);
    }
}

uint32_t file_write(void)
{
    char data[32] = "Hello world!";
    if(cm_nv_write("file_test.txt", (char *)data, sizeof(data)) < 0)
        {
            printf("write to file %s filed\n", data);
            // CM_FILE_LOG("write to file %s filed", data);
            return -1;
        }
    return 0;
}

uint32_t file_read(void)
{
    char data[32] = {0};
    if(cm_nv_read("file_test.txt", (char *)data, sizeof(data)) < 0)
    {
        printf("read from file %s filed\n", data);
        // CM_FILE_LOG("read from file %s filed", data);
        return -1;
    }
    return 0;
}
void curl_test(void)
{
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1); // 初始化内存块
    chunk.size = 0;           // 没有数据

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    if (curl_handle)
    {
        curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.baidu.com");

        // 设置写入回调函数
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        // 设置用户数据传递给回调函数
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        // 执行请求
        res = curl_easy_perform(curl_handle);

        // 检查请求是否成功
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            // 获取响应头信息
            long response_code;
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            printf("HTTP Response Code: %ld\n", response_code);

// 打印响应头信息（可选）
#if 0
            struct curl_slist *headers;
            curl_easy_getinfo(curl_handle, CURLINFO_HEADER_OUT, headers);
            printf("hedears data:%s\n", (headers->data));
            if (headers)
            {
                struct curl_slist *chunk = headers;
                while (chunk)
                {
                    printf("trunk data:%s\n", chunk->data);
                    chunk = chunk->next;
                    printf("trunk to data success.\n");
                }
            }
#endif

            // 解析响应体（假设是JSON格式）
            if (chunk.memory == NULL)
            {
                fprintf(stderr, "Failed to allocate memory for response body.\n");
            }
            else
            {
                chunk.memory[chunk.size] = '\0';
                cJSON *root = cJSON_Parse(chunk.memory);
                printf("[cJSON_Parse] success.\n");
                if (root == NULL)
                {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL)
                    {
                        fprintf(stderr, "Error before: %s\n", error_ptr);
                        // write_to_file("1.txt", (char *)error_ptr);
                    }
                }
                else
                {
                    // 打印解析后的JSON字符串
                    char *json_string = cJSON_Print((const cJSON *)root);
                    printf("Parsed JSON:\n%s\n", json_string);
                    // write_to_file("2.txt", json_string);//Write to file
                    cJSON_Delete((cJSON *)json_string); // 释放打印生成的字符串
                    cJSON_Delete((cJSON *)root);        // 释放解析生成的JSON对象
                }
            }
        }

        // 清理
        curl_easy_cleanup(curl_handle);
        free(chunk.memory);
    }

    curl_global_cleanup();
}

/*
*@brief 写入文件(必选)
*
*@param[in] file_path 文件路径
*@param[in] file_data 需要写入数据
*@param[in] size  需要写入数据长度
*
*@return 成功:0 失败：-1
*/
int __cm_lbs_FileWrite(const char *file_path, void *file_data, uint32_t size)
{
#if 0
    int ret = 0;
    int fd = cm_nv_write(file_path, (char *)file_data, size);
    if(fd < 0)
    {
        printf("error:%d fd = %d", errno, fd);
        ret = -1;
    }
    return ret;

#else
    int ret = 0;
    char real_path[128] = {0};
    if(file_path == NULL || file_data == NULL || size == 0 || strlen(file_path) > 64)
    {
        printf("error:file_path or file_data is null");
        return -1;
    }
    sprintf(real_path, "%s%s", "./", file_path);
    FILE *fd = fopen(real_path, "wb+");
    if(fd == NULL)
    {
        printf("fopen %s fail", real_path);
        return -1;
    }

    int res = fwrite(file_data, size, 1, fd);
    if(res != 1)
    {
        printf("fwrite to %s error", real_path);
        return -1;
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
*@return 成功:0 失败：-1
*/
int __cm_lbs_FileRead(const char *file_path, void *file_rdata, uint32_t read_size)
{
#if 0

    int ret = 0;
    int fd = cm_nv_read(file_path, (char *)file_rdata, read_size);
    if(fd <= 0)//读到0个数据，说明文件不存在或权限不足
    {
        printf("error:%d fd = %d", errno, fd);
        ret = -1;
    }
    return ret;

#else
    int ret = 0;
    char real_path[128] = {0};
    if(file_path == NULL || file_rdata == NULL || read_size == 0 ||strlen(file_path) > 64)
    {
        printf("error:file_path or file_data is null");
        return -1;
    }

    sprintf(real_path, "%s%s", "./", file_path);
    FILE *fd = fopen(file_path, "r");
    if(fd  == NULL)
    {
        printf("fopen %s fail", real_path);
        return -1;
    }
    int res = fread((char *)file_rdata, read_size, 1, fd);
    if(res != 1)
    {
        printf("fread from %s fail", real_path);
        return -1;
    }
    else
    {
        ret = read_size;
    }
    fclose(fd);
    return ret;
#endif
}

#define CM_LBS_ATCMD_CONFIG_FILE_PATH  "./atcmd_lbscfg" 

typedef struct
{
    uint32_t    age;
    uint32_t    height;
    char*       name[32];
}info_t;

typedef struct
{
    uint32_t    user_id;
    info_t      info;
}cfg_t;


int cm_lbs_nv_test(void)
{
    LOG("");
    cfg_t *atcmd_cfg = (cfg_t *)malloc(sizeof(cfg_t));
    cfg_t *read_cfg = (cfg_t *)malloc(sizeof(cfg_t));
    
    atcmd_cfg->user_id = 10001;
    atcmd_cfg->info.age = 25;
    atcmd_cfg->info.height = 175;
    memcpy(atcmd_cfg->info.name, "Mike", sizeof("Mike"));

    LOG("atcmd_cfg init success");

    // 打印结果以验证
    printf("################### write cfg ###################\n");
    printf("User ID: %d\n", atcmd_cfg->user_id);
    printf("Age: %d\n", atcmd_cfg->info.age);
    printf("Height: %d\n", atcmd_cfg->info.height);
    printf("Name: %s\n", (char*)(atcmd_cfg->info.name));

    
    int ret_write = __cm_lbs_FileWrite(CM_LBS_ATCMD_CONFIG_FILE_PATH, (void *)atcmd_cfg, sizeof(cfg_t));
    if(ret_write != 0)
    {
        LOG("write cfg success\nret_write = %d", ret_write);
    }
    

    int ret_read = __cm_lbs_FileRead(CM_LBS_ATCMD_CONFIG_FILE_PATH, (void *)read_cfg, sizeof(cfg_t));
    if(ret_read != 0)
    {
        LOG("read cfg success\nret_read = %d", ret_read);
    }
    
    // 打印结果以验证
    printf("#################### read cfg ##################\n");
    printf("User ID: %d\n", read_cfg->user_id);
    printf("Age: %d\n", read_cfg->info.age);
    printf("Height: %d\n", read_cfg->info.height);
    printf("Name: %s\n", (char*)(read_cfg->info.name));

    int ret = (ret_read == ret_write) ? 0 : -1;

    free(atcmd_cfg);
    free(read_cfg);
    return ret;
}

int main(void)
{

#if 0
    /*cjson test*/
    int json_code = cjson_test_main();

    /*mbedtls test*/
    int mbedtls_code = mbedtls_md5_self_test(25582);

    /*libcurl test*/
    char *libcurl_version = curl_version();

    /*libuv test*/
    const char *uv_version = uv_version_string();

    /*libfs test*/
    uint32_t libfs_write_code = file_write();
    uint32_t libfs_read_code = file_read();

 

    /*print test code*/
    printf("######  curl version:       %s\n", libcurl_version);
    printf("######  mbedtls code:       %d\n", mbedtls_code);
    printf("######  cjson code:         %d\n", json_code);
    printf("######  uv version:         %s\n", uv_version);
    printf("######  libfs_write_code :  %d\n", libfs_write_code);
    printf("######  libfs_read_code:    %d\n", libfs_read_code);
#endif

    /*cm_lbs nv file test*/
    int nv_code =  cm_lbs_nv_test();

    printf("######  cm_lbs_nv_code:     %d\n", nv_code);

    return 0; 
}