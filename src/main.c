#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curl/curl.h"
#include "curl/easy.h"
#include "cJSON.h"
#include "test.h"
#include "mbedtls/md5.h"
#include "uv.h"

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

int main(void)
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

    /*cjson test*/

    int json_code = cjson_test_main();

    /*mbedtls test*/
    int mbedtls_code = mbedtls_md5_self_test(25582);

    char *libcurl_version = curl_version();
    const char *uv_version = uv_version_string();
    /*print test code*/
    printf("######  curl version:%s\n", libcurl_version);
    printf("######  mbedtls code:%d\n", mbedtls_code);
    printf("######  cjson code  :%d\n", json_code);
    printf("######  uv version  :%s\n", uv_version);
    return 0; 
}