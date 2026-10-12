#ifndef GARLIC_JD_ASM_EXPORT_H
#define GARLIC_JD_ASM_EXPORT_H

/*
 * ============================================================================
 *  rosemary 二进制反汇编对外 API
 *
 *  这是给使用方（IDE / imgui / 其它语言绑定）用的唯一入口。
 *  头文件是自包含的：只依赖 <stddef.h> / <stdint.h>，不需要 rosemary 的任何
 *  内部头文件，可以直接被 C 和 C++ 引用。
 *
 *  设计要点：
 *    - 输入是 (pc, code, tpl, isa) 四元组，也就是 dump 出来的那三五个字段；
 *    - 输出是一串定长的 8 字节 token，渲染由使用方自己做；
 *    - **全流程零分配**：所有缓冲由调用方提供，库不调用 malloc / free，
 *      也不依赖 rosemary 的 memory pool，可以安全跨 DLL 边界。
 *
 *  典型用法（imgui 侧）：
 *
 *      jd_arm_token toks[64];
 *      char         strs[64];
 *      jd_asm_buf   buf;
 *      jd_asm_buf_init(&buf, toks, 64, strs, sizeof(strs));
 *
 *      // 从 dump 里读出来的每一条指令
 *      for (每条指令) {
 *          jd_asm_ins ins = { pc, code, tpl_index, flags, isa };
 *          jd_asm_buf_reset(&buf);
 *          if (jd_arm_disasm(&ins, &buf))
 *              render_tokens_to_ui(buf.tokens, buf.count);
 *      }
 * ============================================================================
 */

#include <stddef.h>
#include <stdint.h>

/* ROSEMARY_API 在 jd_export.h —— 四个对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 *  ISA
 * ------------------------------------------------------------------------- */
typedef enum {
    JD_ISA_UNKNOWN = 0,
    JD_ISA_T32     = 1,     /* Thumb-2 */
    JD_ISA_A32     = 2,     /* ARM (AArch32) */
    JD_ISA_A64     = 3,     /* AArch64 */
} jd_isa_t;

/*
 * 指令描述：dump 出来的就是这个。
 *
 * jd_dump_asm_binary 写出来的文件是这些记录的**裸数组**，
 * 没有文件头、也没有控制流信息：条数 = 文件大小 / sizeof(jd_asm_ins)。
 * 顺序按 pc 升序（每个 region 内部按 pc，region 之间按 region 顺序）。
 */
typedef struct {
    uint64_t pc;            /* 指令地址 */
    uint32_t code;          /* 原始 32 位指令字 */
    uint32_t isa;           /* jd_isa_t */
    uint16_t tpl_index;     /* 模板下标，喂回 jd_arm_disasm 用 */
    uint16_t flags;         /* JD_ASM_INS_F_* */
} jd_asm_ins;

#define JD_ASM_INS_F_16BIT  0x01    /* T32 16 位指令 */
#define JD_ASM_INS_F_INVALID 0x02   /* dump 时就没解析出叶子模板，tpl_index 不可用 */

