#ifndef GARLIC_PE_CONST_H
#define GARLIC_PE_CONST_H

#define MAGE_FILE_MACHINE_UNKNOWN 		0x0000 		// unknown
#define IMAGE_FILE_MACHINE_ALPHA 		0x0184 		// Alpha AXP, 32-bit address space
#define IMAGE_FILE_MACHINE_ALPHA64 		0x0284 		// Alpha 64, 64-bit address space
#define IMAGE_FILE_MACHINE_AM33 		0x01d3 		// Matsushita AM33
#define IMAGE_FILE_MACHINE_AMD64 		0x8664 		// X64
#define IMAGE_FILE_MACHINE_ARM 			0x01c0 		// ARM little endian
#define IMAGE_FILE_MACHINE_ARM64 		0xaa64 		// ARM64 little endian
#define IMAGE_FILE_MACHINE_ARMNT 		0x01c4 		// ARM Thumb-2 little endian
#define IMAGE_FILE_MACHINE_AXP64 		0x0284 		// AXP 64 (Same as Alpha 64)
#define IMAGE_FILE_MACHINE_EBC 			0x0ebc 		// EFI byte code
#define IMAGE_FILE_MACHINE_I386 		0x014c 		// Intel 386 or later processors and compatible processors
#define IMAGE_FILE_MACHINE_IA64 		0x0200 		// Intel Itanium processor family
#define IMAGE_FILE_MACHINE_LOONGARCH32 	0x6232 		// LoongArch 32-bit processor family
#define IMAGE_FILE_MACHINE_LOONGARCH64 	0x6264 		// LoongArch 64-bit processor family
#define IMAGE_FILE_MACHINE_M32R 		0x9041 		// Mitsubishi M32R little endian
#define IMAGE_FILE_MACHINE_MIPS16 		0x0266 		// MIPS16
#define IMAGE_FILE_MACHINE_MIPSFPU 		0x0366 		// MIPS with FPU
#define IMAGE_FILE_MACHINE_MIPSFPU16 	0x0466 		// MIPS16 with FPU
#define IMAGE_FILE_MACHINE_POWERPC 		0x01f0 		// Power PC little endian
#define IMAGE_FILE_MACHINE_POWERPCFP 	0x01f1 		// Power PC with floating point support
#define IMAGE_FILE_MACHINE_R4000 		0x0166 		// MIPS little endian
#define IMAGE_FILE_MACHINE_RISCV32 		0x5032 		// RISC-V 32-bit address space
#define IMAGE_FILE_MACHINE_RISCV64 		0x5064 		// RISC-V 64-bit address space
#define IMAGE_FILE_MACHINE_RISCV128 	0x5128 		// RISC-V 128-bit address space
#define IMAGE_FILE_MACHINE_SH3 			0x01a2 		// Hitachi SH3
#define IMAGE_FILE_MACHINE_SH3DSP 		0x01a3 		// Hitachi SH3 DSP
#define IMAGE_FILE_MACHINE_SH4 			0x01a6 		// Hitachi SH4
#define IMAGE_FILE_MACHINE_SH5 			0x01a8 		// Hitachi SH5
#define IMAGE_FILE_MACHINE_THUMB 		0x01c2 		// Thumb
#define IMAGE_FILE_MACHINE_WCEMIPSV2 	0x0169 		// MIPS little-endian WCE v2

#define IMAGE_FILE_RELOCS_STRIPPED 0x0001 
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002 
#define IMAGE_FILE_LINE_NUMS_STRIPPED 0x0004 
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED 0x0008 
#define IMAGE_FILE_AGGRESSIVE_WS_TRIM 0x0010 
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020 
#define IMAGE_FILE_NOT_USED 0x0040 
#define IMAGE_FILE_BYTES_REVERSED_LO 0x0080 
#define IMAGE_FILE_32BIT_MACHINE 0x0100 
#define IMAGE_FILE_DEBUG_STRIPPED 0x0200 
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400 
#define IMAGE_FILE_NET_RUN_FROM_SWAP 0x0800 
#define IMAGE_FILE_SYSTEM 0x1000 
#define IMAGE_FILE_DLL 0x2000 
#define IMAGE_FILE_UP_SYSTEM_ONLY 0x4000 
#define IMAGE_FILE_BYTES_REVERSED_HI 0x8000 

