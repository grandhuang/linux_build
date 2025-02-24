#ifndef __CM_FILE_AT_CMD__H__
#define __CM_FILE_AT_CMD__H__



#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#if 0 
unsigned int at_MFCFG_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFSINFO_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFLIST_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFSIZE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFPUT_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFGET_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFOPEN_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFREAD_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFWRITE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFSYNC_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFSEEK_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFTRUNC_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFCLOSE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFDELETE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFMOVE_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);
unsigned int at_MFCHECK_req(unsigned short atOpId, unsigned short modemId, const char *cmdName, AT_CmdType cmdType, const AT_CmdParaList *paraList);

#define AT_MFCFG_CMD     {"+MFCFG", AT_CMD_ATTR_NONE,NULL,at_MFCFG_req}
#define AT_MFSINFO_CMD   {"+MFSINFO", AT_CMD_ATTR_NONE,NULL,at_MFSINFO_req}
#define AT_MFLIST_CMD    {"+MFLIST", AT_CMD_ATTR_NONE,NULL,at_MFLIST_req}
#define AT_MFSIZE_CMD    {"+MFSIZE", AT_CMD_ATTR_NONE,NULL,at_MFSIZE_req}
#define AT_MFPUT_CMD     {"+MFPUT", AT_CMD_ATTR_NONE,NULL,at_MFPUT_req}
#define AT_MFGET_CMD     {"+MFGET", AT_CMD_ATTR_NONE,NULL,at_MFGET_req}
#define AT_MFOPEN_CMD    {"+MFOPEN", AT_CMD_ATTR_NONE,NULL,at_MFOPEN_req}
#define AT_MFREAD_CMD    {"+MFREAD", AT_CMD_ATTR_NONE,NULL,at_MFREAD_req}
#define AT_MFWRITE_CMD   {"+MFWRITE", AT_CMD_ATTR_NONE,NULL,at_MFWRITE_req}
#define AT_MFSYNC_CMD    {"+MFSYNC", AT_CMD_ATTR_NONE,NULL,at_MFSYNC_req}   //命令不支持
#define AT_MFSEEK_CMD    {"+MFSEEK", AT_CMD_ATTR_NONE,NULL,at_MFSEEK_req}
#define AT_MFTRUNC_CMD   {"+MFTRUNC", AT_CMD_ATTR_NONE,NULL,at_MFTRUNC_req}   //命令不支持
#define AT_MFCLOSE_CMD   {"+MFCLOSE", AT_CMD_ATTR_NONE,NULL,at_MFCLOSE_req}
#define AT_MFDELETE_CMD  {"+MFDELETE", AT_CMD_ATTR_NONE,NULL,at_MFDELETE_req}
#define AT_MFMOVE_CMD    {"+MFMOVE", AT_CMD_ATTR_NONE,NULL,at_MFMOVE_req}
#define AT_MFCHECK_CMD   {"+MFCHECK", AT_CMD_ATTR_NONE,NULL,at_MFCHECK_req}

#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif