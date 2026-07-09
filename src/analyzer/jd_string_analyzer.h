#ifndef GARLIC_JD_STRING_ANALYZER_H
#define GARLIC_JD_STRING_ANALYZER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jd_analyzer.h"
#include "parser/dex/metadata.h"
#include "dalvik/dex_ins_helper.h"
#include "dalvik/dex_meta_helper.h"
#include "libs/hashmap/hashmap_tools.h"
#include "common/str_tools.h"

/**
 * do not use regex its too slow!!!!!
 **/

static bool str_is_url(const char *s)
{
    return
            strncmp(s, "http://", 7) == 0 ||
            strncmp(s, "https://", 8) == 0 ||
            strncmp(s, "ftp://", 6) == 0 ||
            strncmp(s, "ftps://", 7) == 0 ||
            strncmp(s, "rtsp://", 7) == 0 ||
            strncmp(s, "mms://", 6) == 0 ||
            strncmp(s, "ws://", 5) == 0 ||
            strncmp(s, "wss://", 6) == 0 ||
            strncmp(s, "content://", 10) == 0 ||
            strncmp(s, "file://", 7) == 0 ||
            strncmp(s, "socket://", 9) == 0;
}

static bool str_is_enc_dec(const char *s)
{
    static const char *names[] = {
            "AES", "DES", "DESede", "3DES",
            "Blowfish", "Twofish",
            "RC2", "RC4", "RC5", "RC6",
            "ChaCha20", "SM4",

            "RSA", "DSA",
            "EC", "ECDSA", "ECDH",
            "Ed25519", "X25519",
            "SM2",

            "MD2", "MD4", "MD5",
            "SHA1", "SHA-1",
            "SHA224", "SHA-224",
            "SHA256", "SHA-256",
            "SHA384", "SHA-384",
            "SHA512", "SHA-512",
            "SHA3-224", "SHA3-256",
            "SHA3-384", "SHA3-512",
            "SM3",

            "HmacMD5",
            "HmacSHA1",
            "HmacSHA224",
            "HmacSHA256",
            "HmacSHA384",
            "HmacSHA512",
            "HmacSM3",

            NULL
    };

    for (int i = 0; names[i]; i++) {
        if (strcmp(s, names[i]) == 0)
            return true;
    }

    if (strncmp(s, "AES/", 4) == 0) return true;
    if (strncmp(s, "DES/", 4) == 0) return true;
    if (strncmp(s, "DESede/", 7) == 0) return true;
    if (strncmp(s, "RSA/", 4) == 0) return true;
    if (strncmp(s, "EC/", 3) == 0) return true;
    if (strncmp(s, "SM2/", 4) == 0) return true;
    if (strncmp(s, "SM4/", 4) == 0) return true;
    if (strncmp(s, "ChaCha20/", 9) == 0) return true;

    return false;
}

static inline int str_is_uuid(const char *s)
{
    if (strlen(s) != 36)
        return false;

    if (s[8]  != '-') return false;
    if (s[13] != '-') return false;
    if (s[18] != '-') return false;
    if (s[23] != '-') return false;

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23)
            continue;

        unsigned char c = (unsigned char)s[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return false;
    }

    return true;
}

static inline bool str_is_pem_key(const char *s)
{
    if (s == NULL)
        return false;

    /* Standard PEM */
    if (strstr(s, "-----BEGIN PUBLIC KEY-----")) return true;
    if (strstr(s, "-----BEGIN PRIVATE KEY-----")) return true;
    if (strstr(s, "-----BEGIN RSA PUBLIC KEY-----")) return true;
    if (strstr(s, "-----BEGIN RSA PRIVATE KEY-----")) return true;
    if (strstr(s, "-----BEGIN EC PRIVATE KEY-----")) return true;
    if (strstr(s, "-----BEGIN ENCRYPTED PRIVATE KEY-----")) return true;
    if (strstr(s, "-----BEGIN CERTIFICATE-----")) return true;
    if (strstr(s, "-----BEGIN CERTIFICATE REQUEST-----")) return true;
    if (strstr(s, "-----BEGIN OPENSSH PRIVATE KEY-----")) return true;

    if (strstr(s, "BEGIN PUBLIC KEY")) return true;
    if (strstr(s, "BEGIN PRIVATE KEY")) return true;
    if (strstr(s, "BEGIN RSA PUBLIC KEY")) return true;
    if (strstr(s, "BEGIN RSA PRIVATE KEY")) return true;
    if (strstr(s, "BEGIN EC PRIVATE KEY")) return true;
    if (strstr(s, "BEGIN ENCRYPTED PRIVATE KEY")) return true;
    if (strstr(s, "BEGIN CERTIFICATE")) return true;
    if (strstr(s, "BEGIN CERTIFICATE REQUEST")) return true;
    if (strstr(s, "BEGIN OPENSSH PRIVATE KEY")) return true;

    return false;
}

static inline int str_is_der_base64_key(const char *s)
{
    size_t len = strlen(s);

    if (len < 128)
        return false;

    if (strncmp(s, "MIIB", 4) == 0) return true;
    if (strncmp(s, "MIIC", 4) == 0) return true;
    if (strncmp(s, "MIID", 4) == 0) return true;
    if (strncmp(s, "MIIE", 4) == 0) return true;
    if (strncmp(s, "MIIF", 4) == 0) return true;

    return false;
}

static inline bool str_is_so_name(const char *s)
{
    if (s == NULL)
        return false;

    const char *name = strrchr(s, '/');
    if (name)
        name++;
    else
        name = s;

    size_t len = strlen(name);

    if (len < 4)
        return false;

    if (strcmp(name + len - 3, ".so") != 0)
        return false;

    for (size_t i = 0; i < len - 3; i++) {
        char c = name[i];

        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' ||
            c == '-' ||
            c == '+' ||
            c == '.') {
            continue;
        }

        return false;
    }

    return true;
}

static inline bool str_is_ipv4(const char *s)
{
    if (s == NULL || *s == '\0')
        return false;

    int dots = 0;
    int value = 0;
    int digits = 0;

    while (*s) {
        if (*s >= '0' && *s <= '9') {
            value = value * 10 + (*s - '0');
            if (value > 255)
                return false;
            digits++;
            if (digits > 3)
                return false;
        } else if (*s == '.') {
            if (digits == 0)
                return false;
            dots++;
            if (dots > 3)
                return false;
            value = 0;
            digits = 0;
        } else {
            return false;
        }
        s++;
    }

    return dots == 3 && digits > 0;
}

#endif //GARLIC_JD_STRING_ANALYZER_H