// Subsystem 标志
#define IMAGE_SUBSYSTEM_UNKNOWN                      0  // 未知子系统
#define IMAGE_SUBSYSTEM_NATIVE                       1  // 不需要子系统。设备驱动和本机系统进程
#define IMAGE_SUBSYSTEM_WINDOWS_GUI                  2  // Windows 图形用户接口（GUI）子系统
#define IMAGE_SUBSYSTEM_WINDOWS_CUI                  3  // Windows 字符模式用户接口子（CUI）系统
#define IMAGE_SUBSYSTEM_OS2_CUI                      5  //  OS/2 CUI 子系统
#define IMAGE_SUBSYSTEM_POSIX_CUI                    7  // POSIX CUI 子系统
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI               9  // Windows CE 系统
#define IMAGE_SUBSYSTEM_EFI_APPLICATION             10  // 可扩展固件接口（EFI）应用程序
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVEICE_DRIVER    11  // 带引导服务的 EFI 驱动程序
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER          12  // 带运行时服务的 EFI 驱动程序
#define IMAGE_SUBSYSTEM_EFI_ROM                     13  // EFI ROM 映像
#define IMAGE_SUBSYSTEM_XBOX                        14  // XBOX 系统
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION    16  // 引导应用程序

// DLL 属性标志
// 0x0001 0x0002 0x0004 0x0008 保留，值必须为 0。
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE             0x0040  // DLL 可以在加载时重定位
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY          0x0080  // 强制实行代码完整性检验
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT                0x0100  // 映像兼容数据执行保护（DEP）
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION             0x0200  // 映像可以隔离，但不应该被隔离
#define IMAGE_DLLCHARACTERISTICS_NO_SEH                   0x0400  // 映像不使用结构化异常处理（SEH）
#define IMAGE_DLLCHARACTERISTICS_NO_BIND                  0x0800  // 不绑定映像
//#define IMAGE_DLLCHARACTERISTICS_APPCONTAINER           0x1000  // 在 32-bit 保留；64-bit 表示映像必须在 AppContainer 内执行
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER               0x2000  // WDM 驱动
//#define IMAGE_DLLCHARACTERISTICS_GUARD_CF               0x4000  // 在 32-bit 保留；64-bit 表示映像支持控制流保护
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE    0x8000  // 映像可用于终端服务器

// 数据目录索引
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // 导出目录
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // 导入目录
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // 资源目录
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // 异常目录
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // 安全目录
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // 基址重定位表
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // 调试目录
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // 架构特定数据
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // 全局指针
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS目录
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // 加载配置目录
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // 绑定导入目录
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // 导入地址表（IAT）
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // 延迟加载导入描述符
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM 运行时描述符

#define IMAGE_SCN_CNT_CODE                0x00000020  // 节区包含代码
#define IMAGE_SCN_CNT_INITIALIZED_DATA    0x00000040  // 节区包含已初始化数据
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA  0x00000080  // 节区包含未初始化数据
#define IMAGE_SCN_ALIGN_1BYTES            0x00100000  // 1-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_2BYTES            0x00200000  // 2-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_4BYTES            0x00300000  // 4-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_8BYTES            0x00400000  // 8-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_16BYTES           0x00500000  // 16-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_32BYTES           0x00600000  // 32-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_64BYTES           0x00700000  // 64-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_128BYTES          0x00800000  // 128-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_256BYTES          0x00900000  // 256-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_512BYTES          0x00A00000  // 512-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_1024BYTES         0x00B00000  // 1024-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_2048BYTES         0x00C00000  // 2048-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_4096BYTES         0x00D00000  // 4096-byte 对齐。仅用于目标文件
#define IMAGE_SCN_ALIGN_8192BYTES         0x00E00000  // 8192-byte 对齐。仅用于目标文件
#define IMAGE_SCN_LNK_NRELOC_OVFL         0x01000000  // 节区包含扩展的重定位项
#define IMAGE_SCN_MEM_DISCARDABLE         0x02000000  // 节区可根据需要丢弃，如 .reloc 在进程开始后被丢弃
#define IMAGE_SCN_MEM_NOT_CACHED          0x04000000  // 节区不会被缓存
#define IMAGE_SCN_MEM_NOT_PAGED           0x08000000  // 节区不可分页
#define IMAGE_SCN_MEM_SHARED              0x10000000  // 节区可共享给不同进程
#define IMAGE_SCN_MEM_EXECUTE             0x20000000  // 节区可作为代码执行
#define IMAGE_SCN_MEM_READ                0x40000000  // 节区可读
#define IMAGE_SCN_MEM_WRITE               0x80000000  // 节区可写

