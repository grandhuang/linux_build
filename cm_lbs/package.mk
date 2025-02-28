LBS_PATH=cm_lbs

LBS_SOURCES = \
			$(LBS_PATH)/cm_lbs/src/coordinate.c					\
			$(LBS_PATH)/cm_lbs/src/cm_lbs_oneospos.c			\
			$(LBS_PATH)/cm_lbs/src/cm_lbs_amap.c				\
			$(LBS_PATH)/cm_lbs/src/cm_lbs_api.c					\
			$(LBS_PATH)/cm_lbs/src/cm_lbs.c						\
			$(LBS_PATH)/cm_lbs_platform/cm_lbs_platform.c		

LBS_CFLAGS = -I$(LBS_PATH) -I$(LBS_PATH)/cm_lbs/inc -I$(LBS_PATH)/cm_lbs_platform 