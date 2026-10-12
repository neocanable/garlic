#ifndef GARLIC_JD_PLIST_EXPORT_H
#define GARLIC_JD_PLIST_EXPORT_H

/*
 * ============================================================================
 *  property list（plist）与 Info.plist 的对外解析
 *
 *  给 Thyme 这类使用方用：一个 IPA 里 `Payload/<App>.app/Info.plist` 是应用
 *  清单，地位等同 Android 的 AndroidManifest.xml —— 包名、版本、最低系统版本、
 *  主二进制名字都在里面。
 *
 *  两种编码都认，不用调用方先判断：
 *    - 二进制（`bplist00`）—— 真实 IPA 里几乎全是这个；
 *    - XML（`<?xml ...>` / `<plist ...>`）—— 开发期、未打包的 .app 里常见。
 *
 *  **本库不碰 zip**（rosemarylib 不含 zip 实现）。IPA 里的条目要由调用方自己
 *  解出来，把字节喂给 jd_plist_parse_buffer()。文件在磁盘上也可以直接用
 *  jd_plist_parse_path()。
 *
 *  典型用法：
 *      jd_plist_handle *p = jd_plist_parse_path("/x/Info.plist");
 *      if (p) {
 *          jd_info_plist_info info;
 *          if (jd_plist_info(p, &info))
 *              printf("%s %s\n", info.bundle_id, info.short_version);
 *          jd_plist_release(p);
 *      }
 *
 *  本头是自包含的：只依赖 <stddef.h>，不引任何内部类型。
 *  所有返回的字符串都由库持有，句柄释放后失效 —— 不要 free。
 * ============================================================================
 */

#include <stddef.h>

/* ROSEMARY_API 在 jd_export.h —— 四个对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明句柄。一个句柄 = 一次解析 = 一个 pool，用完 jd_plist_release() */
typedef struct jd_plist_handle jd_plist_handle;

/* ==================================================================== */
/* 解析与释放                                                           */
/* ==================================================================== */

/*
 * 从内存解析。buffer 只需要在本次调用期间有效 —— 树里的字符串都拷进库自己的
 * pool 了，调用方随后就能把 buffer 还回去。
 *
 * 失败返回 NULL（具体原因打到 stderr），不会返回半个树。
 */
ROSEMARY_API jd_plist_handle* jd_plist_parse_buffer(const unsigned char *buffer,
                                                    size_t size);

/* 从文件读进来再解析。plist 通常只有 KB 级，整个读进来即可。 */
ROSEMARY_API jd_plist_handle* jd_plist_parse_path(const char *path);

/* 释放句柄。传 NULL 是安全的。之后不要再读从这个句柄拿到的任何字符串 */
ROSEMARY_API void jd_plist_release(jd_plist_handle *h);

/* ==================================================================== */
/* Info.plist 的常用字段                                                */
/* ==================================================================== */

/*
 * 一份快照。取不到的字段一律是 NULL / -1 / 0，**不会**因为缺某个键就让整次
 * 解析失败 —— app / framework / appex / 命令行工具的 Info.plist 差得很远。
 *
 * 字符串都由库持有，跟句柄同寿命。
 */
typedef struct {
    const char *bundle_id;            /* CFBundleIdentifier        —— 包名 */
    const char *bundle_name;          /* CFBundleName */
    const char *display_name;         /* CFBundleDisplayName */
    const char *executable;           /* CFBundleExecutable        —— 主二进制名 */
    const char *bundle_version;       /* CFBundleVersion           —— 构建号 */
    const char *short_version;        /* CFBundleShortVersionString—— 版本号 */
    const char *min_os_version;       /* MinimumOSVersion          —— 原样字符串 */
    const char *package_type;         /* CFBundlePackageType：APPL / FMWK / BNDL … */
    const char *platform_name;        /* DTPlatformName：iphoneos / macosx … */
    const char *sdk_name;             /* DTSDKName */
    const char *development_region;   /* CFBundleDevelopmentRegion */

    long long   min_os_major;         /* MinimumOSVersion 的主版本号（"12.0" -> 12）；-1 = 取不到 */
    int         supports_iphone;      /* UIDeviceFamily 里有 1 */
    int         supports_ipad;        /* UIDeviceFamily 里有 2 */
    int         requires_iphone_os;   /* LSRequiresIPhoneOS */
} jd_info_plist_info;

/*
 * 取 Info.plist 的常用字段。fill 之前 *out 会被清零。
 *
 * 根不是 dict（多半是拿错文件了）返回 0，out 保持全零 —— 注意这和「字段缺失」
 * 不一样：字段缺失仍然返回 1，只是那几项是 NULL。
 */
ROSEMARY_API int jd_plist_info(jd_plist_handle *h, jd_info_plist_info *out);

/* ==================================================================== */
/* 按路径查任意键                                                       */
/* ==================================================================== */

/*
 * 按 'a.b.c' 逐层下钻。中间任何一层不是 dict、或者键不存在，都按「取不到」处理。
 *
 * 数组用下标表示：`CFBundleURLTypes.0.CFBundleURLSchemes.0` 这样。
 * 想翻没在上面列出的键（NSExtension、UIRequiredDeviceCapabilities …）就用这几个。
 */

/* 值的类型，见下面 JD_PLIST_T_*。取不到返回 JD_PLIST_T_MISSING */
#define JD_PLIST_T_MISSING 0    /* 路径不存在，或中间类型对不上 */
#define JD_PLIST_T_DICT    1
#define JD_PLIST_T_ARRAY   2
#define JD_PLIST_T_STRING  3
#define JD_PLIST_T_INT     4
#define JD_PLIST_T_REAL    5
#define JD_PLIST_T_BOOL    6
#define JD_PLIST_T_DATE    7
#define JD_PLIST_T_DATA    8

ROSEMARY_API int jd_plist_type_at(jd_plist_handle *h, const char *path);

/* 非 STRING 返回 NULL。DATA 也返回 NULL（那是字节，不是文本） */
ROSEMARY_API const char* jd_plist_string_at(jd_plist_handle *h, const char *path);

/* 整数。BOOL 按 0/1 认；类型对不上或取不到返回 def */
ROSEMARY_API long long jd_plist_int_at(jd_plist_handle *h, const char *path,
                                       long long def);

/* 布尔。整数按非 0 认；类型对不上或取不到返回 def */
ROSEMARY_API int jd_plist_bool_at(jd_plist_handle *h, const char *path, int def);

/* 数组元素个数；不是 ARRAY 或取不到返回 0 */
ROSEMARY_API size_t jd_plist_count_at(jd_plist_handle *h, const char *path);

/*
 * 把整棵树按 plutil -p 的味道打到 stream（FILE* 由调用方开好）。
 * max_depth < 0 表示不限深度。纯调试用。
 */
ROSEMARY_API void jd_plist_dump_to(jd_plist_handle *h, void *stream, int max_depth);

#ifdef __cplusplus
}
#endif

#endif // GARLIC_JD_PLIST_EXPORT_H
