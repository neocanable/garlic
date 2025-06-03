#include "class_tools.h"

char* get_method_access_flags_str(u2 access_flag)
{
    char *access = x_alloc(40);

    if ((be16toh(access_flag) & METHOD_ACC_PUBLIC) != 0)
        strcat(access, "public ");
    if ((be16toh(access_flag) & METHOD_ACC_PRIVATE) != 0)
        strcat(access, "private ");
    if ((be16toh(access_flag) & METHOD_ACC_PROTECTED) != 0)
        strcat(access, "protected ");
    if ((be16toh(access_flag) & METHOD_ACC_STATIC) != 0)
        strcat(access, "static ");
    if ((be16toh(access_flag) & METHOD_ACC_FINAL) != 0)
        strcat(access, "final ");
    if ((be16toh(access_flag) & METHOD_ACC_SYNCHRONIZED) != 0)
        strcat(access, "synchronized ");
    if ((be16toh(access_flag) & METHOD_ACC_BRIDGE) != 0)
        strcat(access, "bridge ");
    if ((be16toh(access_flag) & METHOD_ACC_VARARGS) != 0)
        strcat(access, "varargs ");
    if ((be16toh(access_flag) & METHOD_ACC_NATIVE) != 0)
        strcat(access, "native ");
    if ((be16toh(access_flag) & METHOD_ACC_ABSTRACT) != 0)
        strcat(access, "abstract ");
    if ((be16toh(access_flag) & METHOD_ACC_STRICT) != 0)
        strcat(access, "strict ");
    if ((be16toh(access_flag) & METHOD_ACC_SYNTHETIC) != 0)
        strcat(access, "synthetic ");

    return access;
}

char* get_class_access_flags_str(u2 access_flag)
{
    char *access = x_alloc(40);
    if ((be16toh(access_flag) & CLASS_ACC_PUBLIC) != 0)
        strcat(access, "public ");
    if ((be16toh(access_flag) & CLASS_ACC_FINAL) != 0)
        strcat(access, "final ");
    if ((be16toh(access_flag) & CLASS_ACC_INTERFACE) != 0)
        strcat(access, "interface ");
    if ((be16toh(access_flag) & CLASS_ACC_ANNOTATION) != 0)
        strcat(access, "annotation ");
    if ((be16toh(access_flag) & CLASS_ACC_ABSTRACT) != 0)
        strcat(access, "abstract ");
    if ((be16toh(access_flag) & CLASS_ACC_ENUM) != 0)
        strcat(access, "strict ");
    if ((be16toh(access_flag) & CLASS_ACC_SYNTHETIC) != 0)
        strcat(access, "synthetic ");

    return access;
}






