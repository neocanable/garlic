#ifndef GARLIC_JD_FUNCTION_EXPORT_H
#define GARLIC_JD_FUNCTION_EXPORT_H

/*
 * ============================================================================
 *  指定一个 PC 当函数入口，现场把它切出来
 *
 *  分析本来是整库跑一遍、入口由符号/unwind 表这些自动发现。这里让使用方**点名
 *  一个地址**（比如用户在一个没被识别成函数的地方点了"设为函数"），把它当函数
 *  入口现场切出基本块，并把它里面的跳转表（含 case 和 default 目标）一并解出来。
 *
 *  ELF 和 Mach-O 共用这一条路。
 *
 *  典型用法：
 *      jd_function_info fi;
 *      int rc = jd_analysis_function_at(elf, pc, &fi);
 *      if (rc == JD_FUNC_OK) {
 *          for (unsigned i = 0; i < fi.bb_count; i++) {
 *              jd_function_bb_info bb;
 *              if (!jd_function_bb_at(elf, pc, i, &bb)) continue;
 *              ...
 *          }
 *          for (unsigned i = 0; i < fi.jt_count; i++) {
 *              jd_function_jt_info jt;
 *              if (!jd_function_jt_at(elf, pc, i, &jt)) continue;
 *              for (unsigned c = 0; c < jt.target_count; c++) {
 *                  unsigned long long target; int decoded;
 *                  jd_function_jt_case_at(elf, pc, i, c, &target, &decoded);
 *              }
 *          }
 *      }
 *
 *  **这是一次分析动作，会在传进来的 elf 句柄上就地改东西**（新解码的指令会进
 *  指令表、新函数会进函数表），而且需要 elf 已经被 jd_analysis_elf_from_path /
 *  jd_analysis_macho_from_path 分析过 —— 没分析过的句柄一律返回
 *  JD_FUNC_ERR_NOT_READY。
 *
 *  本头是自包含的，只依赖 <stddef.h> / <stdint.h> 和同目录的公开头。
 * ============================================================================
 */

#include <stddef.h>
#include <stdint.h>

/* ROSEMARY_API 在 jd_export.h —— 对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"
/* ISA 的取值用 JD_IMG_ISA_*（和 jd_image_isa() 同一套） */
#include "jd_image_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 不透明句柄：使用方不需要知道内部结构 */
struct jd_elf;

/* ---- 返回值 -------------------------------------------------------- */

/*
 * 0 是成功，负数是失败原因。Thyme 那边可以直接按码给提示，不用去猜。
 *
 * 注意 ERR_NOT_BOUNDARY 和 ERR_DATA_TABLE 这两种不是"这个地址不是函数"，
 * 而是"这个地址根本不该当代码"—— 前者说明它落在**一条已经解出来的指令中间**，
 * 硬解下去会把包含它的那条函数的 bb 切坏，所以这里直接拒绝。
 */
#define JD_FUNC_OK                 0
#define JD_FUNC_ERR_ARG           -1   /* elf / pc / out 有 NULL */
#define JD_FUNC_ERR_NO_LICENSE    -2   /* license 不通过 */
#define JD_FUNC_ERR_NO_REGION     -3   /* pc 不在任何可执行段里 */
#define JD_FUNC_ERR_NOT_READY     -4   /* 这个句柄还没做过整库分析 */
#define JD_FUNC_ERR_ALIGN         -5   /* A64 上 pc 不是 4 字节对齐 */
#define JD_FUNC_ERR_BAD_ADDR      -6   /* 这个地址被标成了坏地址 */
#define JD_FUNC_ERR_DATA_TABLE    -7   /* 落在符号表/字符串表/hash/重定位表区间里 */
#define JD_FUNC_ERR_NOT_BOUNDARY  -8   /* 落在一条已解出的指令中间，不是指令边界 */
#define JD_FUNC_ERR_NO_DECODE     -9   /* 这个地址在这个 ISA 下解不出指令 */

/* ---- 结构 ---------------------------------------------------------- */

/* 一个基本块。地址都是归一化的，和 jd_image_export.h 同一套 */
typedef struct {
    unsigned long long start_pc;    /* 首指令地址 */
    unsigned long long end_pc;      /* 末指令地址 */
    unsigned           block_id;    /* 库内编号，只在同一次分析里有意义 */
    unsigned           out_count;   /* 出边数：fallthrough / 跳转 / 跳转表 case */
} jd_function_bb_info;

