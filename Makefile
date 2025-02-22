# 指定编译器
CC = gcc

# 源文件目录
SRC_DIR = src

# 头文件目录
INC_DIR = inc

# 对象文件目录
OBJ_DIR = output/obj

# 可执行文件目录
BIN_DIR = output

# 源文件列表（假设所有 .c 文件都需要编译）
SRC_FILES = $(SRC_DIR)/main.c
INCLUDES = -I$(INC_DIR)
################################################################
###########################Custom###############################
################################################################

# lwip
LWIP_ENABLE=n
ifeq ($(LWIP_ENABLE),y)
include ./lwip/package.mk
SRC_FILES += $(LWIP_SOURCES)
INCLUDES += $(LWIP_CFLAGS)
endif

# libuv-1.50.0
LIBUV_ENABLE=y
ifeq ($(LIBUV_ENABLE),y)
include ./libuv-1.50.0/package.mk
SRC_FILES += $(LIBUV_SOURCES)
INCLUDES += $(LIBUV_CFLAGS)
endif

# curl(暂时用的MR380M中的移植的curl)
LIBCURL_ENABLE=y
ifeq ($(LIBCURL_ENABLE),y)
include ./libcurl/package.mk
SRC_FILES += $(CURL_SOURCES)
INCLUDES += $(CURL_CFLAGS)
endif

# mbedtls-3.6.2
MBEDTLS_ENABLE=y
ifeq ($(MBEDTLS_ENABLE),y)
include ./mbedtls-3.6.2/package.mk
SRC_FILES += $(MBEDTLS_SOURCES)
INCLUDES += $(MBEDTLS_CFLAGS)
endif

# cJson
JSON_ENABLE=y
ifeq ($(JSON_ENABLE),y)
include ./libjson/package.mk
SRC_FILES += $(JSON_SOURCES)
INCLUDES += $(JSON_CFLAGS)
endif


# 生成的对象文件列表
# OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ_FILES  = $(SRC_FILES:%.c=%.o)


# 指定编译选项
CFLAGS = -g2			\
        -Wall			\
        -g				\
		$(INCLUDES)

# 最终的可执行文件
TARGET = $(BIN_DIR)/main

# 创建必要的目录
all: $(OBJ_DIR) $(BIN_DIR) $(TARGET)

# 创建对象文件目录
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 创建可执行文件目录
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# 编译规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# 链接规则
$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	@echo compile time: $(shell date)
	@$(CC) $(CFLAGS) $(OBJ_FILES) -o $@
# @rm -rf $(OBJ_FILES)
	@echo "###########################################################"
	@echo "###########################################################"
	@echo "##################### compile success #####################"
	@echo "############ Perform ./$(TARGET) verification #############"
	@echo "###########################################################"
	@echo "###########################################################"

# 清理生成的文件
clean clear:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(OBJ_FILES)
	@echo "###################### clean success ######################"
# 默认目标
.PHONY: all clean clear