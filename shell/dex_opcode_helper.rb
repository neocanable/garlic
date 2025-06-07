TEXT =<<-EOS
    0x00, "nop", kFmt10x, 1
    0x01, "move", kFmt12x, 1
    0x02, "move/from16", kFmt22x, 2
    0x03, "move/16", kFmt32x, 3
    0x04, "move-wide", kFmt12x, 1
    0x05, "move-wide/from16", kFmt22x, 2
    0x06, "move-wide/16", kFmt32x, 3
    0x07, "move-object", kFmt12x, 1
    0x08, "move-object/from16", kFmt22x, 2
    0x09, "move-object/16", kFmt32x, 3
    0x0A, "move-result", kFmt11x, 1
    0x0B, "move-result-wide", kFmt11x, 1
    0x0C, "move-result-object", kFmt11x, 1
    0x0D, "move-exception", kFmt11x, 1
    0x0E, "return-void", kFmt10x, 1
    0x0F, "return", kFmt11x, 1
    0x10, "return-wide", kFmt11x, 1
    0x11, "return-object", kFmt11x, 1
    0x12, "const/4", kFmt11n, 1
    0x13, "const/16", kFmt21s, 2
    0x14, "const", kFmt31i, 3
    0x15, "const/high16", kFmt21h, 2
    0x16, "const-wide/16", kFmt21s, 2
    0x17, "const-wide/32", kFmt31i, 3
    0x18, "const-wide", kFmt51l, 5
    0x19, "const-wide/high16", kFmt21h, 2
    0x1A, "const-string", kFmt21c, 2
    0x1B, "const-string/jumbo", kFmt31c, 3
    0x1C, "const-class", kFmt21c, 2
    0x1D, "monitor-enter", kFmt11x, 1
    0x1E, "monitor-exit", kFmt11x, 1
    0x1F, "check-cast", kFmt21c, 2
    0x20, "instance-of", kFmt22c, 2
    0x21, "array-length", kFmt12x, 1
    0x22, "new-instance", kFmt21c, 2
    0x23, "new-array", kFmt22c, 2
    0x24, "filled-new-array", kFmt35c, 3
    0x25, "filled-new-array/range", kFmt3rc, 3
    0x26, "fill-array-data", kFmt31t, 3
    0x27, "throw", kFmt11x, 1
    0x28, "goto", kFmt10t, 1
    0x29, "goto/16", kFmt20t, 2
    0x2A, "goto/32", kFmt30t, 3
    0x2B, "packed-switch", kFmt31t, 3
    0x2C, "sparse-switch", kFmt31t, 3
    0x2D, "cmpl-float", kFmt23x, 2
    0x2E, "cmpg-float", kFmt23x, 2
    0x2F, "cmpl-double", kFmt23x, 2
    0x30, "cmpg-double", kFmt23x, 2
    0x31, "cmp-long", kFmt23x, 2
    0x32, "if-eq", kFmt22t, 2
    0x33, "if-ne", kFmt22t, 2
    0x34, "if-lt", kFmt22t, 2
    0x35, "if-ge", kFmt22t, 2
    0x36, "if-gt", kFmt22t, 2
    0x37, "if-le", kFmt22t, 2
    0x38, "if-eqz", kFmt21t, 2
    0x39, "if-nez", kFmt21t, 2
    0x3A, "if-ltz", kFmt21t, 2
    0x3B, "if-gez", kFmt21t, 2
    0x3C, "if-gtz", kFmt21t, 2
    0x3D, "if-lez", kFmt21t, 2
    0x3E, "unused", kFmt10x, 1
    0x3F, "unused", kFmt10x, 1
    0x40, "unused", kFmt10x, 1
    0x41, "unused", kFmt10x, 1
    0x42, "unused", kFmt10x, 1
    0x43, "unused", kFmt10x, 1
    0x44, "aget", kFmt23x, 2
    0x45, "aget-wide", kFmt23x, 2
    0x46, "aget-object", kFmt23x, 2
    0x47, "aget-boolean", kFmt23x, 2
    0x48, "aget-byte", kFmt23x, 2
    0x49, "aget-char", kFmt23x, 2
    0x4A, "aget-short", kFmt23x, 2
    0x4B, "aput", kFmt23x, 2
    0x4C, "aput-wide", kFmt23x, 2
    0x4D, "aput-object", kFmt23x, 2
    0x4E, "aput-boolean", kFmt23x, 2
    0x4F, "aput-byte", kFmt23x, 2
    0x50, "aput-char", kFmt23x, 2
    0x51, "aput-short", kFmt23x, 2
    0x52, "iget", kFmt22c, 2
    0x53, "iget-wide", kFmt22c, 2
    0x54, "iget-object", kFmt22c, 2
    0x55, "iget-boolean", kFmt22c, 2
    0x56, "iget-byte", kFmt22c, 2
    0x57, "iget-char", kFmt22c, 2
    0x58, "iget-short", kFmt22c, 2
    0x59, "iput", kFmt22c, 2
    0x5A, "iput-wide", kFmt22c, 2
    0x5B, "iput-object", kFmt22c, 2
    0x5C, "iput-boolean", kFmt22c, 2
    0x5D, "iput-byte", kFmt22c, 2
    0x5E, "iput-char", kFmt22c, 2
    0x5F, "iput-short", kFmt22c, 2
    0x60, "sget", kFmt21c, 2
    0x61, "sget-wide", kFmt21c, 2
    0x62, "sget-object", kFmt21c, 2
    0x63, "sget-boolean", kFmt21c, 2
    0x64, "sget-byte", kFmt21c, 2
    0x65, "sget-char", kFmt21c, 2
    0x66, "sget-short", kFmt21c, 2
    0x67, "sput", kFmt21c, 2
    0x68, "sput-wide", kFmt21c, 2
    0x69, "sput-object", kFmt21c, 2
    0x6A, "sput-boolean", kFmt21c, 2
    0x6B, "sput-byte", kFmt21c, 2
    0x6C, "sput-char", kFmt21c, 2
    0x6D, "sput-short", kFmt21c, 2
    0x6E, "invoke-virtual", kFmt35c, 3
    0x6F, "invoke-super", kFmt35c, 3
    0x70, "invoke-direct", kFmt35c, 3
    0x71, "invoke-static", kFmt35c, 3
    0x72, "invoke-interface", kFmt35c, 3
    0x73, "unused", kFmt10x, 1
    0x74, "invoke-virtual/range", kFmt3rc, 3
    0x75, "invoke-super/range", kFmt3rc, 3
    0x76, "invoke-direct/range", kFmt3rc, 3
    0x77, "invoke-static/range", kFmt3rc, 3
    0x78, "invoke-interface/range", kFmt3rc, 3
    0x79, "unused", kFmt10x, 1
    0x7A, "unused", kFmt10x, 1
    0x7B, "neg-int", kFmt12x, 1
    0x7C, "not-int", kFmt12x, 1
    0x7D, "neg-long", kFmt12x, 1
    0x7E, "not-long", kFmt12x, 1
    0x7F, "neg-float", kFmt12x, 1
    0x80, "neg-double", kFmt12x, 1
    0x81, "int-to-long", kFmt12x, 1
    0x82, "int-to-float", kFmt12x, 1
    0x83, "int-to-double", kFmt12x, 1
    0x84, "long-to-int", kFmt12x, 1
    0x85, "long-to-float", kFmt12x, 1
    0x86, "long-to-double", kFmt12x, 1
    0x87, "float-to-int", kFmt12x, 1
    0x88, "float-to-long", kFmt12x, 1
    0x89, "float-to-double", kFmt12x, 1
    0x8A, "double-to-int", kFmt12x, 1
    0x8B, "double-to-long", kFmt12x, 1
    0x8C, "double-to-float", kFmt12x, 1
    0x8D, "int-to-byte", kFmt12x, 1
    0x8E, "int-to-char", kFmt12x, 1
    0x8F, "int-to-short", kFmt12x, 1
    0x90, "add-int", kFmt23x, 2
    0x91, "sub-int", kFmt23x, 2
    0x92, "mul-int", kFmt23x, 2
    0x93, "div-int", kFmt23x, 2
    0x94, "rem-int", kFmt23x, 2
    0x95, "and-int", kFmt23x, 2
    0x96, "or-int", kFmt23x, 2
    0x97, "xor-int", kFmt23x, 2
    0x98, "shl-int", kFmt23x, 2
    0x99, "shr-int", kFmt23x, 2
    0x9A, "ushr-int", kFmt23x, 2
    0x9B, "add-long", kFmt23x, 2
    0x9C, "sub-long", kFmt23x, 2
    0x9D, "mul-long", kFmt23x, 2
    0x9E, "div-long", kFmt23x, 2
    0x9F, "rem-long", kFmt23x, 2
    0xA0, "and-long", kFmt23x, 2
    0xA1, "or-long", kFmt23x, 2
    0xA2, "xor-long", kFmt23x, 2
    0xA3, "shl-long", kFmt23x, 2
    0xA4, "shr-long", kFmt23x, 2
    0xA5, "ushr-long", kFmt23x, 2
    0xA6, "add-float", kFmt23x, 2
    0xA7, "sub-float", kFmt23x, 2
    0xA8, "mul-float", kFmt23x, 2
    0xA9, "div-float", kFmt23x, 2
    0xAA, "rem-float", kFmt23x, 2
    0xAB, "add-double", kFmt23x, 2
    0xAC, "sub-double", kFmt23x, 2
    0xAD, "mul-double", kFmt23x, 2
    0xAE, "div-double", kFmt23x, 2
    0xAF, "rem-double", kFmt23x, 2
    0xB0, "add-int/2addr", kFmt12x, 1
    0xB1, "sub-int/2addr", kFmt12x, 1
    0xB2, "mul-int/2addr", kFmt12x, 1
    0xB3, "div-int/2addr", kFmt12x, 1
    0xB4, "rem-int/2addr", kFmt12x, 1
    0xB5, "and-int/2addr", kFmt12x, 1
    0xB6, "or-int/2addr", kFmt12x, 1
    0xB7, "xor-int/2addr", kFmt12x, 1
    0xB8, "shl-int/2addr", kFmt12x, 1
    0xB9, "shr-int/2addr", kFmt12x, 1
    0xBA, "ushr-int/2addr", kFmt12x, 1
    0xBB, "add-long/2addr", kFmt12x, 1
    0xBC, "sub-long/2addr", kFmt12x, 1
    0xBD, "mul-long/2addr", kFmt12x, 1
    0xBE, "div-long/2addr", kFmt12x, 1
    0xBF, "rem-long/2addr", kFmt12x, 1
    0xC0, "and-long/2addr", kFmt12x, 1
    0xC1, "or-long/2addr", kFmt12x, 1
    0xC2, "xor-long/2addr", kFmt12x, 1
    0xC3, "shl-long/2addr", kFmt12x, 1
    0xC4, "shr-long/2addr", kFmt12x, 1
    0xC5, "ushr-long/2addr", kFmt12x, 1
    0xC6, "add-float/2addr", kFmt12x, 1
    0xC7, "sub-float/2addr", kFmt12x, 1
    0xC8, "mul-float/2addr", kFmt12x, 1
    0xC9, "div-float/2addr", kFmt12x, 1
    0xCA, "rem-float/2addr", kFmt12x, 1
    0xCB, "add-double/2addr", kFmt12x, 1
    0xCC, "sub-double/2addr", kFmt12x, 1
    0xCD, "mul-double/2addr", kFmt12x, 1
    0xCE, "div-double/2addr", kFmt12x, 1
    0xCF, "rem-double/2addr", kFmt12x, 1
    0xD0, "add-int/lit16", kFmt22s, 2
    0xD1, "rsub-int", kFmt22s, 2
    0xD2, "mul-int/lit16", kFmt22s, 2
    0xD3, "div-int/lit16", kFmt22s, 2
    0xD4, "rem-int/lit16", kFmt22s, 2
    0xD5, "and-int/lit16", kFmt22s, 2
    0xD6, "or-int/lit16", kFmt22s, 2
    0xD7, "xor-int/lit16", kFmt22s, 2
    0xD8, "add-int/lit8", kFmt22b, 2
    0xD9, "rsub-int/lit8", kFmt22b, 2
    0xDA, "mul-int/lit8", kFmt22b, 2
    0xDB, "div-int/lit8", kFmt22b, 2
    0xDC, "rem-int/lit8", kFmt22b, 2
    0xDD, "and-int/lit8", kFmt22b, 2
    0xDE, "or-int/lit8", kFmt22b, 2
    0xDF, "xor-int/lit8", kFmt22b, 2
    0xE0, "shl-int/lit8", kFmt22b, 2
    0xE1, "shr-int/lit8", kFmt22b, 2
    0xE2, "ushr-int/lit8", kFmt22b, 2
    0xE3, "copy-basic-block", kFmt10x, 1
    0xE4, "unused", kFmt10x, 1
    0xE5, "unused", kFmt10x, 1
    0xE6, "unused", kFmt10x, 1
    0xE7, "unused", kFmt10x, 1
    0xE8, "unused", kFmt10x, 1
    0xE9, "unused", kFmt10x, 1
    0xEA, "unused", kFmt10x, 1
    0xEB, "unused", kFmt10x, 1
    0xEC, "unused", kFmt10x, 1
    0xED, "unused", kFmt10x, 1
    0xEE, "unused", kFmt10x, 1
    0xEF, "unused", kFmt10x, 1
    0xF0, "unused", kFmt10x, 1
    0xF1, "unused", kFmt10x, 1
    0xF2, "unused", kFmt10x, 1
    0xF3, "unused", kFmt10x, 1
    0xF4, "unused", kFmt10x, 1
    0xF5, "unused", kFmt10x, 1
    0xF6, "unused", kFmt10x, 1
    0xF7, "unused", kFmt10x, 1
    0xF8, "unused", kFmt10x, 1
    0xF9, "unused", kFmt10x, 1
    0xFA, "invoke-polymorphic", kFmt45cc, 4
    0xFB, "invoke-polymorphic/range", kFmt4rcc, 4
    0xFC, "invoke-custom", kFmt35c, 3
    0xFD, "invoke-custom/range", kFmt3rc, 3
    0xFE, "const-method-handle", kFmt21c, 2
    0xFF, "const-method-type", kFmt21c, 2