/*
 * 这张函数里的一个跳转表。
 *
 * `valid` 为 0 表示"认出了这里有个表、但没能把目标解出来"—— 这时 target_count
 * 和 default_target 都不可信，只有 pc / type 有意义（和 .jumptable 测试一个口径）。
 */
typedef struct {
    unsigned long long pc;              /* 用它的那条指令 */
    unsigned long long table_addr;      /* 表在镜像里的地址 */
    unsigned long long base;            /* REL32 / COMPACT 的基准地址 */
    unsigned long long default_target;  /* default 分支（valid 为 0 时无意义） */
    unsigned           type;            /* 0=none 1=abs 2=rel32 3=compact 4=tbb */
    unsigned           entry_size;      /* 每个表项的字节数 */
    unsigned           target_count;    /* case 个数（valid 为 0 时无意义） */
    unsigned           valid;
} jd_function_jt_info;

/* 函数概要 */
typedef struct {
    unsigned long long pc;              /* 归一化后的**代码地址**（已抹掉 Thumb 位）*/
    const char        *entry_name;      /* 入口来路的名字，如 "symbol"/"pc_forced"；库持有 */
    unsigned           isa;             /* JD_IMG_ISA_* */
    unsigned           bb_count;
    unsigned           ins_count;       /* 属于这条函数的指令条数 */
    unsigned           ins_added;       /* **本次调用**新物化了多少条指令 */

    /*
     * 本次调用解出了新指令（ins_added > 0）时要注意：bb 的切分可能在新老交界
     * 处不准 —— 见 docs/bugs/2026-09-21-bb-end-normalize-assumes-sorted-ins.md。
     * 调用方可以把 ins_added 当"这次结果要打个折扣看"的信号。
     */
    unsigned           jt_count;
    unsigned           jt_target_unwalked;  /* 本该解出来却没解出来的 case/default 数 */
    unsigned           rounds;          /* 迭代轮数（内部用，调试） */
} jd_function_info;

/* ---- 查询 ---------------------------------------------------------- */

/*
 * 把 pc 当函数入口现场切出来。pc 可以带 Thumb 位（32 位下最低位），库内部会抹。
 *
 * 语义：pc 落在别的函数中间时也**强制在这里开一条新函数**（允许重叠）；本来就是
 * 函数头的保留它原来的入口来路，不覆盖成"用户指定"。
 *
 * 成功返回 JD_FUNC_OK 并填 out；失败返回负的错误码，out 不动。
 * 成功的那些调用**允许重复**（同一个 pc 反复调是幂等的）。
 */
ROSEMARY_API int jd_analysis_function_at(struct jd_elf *elf, unsigned long long pc,
                                         jd_function_info *out);

/* 第 index 个基本块。越界返回 0 */
ROSEMARY_API int jd_function_bb_at(struct jd_elf *elf, unsigned long long func_pc,
                                   unsigned index, jd_function_bb_info *out);

/* 第 index 张跳转表。越界返回 0 */
ROSEMARY_API int jd_function_jt_at(struct jd_elf *elf, unsigned long long func_pc,
                                   unsigned index, jd_function_jt_info *out);

/*
 * 第 index 张表的第 case 个目标。
 *
 * out_target 收目标地址；out_decoded 收"这个目标是不是真解成了代码"
 * （1 = ins_bitmap 里有它，0 = 没有 —— 后者说明这次分析没能覆盖到那儿）。
 * 两个出参都可空。越界或该表 valid 为 0 返回 0。
 */
ROSEMARY_API int jd_function_jt_case_at(struct jd_elf *elf, unsigned long long func_pc,
                                        unsigned jt_index, unsigned case_index,
                                        unsigned long long *out_target,
                                        int *out_decoded);

/*
 * 第 index 张表的 default 目标。语义同 jd_function_jt_case_at。
 * 该表没有 default（或 valid 为 0）返回 0。
 */
ROSEMARY_API int jd_function_jt_default_at(struct jd_elf *elf, unsigned long long func_pc,
                                           unsigned jt_index,
                                           unsigned long long *out_target,
                                           int *out_decoded);

#ifdef __cplusplus
}
#endif

#endif // GARLIC_JD_FUNCTION_EXPORT_H
