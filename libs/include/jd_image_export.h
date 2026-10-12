#ifndef GARLIC_JD_IMAGE_EXPORT_H
#define GARLIC_JD_IMAGE_EXPORT_H

/*
 * ============================================================================
 *  归一化镜像的只读查询
 *
 *  分析结果里的容器信息 —— 段、节、符号、重定位/导入、依赖库 —— 都能从
 *  这里取。**ELF 和 Mach-O 走的是同一份**：loader 已经把两种容器归一化成
 *  同一个 jd_image，使用方不需要关心手里的是哪种。
 *
 *  本头是自包含的：只依赖 <stddef.h> / <stdint.h>，不引任何内部类型。
 *  使用方（Thyme 的 garlic_bridge）dlopen 本库之后照常按名字取函数即可。
 *
 *  典型用法：
 *      jd_elf *elf = jd_analysis_macho_from_path("/path/libfoo.dylib");
 *      if (elf) {
 *          unsigned long long base = jd_image_base(elf);   // Mach-O 才有意义
 *          size_t n = jd_image_segment_count(elf);
 *          for (size_t i = 0; i < n; i++) {
 *              jd_image_segment_info seg;
 *              if (!jd_image_get_segment(elf, i, &seg)) continue;
 *              ...
 *          }
 *      }
 *
 *  所有字符串都由库持有，在 elf 句柄存活期间一直有效 —— 不要 free，也不要
 *  在 jd_elf_release() 之后再使用。
 * ============================================================================
 */

#include <stddef.h>
#include <stdint.h>

/* ROSEMARY_API 在 jd_export.h —— 四个对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明句柄：使用方不需要知道内部结构 */
struct jd_elf;
struct jd_image;

/* ---- 容器 / 架构 -------------------------------------------------- */

#define JD_IMG_FMT_UNKNOWN 0
#define JD_IMG_FMT_ELF     1
#define JD_IMG_FMT_MACHO   2

/* 取值与库内部的 jd_isa 一一对应 */
#ifndef JD_IMG_ISA_UNKNOWN
#define JD_IMG_ISA_UNKNOWN 0
#define JD_IMG_ISA_T32     1
#define JD_IMG_ISA_A32     2
#define JD_IMG_ISA_A64     3
#endif

/* 段权限（已统一成 ELF 的 PF_ 位序） */
#define JD_IMG_PERM_X 0x1
#define JD_IMG_PERM_W 0x2
#define JD_IMG_PERM_R 0x4

/* 归一化后的 section 角色 —— 两种格式对齐用的，analyzer 也只认这个 */
#define JD_IMG_SECT_UNKNOWN    0
#define JD_IMG_SECT_TEXT       1   /* .text        / __TEXT,__text          */
#define JD_IMG_SECT_INIT_ARRAY 2   /* .init_array  / __DATA,__mod_init_func */
#define JD_IMG_SECT_FINI_ARRAY 3   /* .fini_array  / __DATA,__mod_term_func */
#define JD_IMG_SECT_STUBS      4   /* .plt         / __TEXT,__stubs         */
#define JD_IMG_SECT_GOT        5   /* .got         / __DATA_CONST,__got     */
#define JD_IMG_SECT_LAZY_PTR   6   /* .got.plt     / __DATA,__la_symbol_ptr */
#define JD_IMG_SECT_CSTRING    7   /* .rodata      / __TEXT,__cstring       */
#define JD_IMG_SECT_UNWIND     8   /* .eh_frame    / __TEXT,__unwind_info   */
#define JD_IMG_SECT_DATACONST  9   /* .data.rel.ro / __DATA_CONST           */

/* 符号类别 —— Mach-O 的 nlist 不带类型，由它落在哪个节推出来 */
#define JD_IMG_SYM_UNKNOWN 0
#define JD_IMG_SYM_NOTYPE  1
#define JD_IMG_SYM_OBJECT  2
#define JD_IMG_SYM_FUNC    3
#define JD_IMG_SYM_SECTION 4
#define JD_IMG_SYM_FILE    5

#define JD_IMG_BIND_LOCAL  0
#define JD_IMG_BIND_GLOBAL 1
#define JD_IMG_BIND_WEAK   2

/* 重定位槽位在分析里的角色 */
#define JD_IMG_RELOC_GOT    0x0001
#define JD_IMG_RELOC_REL    0x0002   /* rebase：指向本镜像内的地址 */
#define JD_IMG_RELOC_JMP    0x0004
#define JD_IMG_RELOC_IMPORT 0x4000   /* 指向别的库里的符号 */

/* ---- 结构 -------------------------------------------------------- */

typedef struct {
    unsigned long long vmaddr;    /* 归一化之后的地址，见 jd_image_base() */
    unsigned long long vmsize;
    unsigned long long fileoff;
    unsigned long long filesize;
    unsigned           perms;     /* JD_IMG_PERM_* */
    char               name[24];  /* NUL 终止；ELF 的段没有名字，为空串 */
} jd_image_segment_info;

