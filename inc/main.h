/**适配平台log打印函数*/
#define LOG(fmt, args...)               printf("[LOG] function:%s\tLine(%d)\n "fmt"\r\n",__func__, __LINE__, ##args)
#define LOG_ERR(fmt, args...)           printf("[LOG_ERR]function:%s\tLine(%d)\n "fmt"\r\n",__func__, __LINE__, ##args)