static u4 SECTION_HEADER_FLAGS[] = {
    IMAGE_SCN_CNT_CODE,
    IMAGE_SCN_CNT_INITIALIZED_DATA,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA,
    IMAGE_SCN_ALIGN_1BYTES,
    IMAGE_SCN_ALIGN_2BYTES,
    IMAGE_SCN_ALIGN_4BYTES,
    IMAGE_SCN_ALIGN_8BYTES,
    IMAGE_SCN_ALIGN_16BYTES,
    IMAGE_SCN_ALIGN_32BYTES,
    IMAGE_SCN_ALIGN_64BYTES,
    IMAGE_SCN_ALIGN_128BYTES,
    IMAGE_SCN_ALIGN_256BYTES,
    IMAGE_SCN_ALIGN_512BYTES,
    IMAGE_SCN_ALIGN_1024BYTES,
    IMAGE_SCN_ALIGN_2048BYTES,
    IMAGE_SCN_ALIGN_4096BYTES,
    IMAGE_SCN_ALIGN_8192BYTES,
    IMAGE_SCN_LNK_NRELOC_OVFL,
    IMAGE_SCN_MEM_DISCARDABLE,
    IMAGE_SCN_MEM_NOT_CACHED,
    IMAGE_SCN_MEM_NOT_PAGED,
    IMAGE_SCN_MEM_SHARED,
    IMAGE_SCN_MEM_EXECUTE,
    IMAGE_SCN_MEM_READ,
    IMAGE_SCN_MEM_WRITE
};


typedef enum CorTypeAttr
{
    // Use this mask to retrieve the type visibility information.

    tdVisibilityMask        =   0x00000007,
    tdNotPublic             =   0x00000000,
    // Class is not public scope.

    tdPublic                =   0x00000001,
    // Class is public scope.

    tdNestedPublic          =   0x00000002,
    // Class is nested with public visibility.

    tdNestedPrivate         =   0x00000003,
    // Class is nested with private visibility.

    tdNestedFamily          =   0x00000004,
    // Class is nested with family visibility.

    tdNestedAssembly        =   0x00000005,
    // Class is nested with assembly visibility.

    tdNestedFamANDAssem     =   0x00000006,
    // Class is nested with family and assembly visibility.

    tdNestedFamORAssem      =   0x00000007,
    // Class is nested with family or assembly visibility.


    // Use this mask to retrieve class layout information

    tdLayoutMask            =   0x00000018,
    tdAutoLayout            =   0x00000000,
    // Class fields are auto-laid out

    tdSequentialLayout      =   0x00000008,
    // Class fields are laid out sequentially

    tdExplicitLayout        =   0x00000010,
    // Layout is supplied explicitly

    // end layout mask


    // Use this mask to retrieve class semantics information.

    tdClassSemanticsMask    =   0x00000060,
    tdClass                 =   0x00000000,
    // Type is a class.

    tdInterface             =   0x00000020,
    // Type is an interface.

    // end semantics mask


    // Special semantics in addition to class semantics.

    tdAbstract              =   0x00000080,
    // Class is abstract

    tdSealed                =   0x00000100,
    // Class is concrete and may not be extended

    tdSpecialName           =   0x00000400,
    // Class class_name is special. Name describes how.


    // Implementation attributes.

    tdImport                =   0x00001000,
    // Class / interface is imported

    tdSerializable          =   0x00002000,
    // The class is Serializable.


    // Use tdStringFormatMask to retrieve string information for native interop

    tdStringFormatMask      =   0x00030000,
    tdAnsiClass             =   0x00000000,
    // LPTSTR is interpreted as ANSI in this class

    tdUnicodeClass          =   0x00010000,
    // LPTSTR is interpreted as UNICODE

    tdAutoClass             =   0x00020000,
    // LPTSTR is interpreted automatically

    tdCustomFormatClass     =   0x00030000,
    // A non-standard encoding specified by CustomFormatMask

    tdCustomFormatMask      =   0x00C00000,
    // Use this mask to retrieve non-standard encoding

    // information for native interop.

    // The meaning of the values of these 2 bits is unspecified.


    // end string format mask


    tdBeforeFieldInit       =   0x00100000,
    // Initialize the class any time before first static field access.

    tdForwarder             =   0x00200000,
    // This ExportedType is a type forwarder.


    // Flags reserved for runtime use.

    tdReservedMask          =   0x00040800,
    tdRTSpecialName         =   0x00000800,
    // Runtime should check class_name encoding.

    tdHasSecurity           =   0x00040000,
    // Class has security associate with it.

} CorTypeAttr;