EOS

class Intruction
  attr_accessor :code, :name, :fmt, :len, :marco_name
  def initialize(code, name, fmt, len)
    self.code       = code.strip
    self.name       = name.strip
    self.fmt        = fmt.strip
    self.marco_name = "DEX_INS_#{name.gsub(" ", "").gsub('/', "_").gsub('-', "_").upcase}"
    self.len        = len
  end
end

class DEX_OPCODE
  @@opcode = TEXT.lines.collect{|e| e.split(",") }.collect{|arr|
    code = arr[0]
    name = arr[1].to_s.gsub("\"", "").gsub(" ", "")
    fmt = arr[2]
    len = arr[3]

    Intruction.new(code, name, fmt, len)
  }


  class << self
    def opcode
      @@opcode
    end

    def fmt_list
      opcode.collect{|e| e.fmt }.uniq
    end

    def len_list
      opcode.collect{|e| e.len }.uniq
    end

    def ident(level)
      (0..level).collect{" "}.join
    end

    def case_with_level(arr, level)
      arr.each do |i|
        puts "#{ident(level)}case #{i.marco_name}:"
      end
    end

    def opcode_name_method
      puts "extern string dex_opcode_name(u1 code)"
      puts "{"
      puts "#{ident(4)}switch(code) {"
      opcode.each do |ins|
        next if ins.name == "unused"
        puts "#{ident(8)}case #{ins.marco_name}: return \"#{ins.name}\";"
      end
      puts "#{ident(8)}default: return (string)g_str_unknown;"

      puts "#{ident(4)}}"
      puts "}"
    end

    def opcode_len_method
      puts "extern int dex_opcode_len(u1 code)"
      puts "{"
      puts "#{ident(4)}switch(code) {"
      len_list.each do |num|
        arr = opcode.select{|i| i.len == num && i.name != "unused"}
        case_with_level(arr, 8)
        puts "#{ident(12)}return #{num.strip};"
      end
      puts "#{ident(8)}default: return 0;"

      puts "#{ident(4)}}"
      puts "}"
    end

    def opcode_fmt_method
      puts "extern dex_instruction_format dex_opcode_fmt(u1 code)"
      puts "{"
      puts "#{ident(4)}switch(code) {"
      fmt_list.each do |fmt|
        arr = opcode.select{|i| i.fmt == fmt && i.name != "unused"}
        case_with_level(arr, 8)
        puts "#{ident(12)}return #{fmt.strip};"
      end
      puts "#{ident(8)}default: return 0;"

      puts "#{ident(4)}}"
      puts "}"
    end

  end
end

puts DEX_OPCODE.opcode_fmt_method
