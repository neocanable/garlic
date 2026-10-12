#ifndef GARLIC_JD_ELF_EXPORT_H
#define GARLIC_JD_ELF_EXPORT_H

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
 * 这里写 `struct jd_elf *` 而不是 `jd_elf *`：后者那个 typedef 由库内部的
 * parser/elf/elf.h 定义，本头**不能**去 include 它（它会牵出一大串内部头），
 * 也不能在这里再 typedef 一次（同一 TU 里重复 typedef 在 C99 下不合法）。
 * 路径参数同理用 const char*，不引 string 那个内部 typedef。
 */
struct jd_elf;

/* ROSEMARY_API 在 jd_export.h —— 四个对外头共用一份，别在这里再抄一遍 */
#include "jd_export.h"

ROSEMARY_API struct jd_elf* jd_analysis_elf_from_path(const char *path);

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
ROSEMARY_API struct jd_elf* jd_analysis_elf_from_buffer(unsigned char *buffer, size_t size);

/*
 * Mach-O（arm64）分析。走的是和 ELF 完全同一条分析管线 —— 差别只在
 * loader 换成了 macho_loader，analyzer 侧一行没变。
 *
 * 输入可以是薄镜像，也可以是 fat/universal（会自动抽 arm64 slice）。
 * 分析完同样在文件旁边写测试（.exports / .imports / .strings / ...）。
 */
ROSEMARY_API struct jd_elf* jd_analysis_macho_from_path(const char *path);

/* 同上，Mach-O 的 buffer 版 —— 语义和 elf 那份完全一致 */
ROSEMARY_API struct jd_elf* jd_analysis_macho_from_buffer(unsigned char *buffer, size_t size);

/*
 * 释放 jd_analysis_* 返回的句柄，连带它持有的字符串和各个数组。
 *
 * 传 NULL 是安全的。释放之后不要再读任何从这个句柄拿到的指针 —— 包括
 * jd_image_export.h 里那些 name / lib（它们都由库持有，跟着句柄一起走）。
 */
ROSEMARY_API void jd_analysis_release(struct jd_elf *elf);

// ROSEMARY_API void jd_dump_all_csv(jd_elf *elf);

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
ROSEMARY_API size_t jd_dump_asm_binary(struct jd_elf *elf, FILE *stream);

#endif // GARLIC_JD_ELF_EXPORT_H
