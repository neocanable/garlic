#ifndef GARLIC_JD_EXPORT_H
#define GARLIC_JD_EXPORT_H

/*
 * ============================================================================
 *  ROSEMARY_API —— 对外符号的可见性宏
 *
 *  这个宏原来在四个对外头里各抄了一份，而且已经漂移了：jd_asm_export.h 和
 *  jd_elf_export.h 是**无条件重定义**，jd_image_export.h 和 jd_plist_export.h
 *  带 #ifndef 保护。同一个库里两种写法。
 *
 *  后果不只是难看：一个使用方的 TU 先包含带保护的头（宏已定义），再包含不带
 *  保护的头，后者会把它**重新定义一遍**。两边内容一旦分叉，轻的是
 *  `-Wmacro-redefined`，重的是某个符号没拿到 default 可见性 —— 编得过去、
 *  dlopen 时 dlsym 取不到，报出来像是「库坏了」。
 *
 *  所以收到这里，只此一份。四个对外头都 `#include "jd_export.h"`。
 *
 *  本头是自包含的：不引任何其它头。
 * ============================================================================
 */

#ifndef ROSEMARY_API
#if defined(ROSEMARY_STATIC)
    /* 静态库模式：不做任何可见性标注 */
    #define ROSEMARY_API
#elif defined(_MSC_VER)
    #ifdef EXPORTING_ROSEMARY
        #define ROSEMARY_API __declspec(dllexport)
    #else
        #define ROSEMARY_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #ifdef EXPORTING_ROSEMARY
        #define ROSEMARY_API __attribute__((visibility("default")))
    #else
        #define ROSEMARY_API
    #endif
#else
    #define ROSEMARY_API
#endif
#endif /* ROSEMARY_API */

#endif // GARLIC_JD_EXPORT_H