/* ---------------------------------------------------------------------------
 *  函数表（.entries）
 *
 * .cfg_nodes / .cfg_edges 是整个库的平面文件：libapp.so 有 82 万个 basic
 * block、108 万条边，全读进内存要 31 MB，而使用方（IDE 的 CFG 视图）一次只
 * 显示一个函数 —— 五十来个 block。
 *
 * .entries 一个函数一行，除了原有的入口地址和函数名，还记着这个函数的
 * block/edge 在另外两个文件里的**字节区间**。使用方按区间 seek 过去只解析
 * 这一段，内存从几十 MB 降到几 KB。
 *
 * 格式（制表符分隔，没有文件头）：
 *     pc \t name \t start_pc \t end_pc \t node_off \t node_end \t edge_off \t edge_end
 *
 * 使用方（Thyme）把它连同其它测试一起导入 DuckDB 的 native_funcs 表，
 * 查函数时直接查库。
 *
 * 查函数时的两个坑（用 SQL 查也一样要防）：
 *   - start_pc/end_pc 是函数所有 block 的最小/最大值，函数要是 block 不连续
 *     中间就有洞，区间盖住 pc 不等于真有 block 盖住 pc；
 *   - 区间会重叠（basic block 被多个函数共享），一个 pc 可能被上千条记录
 *     盖住（libapp.so 实测最多 12437 条）。
 * 所以要按 node_off 升序逐条读出来验 —— node_off 的先后就是 block id 的先后，
 * 取第一条真有 block 盖住 pc 的，结果和全量解析逐字节一致。
 *
 * 因为要记的是字节偏移，这个文件只能在写 .cfg_nodes/.cfg_edges 的同一趟
 * 遍历里 ftell 出来，没法单独再遍历一遍函数。
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 *  token
 * ------------------------------------------------------------------------- */

/* token 类型。使用方按这个 switch 渲染即可。 */
typedef enum {
    JD_ASM_TOK_END = 0,     /* 流结束 */

    JD_ASM_TOK_INS,         /* 指令头：aux = isa<<1|16bit，value = code */
    JD_ASM_TOK_ADDR,        /* 地址：value = pc 低 32 位（WIDE 时后跟 HI） */
    JD_ASM_TOK_MNEMONIC,    /* 助记符：aux = 助记符下标，value = 模板下标 */
    JD_ASM_TOK_ALIAS,       /* 别名的真名：aux = 助记符下标 */
    JD_ASM_TOK_SUFFIX,      /* 后缀（.8b/.4s…）：aux = 尺寸枚举 */

    JD_ASM_TOK_REG,         /* 寄存器：aux = 寄存器类型，value = 寄存器号 */
    JD_ASM_TOK_IMM,         /* 立即数：value = 值（见 flags） */
    JD_ASM_TOK_MEM,         /* 内存操作数：aux = 寻址模式 */
    JD_ASM_TOK_SHIFT,       /* 移位/扩展：aux = 类型，value = 位移量 */
    JD_ASM_TOK_OFFSET,      /* 偏移：aux = 偏移类型 */
    JD_ASM_TOK_COND,        /* 条件码：aux = 条件值 */

    JD_ASM_TOK_VLIST,       /* 寄存器列表：aux = 个数 */
    JD_ASM_TOK_VLIST_RANGE, /* 寄存器范围列表：aux = 个数 */
    JD_ASM_TOK_SLICE,       /* 切片下标：aux = 下标个数 */
    JD_ASM_TOK_RLIST,       /* 位图寄存器列表：aux = 个数 */

    JD_ASM_TOK_MAP,         /* 映射表查值：aux = 映射类型，value = 查表值 */
    JD_ASM_TOK_MUL,         /* 乘数选项：aux = 类型 */
    JD_ASM_TOK_VALUE,       /* 裸数值：value = 值 */

    JD_ASM_TOK_HV,          /* 向量 h/v 后缀：aux = 枚举 */
    JD_ASM_TOK_MODE,        /* 谓词模式：aux = 枚举 */
    JD_ASM_TOK_ALIGN,       /* 对齐：value = 对齐值 */
    JD_ASM_TOK_ELT_IDX,     /* 元素下标包装，渲染成 [..] */
    JD_ASM_TOK_NAMED,       /* 具名操作数：aux = 名字长度，value = 字符串池偏移 */
    JD_ASM_TOK_IT,          /* IT 块：aux = 条件，value = 掩码 */
} jd_asm_token_type;

/*
 * 通用 flags 位（40/80 位，与 0x01~0x10 的类型专用位不冲突）：
 *   0x20  HI   —— 本条承载 value 的高 32 位
 *   0x40  WIDE —— value 超 32 位，后跟一条 HI token
 */