typedef struct {
    char               name[24];
    char               seg_name[24];
    unsigned long long addr;
    unsigned long long size;
    unsigned           offset;    /* 文件偏移；0 表示 zerofill（.bss / __common） */
    unsigned           kind;      /* JD_IMG_SECT_* */
    unsigned           flags;     /* 容器原生的 section flags */
} jd_image_section_info;

typedef struct {
    unsigned long long addr;
    const char        *name;      /* 可空 */
    unsigned           kind;      /* JD_IMG_SYM_* */
    unsigned           bind;      /* JD_IMG_BIND_* */
    unsigned           section_idx;
    int                is_export;
    int                is_import;
} jd_image_symbol_info;

typedef struct {
    unsigned long long slot;      /* 被写指针的地址 */
    unsigned long long addend;    /* rebase 的目标地址 */
    const char        *name;      /* 导入符号名（已剥前导 '_'）；非导入为 NULL */
    const char        *lib;       /* 导入来源库名；可空 */
    unsigned           flags;     /* JD_IMG_RELOC_*；用 IMPORT 位筛导入 */
} jd_image_reloc_info;

typedef struct {
    unsigned long long addr;
    const char        *name;      /* 可空 */
    unsigned           type;      /* 容器给出的入口类别，见下 */
} jd_image_entry_info;

#define JD_IMG_ENTRY_UNKNOWN        0
#define JD_IMG_ENTRY_MAIN           1   /* ELF e_entry / Mach-O LC_MAIN */
#define JD_IMG_ENTRY_SYMBOL         2
#define JD_IMG_ENTRY_EXPORT         3
#define JD_IMG_ENTRY_INIT_ARRAY     4
#define JD_IMG_ENTRY_FUNCTION_START 5   /* Mach-O LC_FUNCTION_STARTS */
#define JD_IMG_ENTRY_FDE            6
#define JD_IMG_ENTRY_IFUNC          7

/* ---- 查询 -------------------------------------------------------- */

/*
 * 镜像基址。
 *
 * Mach-O 的地址是 64 位的（__TEXT 通常从 0x100000000 起），而分析内部一律用
 * 32 位存 pc —— 所以 loader 把镜像按 __TEXT 起始地址归一到 0 基址。
 *
 * **下面所有地址都是归一化之后的**。要还原成 Mach-O 的真实地址，把 base
 * 加回去。ELF 恒为 0。
 */
ROSEMARY_API unsigned long long jd_image_base(struct jd_elf *elf);

ROSEMARY_API unsigned jd_image_format(struct jd_elf *elf);   /* JD_IMG_FMT_* */
ROSEMARY_API unsigned jd_image_isa(struct jd_elf *elf);      /* JD_IMG_ISA_* */

/* 段 */
ROSEMARY_API size_t jd_image_segment_count(struct jd_elf *elf);
ROSEMARY_API int    jd_image_get_segment(struct jd_elf *elf, size_t index,
                                        jd_image_segment_info *out);

/* 节 */
ROSEMARY_API size_t jd_image_section_count(struct jd_elf *elf);
ROSEMARY_API int    jd_image_get_section(struct jd_elf *elf, size_t index,
                                        jd_image_section_info *out);

/* 按 (段名, 节名) 查节；某个名字传 NULL 表示不限定。命中返回 1 并填 out */
ROSEMARY_API int jd_image_find_section(struct jd_elf *elf,
                                       const char *seg_name, const char *sect_name,
                                       jd_image_section_info *out);

/* 符号 */
ROSEMARY_API size_t jd_image_symbol_count(struct jd_elf *elf);
ROSEMARY_API int    jd_image_get_symbol(struct jd_elf *elf, size_t index,
                                       jd_image_symbol_info *out);

/* 重定位 / 导入（同一张表；用 flags 的 JD_IMG_RELOC_IMPORT 位筛导入） */
ROSEMARY_API size_t jd_image_reloc_count(struct jd_elf *elf);
ROSEMARY_API int    jd_image_get_reloc(struct jd_elf *elf, size_t index,
                                      jd_image_reloc_info *out);

/* 容器给出的入口点（不含 PLT 扫描、序言扫描那些启发式结果） */
ROSEMARY_API size_t jd_image_entry_count(struct jd_elf *elf);
ROSEMARY_API int    jd_image_get_entry(struct jd_elf *elf, size_t index,
                                      jd_image_entry_info *out);

/* 依赖库。index 0 是主二进制自身，1 起对应 Mach-O 的 lib_ordinal */
ROSEMARY_API size_t      jd_image_dylib_count(struct jd_elf *elf);
ROSEMARY_API const char* jd_image_get_dylib(struct jd_elf *elf, size_t index);

#ifdef __cplusplus
}
#endif

#endif // GARLIC_JD_IMAGE_EXPORT_H
