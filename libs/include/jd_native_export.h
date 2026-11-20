#ifndef GARLIC_JD_NATIVE_EXPORT_H
#define GARLIC_JD_NATIVE_EXPORT_H

/*
 * 反汇编那条链的对外定义（jd_asm_ins / jd_arm_token / jd_asm_buf …）。
 * Thyme 之类的使用方 dlopen 本库之后，两个头文件都要用：
 * 先用这里的 jd_analysis_elf_from_path 做分析，
 * 再用 jd_dump_asm_binary 把指令 dump 成二进制，
 * 最后按 jd_asm_export.h 里的 token 定义自己渲染。
 */
#include <stdio.h>
#include "jd_asm_export.h"

/*
 * 使用方只看得到不透明句柄。
 *
 * 这里写 `struct jd_native *` 而不是 `jd_native *`：后者那个 typedef 由库内部的
 * parser/elf/elf.h 定义，本头**不能**去 include 它（它会牵出一大串内部头），
 * 也不能在这里再 typedef 一次（同一 TU 里重复 typedef 在 C99 下不合法）。
 * 路径参数同理用 const char*，不引 string 那个内部 typedef。
 */
struct jd_native;

/* ROSEMARY_API 在 jd_export.h —— 四个对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"

ROSEMARY_API struct jd_native* jd_analysis_elf_from_path(const char *path);

/*
 * 把「文件里的绝对地址」换算成分析器内部的归一化地址（PE 的 image_base+RVA、
 * Mach-O 的 VM 地址）。已经是归一化的、或不在镜像里的一律原样返回。
 *
 * 单函数分析的入口过这道，所以调用方传 objdump/IDA 显示的地址即可。
 */
ROSEMARY_API uint64_t jd_native_norm_entry_pc(struct jd_native *elf, uint64_t pc);

/*
 * 直接从内存里的镜像字节分析，不落盘。两种容器各一份，语义一一对应。
 *
 * 和 from_path 的唯一区别：**不写测试**。测试文件名是「<输入路径>.<后缀>」
 * 拼出来的，这条路上没有输入路径。要测试就用 from_path。
 *
 * buffer 的所有权仍属于调用方（库不 free 它）。返回的句柄里可能有指向它的
 * 指针 —— 符号名、字符串都是直接引用镜像里的字节 —— 所以 buffer 要活到
 * jd_analysis_release() 之后才能释放。
 */
ROSEMARY_API struct jd_native* jd_analysis_elf_from_buffer(unsigned char *buffer, size_t size);

/*
 * Mach-O（arm64）分析。走的是和 ELF 完全同一条分析管线 —— 差别只在
 * loader 换成了 macho_loader，analyzer 侧一行没变。
 *
 * 输入可以是薄镜像，也可以是 fat/universal（会自动抽 arm64 slice）。
 * 分析完同样在文件旁边写测试（.exports / .imports / .strings / ...）。
 */
ROSEMARY_API struct jd_native* jd_analysis_macho_from_path(const char *path);

/* PE（x86-64）；和 Mach-O 同一套装配 */
ROSEMARY_API struct jd_native* jd_analysis_pe_from_path(const char *path);

/* 同上，Mach-O 的 buffer 版 —— 语义和 elf 那份完全一致 */
ROSEMARY_API struct jd_native* jd_analysis_macho_from_buffer(unsigned char *buffer, size_t size);

/*
 * 单函数分析：只从 `pc` 这一个地址做递归下降，不做全量入口收集
 * （不扫符号/FDE/序言、不跑插件、不扫全库 vtable），也**不跟直接调用目标**。
 * 适合「用户点名一个函数，只想反编译它」的场景 —— 大库上比全量分析快一个
 * 数量级，内存也不再随库大小线性增长（仍然和地址空间/重定位表大小有关）。
 *
 * 返回的句柄已经可以交给 jd_analysis_function_at 用。
 *
 * 注意：
 *   - 不会写 .entries/.exports 那些测试侧车 —— 单函数分析出来的"全库视图"
 *     是半截的，写出去会误导；
 *   - 32 位 ARM 的 Thumb 入口和符号入口一样，地址可以带 bit0（`pc | 1`）；
 *   - 需要跨函数的信息（被调者签名、vtable、全库字符串）不会完整 —— 调用方
 *     如果要做全库交叉引用，还是走 jd_analysis_*_from_path。
 */
ROSEMARY_API struct jd_native* jd_analysis_elf_from_path_at(const char *path,
                                                            unsigned long long pc);
ROSEMARY_API struct jd_native* jd_analysis_macho_from_path_at(const char *path,
                                                              unsigned long long pc);
ROSEMARY_API struct jd_native* jd_analysis_pe_from_path_at(const char *path,
                                                           unsigned long long pc);

/*
 * 释放 jd_analysis_* 返回的句柄，连带它持有的字符串和各个数组。
 *
 * 传 NULL 是安全的。释放之后不要再读任何从这个句柄拿到的指针 —— 包括
 * jd_image_export.h 里那些 name / lib（它们都由库持有，跟着句柄一起走）。
 */
ROSEMARY_API void jd_analysis_release(struct jd_native *elf);

// ROSEMARY_API void jd_dump_all_csv(jd_native *elf);

/*
 * 把分析结果里的指令按 pc 顺序 dump 成 jd_asm_ins 裸数组，写进 stream。
 *
 * 文件内容就是定长记录数组，没有文件头：
 *     条数 = 文件大小 / sizeof(jd_asm_ins)
 * 每条含 pc / code / isa / tpl_index，使用方读出来逐条喂给
 * jd_arm_disasm 就能重建 token。
 *
 * 零分配。返回写出的指令条数，失败返回 0。
 */
ROSEMARY_API size_t jd_dump_asm_binary(struct jd_native *elf, FILE *stream);

#endif // GARLIC_JD_NATIVE_EXPORT_H