#define JD_ASM_F_HI         0x20
#define JD_ASM_F_WIDE       0x40

/* JD_ASM_TOK_REG */
#define JD_ASM_REG_F_TILES    0x01  /* 渲染成 { .. } */
#define JD_ASM_REG_F_WBACK    0x02  /* 渲染成 "!" */
#define JD_ASM_REG_F_HV       0x04  /* 后跟 JD_ASM_TOK_HV */
#define JD_ASM_REG_F_ALIAS    0x08  /* aux 是 jd_asm_reg_alias */
#define JD_ASM_REG_F_NO_INDEX 0x10  /* 无寄存器号 */
#define JD_ASM_REG_F_ELT      0x20  /* 后跟元素下标 */

typedef enum {
    JD_ASM_REG_ALIAS_SP = 1,
    JD_ASM_REG_ALIAS_WSP,
    JD_ASM_REG_ALIAS_XZR,
    JD_ASM_REG_ALIAS_WZR,
    JD_ASM_REG_ALIAS_LR,
    JD_ASM_REG_ALIAS_PC,
} jd_asm_reg_alias;

/* JD_ASM_TOK_IMM */
#define JD_ASM_IMM_F_SIGNED  0x01
#define JD_ASM_IMM_F_HEX     0x02
#define JD_ASM_IMM_F_FLOAT   0x04  /* value 是 float 位模式 */

/* JD_ASM_TOK_SHIFT */
#define JD_ASM_SHIFT_F_NAME_ONLY  0x01  /* 只渲染名字 */
#define JD_ASM_SHIFT_F_AMOUNT_REG 0x02  /* 位移量是寄存器，后跟该寄存器 */
#define JD_ASM_SHIFT_F_SUPPRESS   0x04  /* 移位量为 0，整段省略 */

/* JD_ASM_TOK_MEM */
#define JD_ASM_MEM_F_HAS_OFFSET 0x01    /* 后跟 offset 子 token */
#define JD_ASM_MEM_F_HAS_ALIGN  0x02    /* 后跟 JD_ASM_TOK_ALIGN */
#define JD_ASM_MEM_F_BANG       0x04    /* POST_IDX 无 offset 时补 "!" */

/* JD_ASM_TOK_OFFSET */
#define JD_ASM_OFFSET_F_NEG   0x01      /* 前面加 "-" */
#define JD_ASM_OFFSET_F_REG   0x02      /* 偏移来自寄存器 */
#define JD_ASM_OFFSET_F_MUL   0x04      /* 后跟乘数子 token */

/* JD_ASM_TOK_VALUE */
#define JD_ASM_VALUE_F_HEX    0x01
#define JD_ASM_VALUE_F_HASH   0x02

/* JD_ASM_TOK_ELT_IDX */
#define JD_ASM_ELT_F_NO_BRACKET 0x01    /* 不补 "[" "]" */

/* JD_ASM_TOK_VLIST / VLIST_RANGE */
#define JD_ASM_VLIST_F_ALL_LANES 0x01
#define JD_ASM_VLIST_F_ELT       0x02

/* JD_ASM_TOK_MNEMONIC */
#define JD_ASM_MNEM_F_ALIAS   0x01
#define JD_ASM_MNEM_F_WIDE    0x02  /* 需要追加 ".w" */
#define JD_ASM_MNEM_F_SETFLAGS 0x04 /* 需要追加 "s" */
#define JD_ASM_MNEM_F_COND    0x08  /* 后跟条件 token */
#define JD_ASM_MNEM_F_IT      0x10  /* IT 指令 */

/*
 * 一条 token，固定 8 字节。可以直接写文件、跨进程传。
 * 内部实现用的是同名布局的 jd_arm_token，两边由编译期断言保证一致。
 */
typedef struct {
    uint8_t  type;
    uint8_t  flags;
    uint16_t aux;
    uint32_t value;
} jd_asm_token;

