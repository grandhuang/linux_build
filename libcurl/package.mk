
CURL_CUR_DIR=libcurl

CURL_SOURCES = \
  $(CURL_CUR_DIR)/lib/altsvc.c           \
  $(CURL_CUR_DIR)/lib/amigaos.c          \
  $(CURL_CUR_DIR)/lib/asyn-ares.c        \
  $(CURL_CUR_DIR)/lib/asyn-thread.c      \
  $(CURL_CUR_DIR)/lib/base64.c           \
  $(CURL_CUR_DIR)/lib/bufq.c             \
  $(CURL_CUR_DIR)/lib/bufref.c           \
  $(CURL_CUR_DIR)/lib/c-hyper.c          \
  $(CURL_CUR_DIR)/lib/cf-h1-proxy.c      \
  $(CURL_CUR_DIR)/lib/cf-h2-proxy.c      \
  $(CURL_CUR_DIR)/lib/cf-haproxy.c       \
  $(CURL_CUR_DIR)/lib/cf-https-connect.c \
  $(CURL_CUR_DIR)/lib/cf-socket.c        \
  $(CURL_CUR_DIR)/lib/cfilters.c         \
  $(CURL_CUR_DIR)/lib/conncache.c        \
  $(CURL_CUR_DIR)/lib/connect.c          \
  $(CURL_CUR_DIR)/lib/content_encoding.c \
  $(CURL_CUR_DIR)/lib/cookie.c           \
  $(CURL_CUR_DIR)/lib/curl_addrinfo.c    \
  $(CURL_CUR_DIR)/lib/curl_des.c         \
  $(CURL_CUR_DIR)/lib/curl_endian.c      \
  $(CURL_CUR_DIR)/lib/curl_fnmatch.c     \
  $(CURL_CUR_DIR)/lib/curl_get_line.c    \
  $(CURL_CUR_DIR)/lib/curl_gethostname.c \
  $(CURL_CUR_DIR)/lib/curl_gssapi.c      \
  $(CURL_CUR_DIR)/lib/curl_memrchr.c     \
  $(CURL_CUR_DIR)/lib/curl_multibyte.c   \
  $(CURL_CUR_DIR)/lib/curl_ntlm_core.c   \
  $(CURL_CUR_DIR)/lib/curl_range.c       \
  $(CURL_CUR_DIR)/lib/curl_rtmp.c        \
  $(CURL_CUR_DIR)/lib/curl_sasl.c        \
  $(CURL_CUR_DIR)/lib/curl_sha512_256.c  \
  $(CURL_CUR_DIR)/lib/curl_sspi.c        \
  $(CURL_CUR_DIR)/lib/curl_threads.c     \
  $(CURL_CUR_DIR)/lib/curl_trc.c         \
  $(CURL_CUR_DIR)/lib/cw-out.c           \
  $(CURL_CUR_DIR)/lib/dict.c             \
  $(CURL_CUR_DIR)/lib/dllmain.c          \
  $(CURL_CUR_DIR)/lib/doh.c              \
  $(CURL_CUR_DIR)/lib/dynbuf.c           \
  $(CURL_CUR_DIR)/lib/dynhds.c           \
  $(CURL_CUR_DIR)/lib/easy.c             \
  $(CURL_CUR_DIR)/lib/easygetopt.c       \
  $(CURL_CUR_DIR)/lib/easyoptions.c      \
  $(CURL_CUR_DIR)/lib/escape.c           \
  $(CURL_CUR_DIR)/lib/file.c             \
  $(CURL_CUR_DIR)/lib/fileinfo.c         \
  $(CURL_CUR_DIR)/lib/fopen.c            \
  $(CURL_CUR_DIR)/lib/formdata.c         \
  $(CURL_CUR_DIR)/lib/ftp.c              \
  $(CURL_CUR_DIR)/lib/ftplistparser.c    \
  $(CURL_CUR_DIR)/lib/getenv.c           \
  $(CURL_CUR_DIR)/lib/getinfo.c          \
  $(CURL_CUR_DIR)/lib/gopher.c           \
  $(CURL_CUR_DIR)/lib/hash.c             \
  $(CURL_CUR_DIR)/lib/headers.c          \
  $(CURL_CUR_DIR)/lib/hmac.c             \
  $(CURL_CUR_DIR)/lib/hostasyn.c         \
  $(CURL_CUR_DIR)/lib/hostip.c           \
  $(CURL_CUR_DIR)/lib/hostip4.c          \
  $(CURL_CUR_DIR)/lib/hostip6.c          \
  $(CURL_CUR_DIR)/lib/hostsyn.c          \
  $(CURL_CUR_DIR)/lib/hsts.c             \
  $(CURL_CUR_DIR)/lib/http.c             \
  $(CURL_CUR_DIR)/lib/http1.c            \
  $(CURL_CUR_DIR)/lib/http2.c            \
  $(CURL_CUR_DIR)/lib/http_aws_sigv4.c   \
  $(CURL_CUR_DIR)/lib/http_chunks.c      \
  $(CURL_CUR_DIR)/lib/http_digest.c      \
  $(CURL_CUR_DIR)/lib/http_negotiate.c   \
  $(CURL_CUR_DIR)/lib/http_ntlm.c        \
  $(CURL_CUR_DIR)/lib/http_proxy.c       \
  $(CURL_CUR_DIR)/lib/idn.c              \
  $(CURL_CUR_DIR)/lib/if2ip.c            \
  $(CURL_CUR_DIR)/lib/imap.c             \
  $(CURL_CUR_DIR)/lib/inet_ntop.c        \
  $(CURL_CUR_DIR)/lib/inet_pton.c        \
  $(CURL_CUR_DIR)/lib/krb5.c             \
  $(CURL_CUR_DIR)/lib/ldap.c             \
  $(CURL_CUR_DIR)/lib/llist.c            \
  $(CURL_CUR_DIR)/lib/macos.c            \
  $(CURL_CUR_DIR)/lib/md4.c              \
  $(CURL_CUR_DIR)/lib/md5.c              \
  $(CURL_CUR_DIR)/lib/memdebug.c         \
  $(CURL_CUR_DIR)/lib/mime.c             \
  $(CURL_CUR_DIR)/lib/mprintf.c          \
  $(CURL_CUR_DIR)/lib/multi.c            \
  $(CURL_CUR_DIR)/lib/netrc.c            \
  $(CURL_CUR_DIR)/lib/nonblock.c         \
  $(CURL_CUR_DIR)/lib/noproxy.c          \
  $(CURL_CUR_DIR)/lib/openldap.c         \
  $(CURL_CUR_DIR)/lib/parsedate.c        \
  $(CURL_CUR_DIR)/lib/pingpong.c         \
  $(CURL_CUR_DIR)/lib/pop3.c             \
  $(CURL_CUR_DIR)/lib/progress.c         \
  $(CURL_CUR_DIR)/lib/psl.c              \
  $(CURL_CUR_DIR)/lib/rand.c             \
  $(CURL_CUR_DIR)/lib/rename.c           \
  $(CURL_CUR_DIR)/lib/request.c          \
  $(CURL_CUR_DIR)/lib/rtsp.c             \
  $(CURL_CUR_DIR)/lib/select.c           \
  $(CURL_CUR_DIR)/lib/sendf.c            \
  $(CURL_CUR_DIR)/lib/setopt.c           \
  $(CURL_CUR_DIR)/lib/sha256.c           \
  $(CURL_CUR_DIR)/lib/share.c            \
  $(CURL_CUR_DIR)/lib/slist.c            \
  $(CURL_CUR_DIR)/lib/smb.c              \
  $(CURL_CUR_DIR)/lib/smtp.c             \
  $(CURL_CUR_DIR)/lib/socketpair.c       \
  $(CURL_CUR_DIR)/lib/socks.c            \
  $(CURL_CUR_DIR)/lib/socks_gssapi.c     \
  $(CURL_CUR_DIR)/lib/socks_sspi.c       \
  $(CURL_CUR_DIR)/lib/speedcheck.c       \
  $(CURL_CUR_DIR)/lib/splay.c            \
  $(CURL_CUR_DIR)/lib/strcase.c          \
  $(CURL_CUR_DIR)/lib/strdup.c           \
  $(CURL_CUR_DIR)/lib/strerror.c         \
  $(CURL_CUR_DIR)/lib/strtok.c           \
  $(CURL_CUR_DIR)/lib/strtoofft.c        \
  $(CURL_CUR_DIR)/lib/system_win32.c     \
  $(CURL_CUR_DIR)/lib/telnet.c           \
  $(CURL_CUR_DIR)/lib/tftp.c             \
  $(CURL_CUR_DIR)/lib/timediff.c         \
  $(CURL_CUR_DIR)/lib/timeval.c          \
  $(CURL_CUR_DIR)/lib/transfer.c         \
  $(CURL_CUR_DIR)/lib/url.c              \
  $(CURL_CUR_DIR)/lib/urlapi.c           \
  $(CURL_CUR_DIR)/lib/version.c          \
  $(CURL_CUR_DIR)/lib/version_win32.c    \
  $(CURL_CUR_DIR)/lib/warnless.c         \
  $(CURL_CUR_DIR)/lib/ws.c               \
  $(CURL_CUR_DIR)/lib/vquic/curl_msh3.c   \
  $(CURL_CUR_DIR)/lib/vquic/curl_ngtcp2.c   \
  $(CURL_CUR_DIR)/lib/vquic/curl_osslq.c   \
  $(CURL_CUR_DIR)/lib/vquic/curl_quiche.c   \
  $(CURL_CUR_DIR)/lib/vquic/vquic.c \
  $(CURL_CUR_DIR)/lib/vquic/vquic-tls.c \
  $(CURL_CUR_DIR)/lib/vtls/bearssl.c            \
  $(CURL_CUR_DIR)/lib/vtls/cipher_suite.c       \
  $(CURL_CUR_DIR)/lib/vtls/gtls.c               \
  $(CURL_CUR_DIR)/lib/vtls/hostcheck.c          \
  $(CURL_CUR_DIR)/lib/vtls/keylog.c             \
  $(CURL_CUR_DIR)/lib/vtls/mbedtls.c            \
  $(CURL_CUR_DIR)/lib/vtls/mbedtls_threadlock.c \
  $(CURL_CUR_DIR)/lib/vtls/openssl.c            \
  $(CURL_CUR_DIR)/lib/vtls/rustls.c             \
  $(CURL_CUR_DIR)/lib/vtls/schannel.c           \
  $(CURL_CUR_DIR)/lib/vtls/schannel_verify.c    \
  $(CURL_CUR_DIR)/lib/vtls/sectransp.c          \
  $(CURL_CUR_DIR)/lib/vtls/vtls.c               \
  $(CURL_CUR_DIR)/lib/vtls/wolfssl.c            \
  $(CURL_CUR_DIR)/lib/vtls/x509asn1.c \
  $(CURL_CUR_DIR)/lib/vauth/cleartext.c     \
  $(CURL_CUR_DIR)/lib/vauth/cram.c          \
  $(CURL_CUR_DIR)/lib/vauth/digest.c        \
  $(CURL_CUR_DIR)/lib/vauth/digest_sspi.c   \
  $(CURL_CUR_DIR)/lib/vauth/gsasl.c         \
  $(CURL_CUR_DIR)/lib/vauth/krb5_gssapi.c   \
  $(CURL_CUR_DIR)/lib/vauth/krb5_sspi.c     \
  $(CURL_CUR_DIR)/lib/vauth/ntlm.c          \
  $(CURL_CUR_DIR)/lib/vauth/ntlm_sspi.c     \
  $(CURL_CUR_DIR)/lib/vauth/oauth2.c        \
  $(CURL_CUR_DIR)/lib/vauth/spnego_gssapi.c \
  $(CURL_CUR_DIR)/lib/vauth/spnego_sspi.c   \
  $(CURL_CUR_DIR)/lib/vauth/vauth.c \


CURL_CFLAGS = -I$(CURL_CUR_DIR)/lib -I$(CURL_CUR_DIR)/include -I$(CURL_CUR_DIR)/src -DHAVE_CONFIG_H -DCURLDEBUG -DBUILDING_LIBCURL

