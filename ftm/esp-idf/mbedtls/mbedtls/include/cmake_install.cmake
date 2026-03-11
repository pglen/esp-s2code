# Install script for directory: /akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/peterglen/.espressif/tools/xtensa-esp32-elf/esp-2020r3-8.4.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbedtls" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aes.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aesni.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/arc4.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/aria.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/asn1write.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/base64.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/bignum.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/blowfish.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/bn_mul.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/camellia.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ccm.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/certs.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chacha20.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/chachapoly.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/check_config.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cipher.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cipher_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/cmac.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/compat-1.3.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/config.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ctr_drbg.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/debug.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/des.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/dhm.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdh.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecdsa.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecjpake.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecp.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ecp_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/entropy.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/entropy_poll.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/error.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/gcm.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/havege.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hkdf.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/hmac_drbg.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md2.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md4.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md5.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/md_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/memory_buffer_alloc.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/net.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/net_sockets.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/nist_kw.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/oid.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/padlock.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pem.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pk.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pk_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs11.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs12.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/pkcs5.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_time.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/platform_util.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/poly1305.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ripemd160.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/rsa.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/rsa_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha1.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha256.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/sha512.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cache.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ciphersuites.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_cookie.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_internal.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/ssl_ticket.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/threading.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/timing.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/version.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crl.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_crt.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/x509_csr.h"
    "/akostar/esp32-s2/esp-idf/components/mbedtls/mbedtls/include/mbedtls/xtea.h"
    )
endif()