/*
 * ---------------------------------------------------------------------------
 *  缓冲容器
 *
 *  所有指针都由调用方持有。库只往里写，绝不分配、绝不释放。
 *  建议用下面两个宏算容量，或者直接按每条指令 64 token / 64 字节字符串估。
 * ------------------------------------------------------------------------- */
typedef struct {
    jd_asm_token *tokens;
    size_t        token_cap;
    size_t        token_count;   /* 输出：写入的 token 数（含 END） */

    char         *strs;          /* 具名操作数的字符串池，可传 NULL */
    size_t        strs_cap;
    size_t        strs_len;      /* 输出：字符串池用量 */

    int           overflow;      /* 输出：1 表示缓冲不足，数据被截断 */
} jd_asm_buf;

#define JD_ASM_TOKENS_PER_INS  64
#define JD_ASM_STRS_PER_INS    64

/* 消息头：5 字节 "JDASM" + u2 版本 + u2 保留 + u4 token 数 + u4 字符串池长度 */
#define JD_ASM_TOKEN_SIZE   8
#define JD_ASM_MAGIC        "JDASM"
#define JD_ASM_VERSION      1
#define JD_ASM_HEADER_SIZE  17

/* ---------------------------------------------------------------------------
 *  API
 * ------------------------------------------------------------------------- */

/* 绑定调用方的缓冲。strs 可以传 NULL（只有具名操作数会用到）。 */
ROSEMARY_API void jd_asm_buf_init(jd_asm_buf *buf,
                                  jd_asm_token *tokens, size_t token_cap,
                                  char *strs, size_t strs_cap);

/* 清空长度保留缓冲，准备反汇编下一条指令 */
ROSEMARY_API void jd_asm_buf_reset(jd_asm_buf *buf);

/*
 * 反汇编一条指令，token 写到 buf->tokens[0..token_count)，以 END 结尾。
 *
 * 返回 1 成功、0 失败。失败时 token_count 会回滚成 0，
 * 不会留下半条指令。这是零分配的核心入口。
 */
ROSEMARY_API int jd_arm_disasm(const jd_asm_ins *ins, jd_asm_buf *buf);

/*
 * 批量反汇编。buf 的缓冲由调用方一次性给足，内部逐条 reset 复用，
 * 全程零分配。
 *
 *   offsets 可为 NULL；给了的话长度至少 count，写回每条指令在
 *   buf->tokens 里的起始下标（失败的写 0，用 ok 的返回值区分）。
 *
 * 返回成功反汇编的条数。token 是连续排布的：
 *   第 i 条 = buf->tokens[offsets[i] .. offsets[i+1])   （最后一条到 token_count）
 */
ROSEMARY_API size_t jd_arm_disasm_many(const jd_asm_ins *ins, size_t count,
                                       jd_asm_buf *buf, size_t *offsets);

/*
 * 直接把一条指令渲染成文本，写进调用方的 out（总是 NUL 结尾）。
 *
 * 正常路径应该是自己按 token 类型渲染；这个是给兜底 / 调试用的便利函数。
 * 返回写入的字节数（不含 NUL），缓冲不够返回 0。
 * 零分配，内部用约 1KB 栈空间。
 */
ROSEMARY_API int jd_arm_disasm_text(const jd_asm_ins *ins, char *out, size_t cap);

/*
 * 助记符 / 寄存器名。使用方自己按 token 渲染时需要它们：
 *   - JD_ASM_TOK_MNEMONIC 的 aux 是助记符下标，isd = 该 token 的 value
 *     （也就是模板下标），传 1 表示取助记符名、0 表示取别名指令的真名
 *   - JD_ASM_TOK_ALIAS 同理，用 aux + isd=0
 *
 * 这两个都不分配，返回的是库内部静态表里的字符串，不要 free。
 * 查不到返回 NULL。
 */