typedef enum CorFieldAttr
{
    // member access mask - Use this mask to retrieve

    // accessibility information.

    fdFieldAccessMask           =   0x0007,
    fdPrivateScope              =   0x0000,
    // Member not referenceable.

    fdPrivate                   =   0x0001,
    // Accessible only by the parent type.

    fdFamANDAssem               =   0x0002,
    // Accessible by sub-types only in this Assembly.

    fdAssembly                  =   0x0003,
    // Accessibly by anyone in the Assembly.

    fdFamily                    =   0x0004,
    // Accessible only by type and sub-types.

    fdFamORAssem                =   0x0005,
    // Accessibly by sub-types anywhere, plus anyone in assembly.

    fdPublic                    =   0x0006,
    // Accessibly by anyone who has visibility to this scope.

    // end member access mask

    // field contract attributes.

    fdStatic                    =   0x0010,
    // Defined on type, else per instance.

    fdInitOnly                  =   0x0020,
    // Field may only be initialized, not written to after init.

    fdLiteral                   =   0x0040,
    // Value is compile time constant.

    fdNotSerialized             =   0x0080,
    // Field does not have to be serialized when type is remoted.

    fdSpecialName               =   0x0200,
    // field is special. Name describes how.

    // interop attributes

    fdPinvokeImpl               =   0x2000,
    // Implementation is forwarded through pinvoke.

    // Reserved flags for runtime use only.

    fdReservedMask              =   0x9500,
    fdRTSpecialName             =   0x0400,
    // Runtime(metadata internal APIs) should check class_name encoding.

    fdHasFieldMarshal           =   0x1000,
    // Field has marshalling information.

    fdHasDefault                =   0x8000,
    // Field has default.

    fdHasFieldRVA               =   0x0100,
    // Field has RVA.

} CorFieldAttr;

typedef enum CorMethodAttr
{
    // member access mask - Use this mask to retrieve

    // accessibility information.

    mdMemberAccessMask          =   0x0007,
    mdPrivateScope              =   0x0000,
    // Member not referenceable.

    mdPrivate                   =   0x0001,
    // Accessible only by the parent type.

    mdFamANDAssem               =   0x0002,
    // Accessible by sub-types only in this Assembly.

    mdAssem                     =   0x0003,
    // Accessibly by anyone in the Assembly.

    mdFamily                    =   0x0004,
    // Accessible only by type and sub-types.

    mdFamORAssem                =   0x0005,
    // Accessibly by sub-types anywhere, plus anyone in assembly.

    mdPublic                    =   0x0006,
    // Accessibly by anyone who has visibility to this scope.

    // end member access mask


    // m contract attributes.

    mdStatic                    =   0x0010,
    // Defined on type, else per instance.

    mdFinal                     =   0x0020,
    // Method may not be overridden.

    mdVirtual                   =   0x0040,
    // Method virtual.

    mdHideBySig                 =   0x0080,
    // Method hides by class_name+sig, else just by class_name.


    // vtable layout mask - Use this mask to retrieve vtable attributes.

    mdVtableLayoutMask          =   0x0100,
    mdReuseSlot                 =   0x0000,     // The default.

    mdNewSlot                   =   0x0100,
    // Method always gets a new slot in the vtable.

    // end vtable layout mask


    // m implementation attributes.

    mdCheckAccessOnOverride     =   0x0200,
    // Overridability is the same as the visibility.

    mdAbstract                  =   0x0400,
    // Method does not provide an implementation.

    mdSpecialName               =   0x0800,
    // Method is special. Name describes how.


    // interop attributes

    mdPinvokeImpl               =   0x2000,
    // Implementation is forwarded through pinvoke.

    mdUnmanagedExport           =   0x0008,
    // Managed m exported via thunk to unmanaged code.


    // Reserved flags for runtime use only.

    mdReservedMask              =   0xd000,
    mdRTSpecialName             =   0x1000,
    // Runtime should check class_name encoding.

    mdHasSecurity               =   0x4000,
    // Method has security associate with it.

    mdRequireSecObject          =   0x8000,
    // Method calls another m containing security code.


} CorMethodAttr;

