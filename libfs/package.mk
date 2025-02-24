LIBFS_DIR = libfs
LIBFS_SOURCES += 	$(LIBFS_DIR)/cm_file_at_cmd.c \
					$(LIBFS_DIR)/src/cm_file_util.c \
					$(LIBFS_DIR)/src/cm_file_platform.c

LIBFS_CFLAGS += -I$(LIBFS_DIR)/inc	\
				-I$(LIBFS_DIR)