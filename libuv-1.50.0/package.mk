LIBUV_PATH = libuv-1.50.0
LIBUV_SOURCES =$(LIBUV_PATH)/src/fs-poll.c     \
	$(LIBUV_PATH)/src/idna.c                   \
	$(LIBUV_PATH)/src/inet.c                   \
	$(LIBUV_PATH)/src/strscpy.c                \
	$(LIBUV_PATH)/src/threadpool.c             \
	$(LIBUV_PATH)/src/timer.c                  \
	$(LIBUV_PATH)/src/strtok.c                 \
	$(LIBUV_PATH)/src/uv-data-getter-setters.c \
	$(LIBUV_PATH)/src/uv-common.c              \
	$(LIBUV_PATH)/src/version.c                \
	$(LIBUV_PATH)/src/unix/loop-watcher.c      \
	$(LIBUV_PATH)/src/unix/async.c             \
	$(LIBUV_PATH)/src/unix/core.c              \
	$(LIBUV_PATH)/src/unix/dl.c                \
	$(LIBUV_PATH)/src/unix/fs.c                \
	$(LIBUV_PATH)/src/unix/getaddrinfo.c       \
	$(LIBUV_PATH)/src/unix/getnameinfo.c       \
	$(LIBUV_PATH)/src/unix/loop.c              \
	$(LIBUV_PATH)/src/unix/pipe.c              \
	$(LIBUV_PATH)/src/unix/poll.c              \
	$(LIBUV_PATH)/src/unix/process.c           \
	$(LIBUV_PATH)/src/unix/signal.c            \
	$(LIBUV_PATH)/src/unix/stream.c            \
	$(LIBUV_PATH)/src/unix/tcp.c               \
	$(LIBUV_PATH)/src/unix/thread.c            \
	$(LIBUV_PATH)/src/unix/tty.c               \
	$(LIBUV_PATH)/src/unix/udp.c               \
	$(LIBUV_PATH)/src/unix/proctitle.c         \
	$(LIBUV_PATH)/src/unix/linux.c             \
	$(LIBUV_PATH)/src/unix/procfs-exepath.c    \

LIBUV_CFLAGS = -I$(LIBUV_PATH)/include     \
					-I$(LIBUV_PATH)/src            \
					-I$(LIBUV_PATH)/src/unix       \
					-D_LARGEFILE_SOURCE    \
					-D_FILE_OFFSET_BITS=64 \
					-DBUILDING_UV_SHARED=1 \
					-D_GNU_SOURCE          \