typedef enum CorMethodImpl
{
    // code impl mask
    miCodeTypeMask      =   0x0003,   // Flags about code type.
    miIL                =   0x0000,   // Method impl is IL.
    miNative            =   0x0001,   // Method impl is native.
    miOPTIL             =   0x0002,   // Method impl is OPTIL
    miRuntime           =   0x0003,   // Method impl is provided by the runtime.
    // end code impl mask

    // managed mask
    miManagedMask       =   0x0004,   // Flags specifying whether the code is managed
    // or unmanaged.
    miUnmanaged         =   0x0004,   // Method impl is unmanaged, otherwise managed.
    miManaged           =   0x0000,   // Method impl is managed.
    // end managed mask

    // implementation info and interop
    miForwardRef        =   0x0010,   // Indicates m is defined; used primarily
    // in merge scenarios.
    miPreserveSig       =   0x0080,   // Indicates m sig is not to be mangled to
    // do HRESULT conversion.

    miInternalCall      =   0x1000,   // Reserved for internal use.

    miSynchronized      =   0x0020,   // Method is single threaded through the body.
    miNoInlining        =   0x0008,   // Method may not be inlined.
    miMaxMethodImplVal  =   0xffff,   // Range check value
} CorMethodImpl;

typedef enum CorParamAttr
{
    pdIn                        =   0x0001,     // Param is [In]

    pdOut                       =   0x0002,     // Param is [out]

    pdOptional                  =   0x0010,     // Param is optional


    // Reserved flags for Runtime use only.

    pdReservedMask              =   0xf000,
    pdHasDefault                =   0x1000,     // Param has default value.

    pdHasFieldMarshal           =   0x2000,     // Param has FieldMarshal.


    pdUnused                    =   0xcfe0,
} CorParamAttr;

typedef enum CorEventAttr
{
    evSpecialName           =   0x0200,
    // event is special. Name describes how.


    // Reserved flags for Runtime use only.

    evReservedMask          =   0x0400,
    evRTSpecialName         =   0x0400,
    // Runtime(metadata internal APIs) should check class_name encoding.

} CorEventAttr;

typedef enum CorPropertyAttr
{
    prSpecialName           =   0x0200,
    // property is special. Name describes how.


    // Reserved flags for Runtime use only.

    prReservedMask          =   0xf400,
    prRTSpecialName         =   0x0400,
    // Runtime(metadata internal APIs) should check class_name encoding.

    prHasDefault            =   0x1000,     // Property has default


    prUnused                =   0xe9ff,
} CorPropertyAttr;

typedef enum CorMethodSemanticsAttr
{
    msSetter    =   0x0001,     // Setter for property

    msGetter    =   0x0002,     // Getter for property

    msOther     =   0x0004,     // other m for property or event

    msAddOn     =   0x0008,     // AddOn m for event

    msRemoveOn  =   0x0010,     // RemoveOn m for event

    msFire      =   0x0020,     // Fire m for event

} CorMethodSemanticsAttr;

