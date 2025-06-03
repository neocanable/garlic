text = <<-EOS
add_opcode_to_map(class_opcode, 0x00, 0, "nop", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x01, 0, "aconst_null", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x02, 0, "iconst_m1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x03, 0, "iconst_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x04, 0, "iconst_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x05, 0, "iconst_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x06, 0, "iconst_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x07, 0, "iconst_4", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x08, 0, "iconst_5", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x09, 0, "lconst_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0a, 0, "lconst_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0b, 0, "fconst_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0c, 0, "fconst_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0d, 0, "fconst_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0e, 0, "dconst_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x0f, 0, "dconst_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x10, 1, "bipush", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x11, 2, "sipush", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x12, 1, "ldc", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x13, 2, "ldc_w", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x14, 2, "ldc2_w", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x15, 1, "iload", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x16, 1, "lload", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x17, 1, "fload", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x18, 1, "dload", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x19, 1, "aload", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1a, 0, "iload_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1b, 0, "iload_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1c, 0, "iload_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1d, 0, "iload_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1e, 0, "lload_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x1f, 0, "lload_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x20, 0, "lload_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x21, 0, "lload_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x22, 0, "fload_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x23, 0, "fload_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x24, 0, "fload_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x25, 0, "fload_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x26, 0, "dload_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x27, 0, "dload_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x28, 0, "dload_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x29, 0, "dload_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x2a, 0, "aload_0", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x2b, 0, "aload_1", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x2c, 0, "aload_2", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x2d, 0, "aload_3", 0, 0, 1);
add_opcode_to_map(class_opcode, 0x2e, 0, "iaload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x2f, 0, "laload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x30, 0, "faload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x31, 0, "daload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x32, 0, "aaload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x33, 0, "baload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x34, 0, "caload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x35, 0, "saload", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x36, 1, "istore", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x37, 1, "lstore", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x38, 1, "fstore", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x39, 1, "dstore", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3a, 1, "astore", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3b, 0, "istore_0", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3c, 0, "istore_1", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3d, 0, "istore_2", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3e, 0, "istore_3", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x3f, 0, "lstore_0", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x40, 0, "lstore_1", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x41, 0, "lstore_2", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x42, 0, "lstore_3", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x43, 0, "fstore_0", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x44, 0, "fstore_1", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x45, 0, "fstore_2", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x46, 0, "fstore_3", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x47, 0, "dstore_0", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x48, 0, "dstore_1", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x49, 0, "dstore_2", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4a, 0, "dstore_3", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4b, 0, "astore_0", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4c, 0, "astore_1", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4d, 0, "astore_2", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4e, 0, "astore_3", 0, 1, 0);
add_opcode_to_map(class_opcode, 0x4f, 0, "iastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x50, 0, "lastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x51, 0, "fastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x52, 0, "dastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x53, 0, "aastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x54, 0, "bastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x55, 0, "castore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x56, 0, "sastore", 0, 3, 0);
add_opcode_to_map(class_opcode, 0x57, 0, "pop", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x58, 0, "pop2", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x59, 0, "dup", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5a, 0, "dup_x1", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5b, 0, "dup_x2", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5c, 0, "dup2", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5d, 0, "dup2_x1", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5e, 0, "dup2_x2", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x5f, 0, "swap", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x60, 0, "iadd", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x61, 0, "ladd", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x62, 0, "fadd", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x63, 0, "dadd", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x64, 0, "isub", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x65, 0, "lsub", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x66, 0, "fsub", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x67, 0, "dsub", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x68, 0, "imul", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x69, 0, "lmul", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6a, 0, "fmul", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6b, 0, "dmul", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6c, 0, "idiv", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6d, 0, "ldiv", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6e, 0, "fdiv", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x6f, 0, "ddiv", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x70, 0, "irem", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x71, 0, "lrem", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x72, 0, "frem", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x73, 0, "drem", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x74, 0, "ine", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x75, 0, "lne", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x76, 0, "fne", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x77, 0, "dne", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x78, 0, "ishl", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x79, 0, "lshl", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7a, 0, "ishr", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7b, 0, "lshr", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7c, 0, "iushr", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7d, 0, "lushr", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7e, 0, "iand", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x7f, 0, "land", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x80, 0, "ior", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x81, 0, "lor", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x82, 0, "ixor", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x83, 0, "lxor", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x84, 2, "iinc", 0, 0, 0);
add_opcode_to_map(class_opcode, 0x85, 0, "i2l", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x86, 0, "i2f", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x87, 0, "i2d", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x88, 0, "l2i", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x89, 0, "l2f", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8a, 0, "l2d", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8b, 0, "f2i", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8c, 0, "f2l", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8d, 0, "f2d", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8e, 0, "d2i", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x8f, 0, "d2l", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x90, 0, "d2f", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x91, 0, "i2b", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x92, 0, "i2c", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x93, 0, "i2s", 0, 1, 1);
add_opcode_to_map(class_opcode, 0x94, 0, "lcmp", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x95, 0, "fcmpl", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x96, 0, "fcmp", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x97, 0, "dcmpl", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x98, 0, "dcmp", 0, 2, 1);
add_opcode_to_map(class_opcode, 0x99, 2, "ifeq", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9a, 2, "ifne", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9b, 2, "iflt", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9c, 2, "ifge", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9d, 2, "ifgt", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9e, 2, "ifle", 1, 1, 0);
add_opcode_to_map(class_opcode, 0x9f, 2, "if_icmpeq", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa0, 2, "if_icmpne", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa1, 2, "if_icmplt", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa2, 2, "if_icmpge", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa3, 2, "if_icmpgt", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa4, 2, "if_icmple", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa5, 2, "if_acmpeq", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa6, 2, "if_acmpne", 1, 2, 0);
add_opcode_to_map(class_opcode, 0xa7, 2, "goto", 1, 0, 0);
add_opcode_to_map(class_opcode, 0xa8, 2, "jsr", 1, 0, 0);
add_opcode_to_map(class_opcode, 0xa9, 1, "ret", 1, 0, 0);
add_opcode_to_map(class_opcode, 0xaa, 16, "tableswitch", 1, 1, 0);
add_opcode_to_map(class_opcode, 0xab, 8, "lookupswitch", 1, 1, 0);
add_opcode_to_map(class_opcode, 0xac, 0, "ireturn", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xad, 0, "lreturn", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xae, 0, "freturn", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xaf, 0, "dreturn", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xb0, 0, "areturn", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xb1, 0, "return", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xb2, 2, "getstatic", 0, 0, 1);
add_opcode_to_map(class_opcode, 0xb3, 2, "putstatic", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xb4, 2, "getfield", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xb5, 2, "putfield", 0, 2, 0);
add_opcode_to_map(class_opcode, 0xb6, 2, "invokevirtual", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xb7, 2, "invokespecial", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xb8, 2, "invokestatic", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xb9, 4, "invokeinterface", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xba, 4, "invokedynamic", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xbb, 2, "new", 0, 0, 1);
add_opcode_to_map(class_opcode, 0xbc, 1, "newarray", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xbd, 2, "anewarray", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xbe, 0, "arraylength", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xbf, 0, "athrow", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xc0, 2, "checkcast", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xc1, 2, "instanceof", 0, 1, 1);
add_opcode_to_map(class_opcode, 0xc2, 0, "monitorenter", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xc3, 0, "monitorexit", 0, 1, 0);
add_opcode_to_map(class_opcode, 0xc4, 3, "wide", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xc5, 3, "multianewarray", 0, 0, 1);
add_opcode_to_map(class_opcode, 0xc6, 2, "ifnull", 1, 1, 0);
add_opcode_to_map(class_opcode, 0xc7, 2, "ifnonnull", 1, 1, 0);
add_opcode_to_map(class_opcode, 0xc8, 4, "goto_w", 1, 0, 0);
add_opcode_to_map(class_opcode, 0xc9, 4, "jsr_w", 1, 0, 0);
add_opcode_to_map(class_opcode, 0xca, 0, "breakpoint", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xfe, 0, "impdep1", 0, 0, 0);
add_opcode_to_map(class_opcode, 0xff, 0, "leavefinally", 0, 0, 0);
EOS

def generate_opcode_name(text)
  text.lines.collect{|e| e.split(",") }.each do |arr|
    code = arr[1]
    param_len = arr[2]
    name = arr[3]
    is_jump = arr[4]
    popped_count = arr[5]
    pushed_count = arr[6]
    # puts "  case #{code}:"
    # puts "    return #{param_len};"
    puts "jd_jvm_opcode jvm_opcode_#{name.gsub("\"", "").strip} = {#{code}, #{pushed_count.gsub(");", "").strip}, #{popped_count}, #{param_len}, #{is_jump}, #{name}};"
  end
end

# generate_opcode_name(text)


def generate_opcode_name_switch(text)
  puts "switch(ins->code) {"
  text.lines.collect{|e| e.split(",") }.each do |arr|
    code = arr[1]
    param_len = arr[2]
    name = arr[3]
    is_jump = arr[4]
    popped_count = arr[5]
    pushed_count = arr[6]
    puts "  case #{code}: ins->op = &jvm_opcode_#{name.gsub("\"", "").strip}; break;"
  end
  puts "}"
end

generate_opcode_name_switch(text);