ROSEMARY_API const char *jd_asm_mnemonic_name(uint32_t isa, uint32_t mnemonic_index);
ROSEMARY_API const char *jd_asm_alias_name(uint32_t isa, uint32_t mnemonic_index);

/*
 * 序列化 buf 里当前这条指令流，写进调用方的字节缓冲。
 * 返回写出的字节数；缓冲不够返回 0，*need 是所需大小
 * （传 buf=NULL, cap=0 可以只问大小）。零分配。
 */
ROSEMARY_API size_t jd_asm_serialize(const jd_asm_buf *buf,
                                     uint8_t *out, size_t cap, size_t *need);

/*
 * 反序列化。token / 字符串写进调用方已备好的 buf，
 * 容量不够返回 0 并置 buf->overflow。
 */
ROSEMARY_API int jd_asm_deserialize(jd_asm_buf *buf, const uint8_t *in, size_t len);

/* ---------------------------------------------------------------------------
 *  C++ 符号名 demangle（Itanium C++ ABI）
 *
 *  符号表里的 C++ 名字是 mangled 的，直接显示给用户没法看：
 *
 *      _ZNSt6__ndk113basic_ostreamIcNS_11char_traitsIcEEElsEj
 *      -> std::__ndk1::basic_ostream<char, std::__ndk1::char_traits<char>
 *         >::operator<<(unsigned int)
 *
 *  out 由调用方提供，size 是它的字节数。返回非 0 表示成功并写出了结果。
 *
 *  返回 0 的两种情况，调用方都按“没有 demangle 结果”处理即可，不要当错误：
 *    - mangled 不是 Itanium ABI 名（必须 _Z 开头），比如 C 符号、汇编标签；
 *    - 名字太怪，这个实现解析不了。
 *  这两种情况下 out 都是空串（失败时可能被写过一半，本函数负责清干净）。
 * ------------------------------------------------------------------------- */
ROSEMARY_API int jd_demangle(char *out, size_t size, const char *mangled);

/* ---------------------------------------------------------------------------
 *  Swift 符号名 demangle（Swift ABI mangling）
 *
 *  Mach-O 里的 Swift 名字是另一套方案（$s / $S / _T0 前缀），和上面的
 *  Itanium C++ ABI 毫无关系 —— 所以是**两个入口**，不是一个函数兜两套。
 *  调用方按前缀自己分派即可：
 *
 *      if (jd_swift_is_mangled(sym))   jd_swift_demangle(buf, n, sym);
 *      else if (sym[0]=='_' && sym[1]=='Z')  jd_demangle(buf, n, sym);
 *
 *      $s4main3FooC                       -> main.Foo
 *      $sSaySiG                           -> [Swift.Int]
 *      $sSS_SutSg                         -> (Swift.String, Swift.UInt)?
 *      $sSiSg                             -> Swift.Int?
 *
 *  输出与 `xcrun swift-demangle` 对齐（27352 条真实符号逐条对拍一致）。
 *
 *  out 由调用方提供，size 是它的字节数。返回非 0 表示成功并写出了结果；
 *  返回 0 时 out 是空串，调用方按“没有 demangle 结果”处理，不要当错误。
 *
 *  **注意 `jd_swift_is_mangled` 比 `jd_swift_demangle` 认的范围宽一点**：
 *  `_T` 开头的老 mangling（Swift 3 那套）前者返回 1、后者必定返回 0。
 *  所以别拿 is_mangled 当“一定会成功”的保证，按返回值走就行。
 *
 *  线程安全：可以并发调用（内部状态是线程局部的）。
 * ------------------------------------------------------------------------- */
ROSEMARY_API int jd_swift_is_mangled(const char *mangled);
ROSEMARY_API int jd_swift_demangle(char *out, size_t size, const char *mangled);

#ifdef __cplusplus
}
#endif

#endif /* GARLIC_JD_ASM_EXPORT_H */