typedef enum  CorPinvokeMap
{
    pmNoMangle          = 0x0001,
    // Pinvoke is to use the member class_name as specified.


    // Use this mask to retrieve the CharSet information.

    pmCharSetMask       = 0x0006,
    pmCharSetNotSpec    = 0x0000,
    pmCharSetAnsi       = 0x0002,
    pmCharSetUnicode    = 0x0004,
    pmCharSetAuto       = 0x0006,

    pmBestFitUseAssem   = 0x0000,
    pmBestFitEnabled    = 0x0010,
    pmBestFitDisabled   = 0x0020,
    pmBestFitMask       = 0x0030,

    pmThrowOnUnmappableCharUseAssem   = 0x0000,
    pmThrowOnUnmappableCharEnabled    = 0x1000,
    pmThrowOnUnmappableCharDisabled   = 0x2000,
    pmThrowOnUnmappableCharMask       = 0x3000,

    pmSupportsLastError = 0x0040,
    // Information about target function. Not relevant for fields.


    // None of the calling convention flags is relevant for fields.

    pmCallConvMask      = 0x0700,
    pmCallConvWinapi    = 0x0100,
    // Pinvoke will use native callconv appropriate to target windows platform.

    pmCallConvCdecl     = 0x0200,
    pmCallConvStdcall   = 0x0300,
    pmCallConvThiscall  = 0x0400,
    // In M9, pinvoke will raise exception.

    pmCallConvFastcall  = 0x0500,

    pmMaxValue          = 0xFFFF,
} CorPinvokeMap;

typedef enum CorAssemblyFlags
{
    afPublicKey             =   0x0001,
    // The assembly ref holds the full (unhashed) public key.


    afPA_None               =   0x0000,
    // Processor Architecture unspecified

    afPA_MSIL               =   0x0010,
    // Processor Architecture: neutral (PE32)

    afPA_x86                =   0x0020,
    // Processor Architecture: x86 (PE32)

    afPA_IA64               =   0x0030,
    // Processor Architecture: Itanium (PE32+)

    afPA_AMD64              =   0x0040,
    // Processor Architecture: AMD X64 (PE32+)

    afPA_Specified          =   0x0080,
    // Propagate PA flags to AssemblyRef record

    afPA_Mask               =   0x0070,
    // Bits describing the processor architecture

    afPA_FullMask           =   0x00F0,
    // Bits describing the PA incl. Specified

    afPA_Shift              =   0x0004,
    // NOT A FLAG, shift count in PA flags <--> index conversion


    afEnableJITcompileTracking  =   0x8000, // From "DebuggableAttribute".

    afDisableJITcompileOptimizer=   0x4000, // From "DebuggableAttribute".


    afRetargetable          =   0x0100,
    // The assembly can be retargeted (at runtime) to an

    //  assembly from a different publisher.

} CorAssemblyFlags;

typedef enum CorFileFlags
{
    ffContainsMetaData      =   0x0000,
    // This is not a resource file

    ffContainsNoMetaData    =   0x0001,
    // This is a resource file or other non-metadata-containing file

} CorFileFlags;

typedef enum CorManifestResourceFlags
{
    mrVisibilityMask        =   0x0007,
    mrPublic                =   0x0001,
    // The Resource is exported from the Assembly.

    mrPrivate               =   0x0002,
    // The Resource is private to the Assembly.

} CorManifestResourceFlags;

typedef enum CorGenericParamAttr
{
    // Variance of type parameters, only applicable to generic parameters
    // for generic interfaces and delegates
    gpVarianceMask          =   0x0003,
    gpNonVariant            =   0x0000,
    gpCovariant             =   0x0001,
    gpContravariant         =   0x0002,

    // Special constraints, applicable to any type parameters
    gpSpecialConstraintMask =  0x001C,
    gpNoSpecialConstraint   =   0x0000,
    gpReferenceTypeConstraint = 0x0004,      // type argument must be a reference type
    gpNotNullableValueTypeConstraint   =   0x0008,      // type argument must be a value
    // type but not Nullable
    gpDefaultConstructorConstraint = 0x0010, // type argument must have a public
    // default constructor
} CorGenericParamAttr;

#endif //GARLIC_PE_CONST_H
