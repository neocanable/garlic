#ifndef GARLIC_PE_TOOLS_H
#define GARLIC_PE_TOOLS_H

#include <string.h>
#include "parser/pe/pe.h"

#define jpe_read(pe, ptr, size) (jd_bin_read(pe->bin, ptr, size))
#define jpe_read1(pe, ptr) (jpe_read(pe, ptr, sizeof(u1)))
#define jpe_read2(pe, ptr) (jpe_read(pe, ptr, sizeof(u2)))
#define jpe_read4(pe, ptr) (jpe_read(pe, ptr, sizeof(u4)))
#define jpe_read8(pe, ptr) (jpe_read(pe, ptr, sizeof(u8)))

static inline bool is32bit(pe_file *pe)
{
    return pe->nt_header->size_of_optional_header == 0x00E0;
}

static inline bool pe_section_has_flag(pe_section_header *header, u4 flag)
{
    return (header->characteristics & flag) != 0;
}

static inline u8 rva_to_offset(pe_file *pe, u8 rva)
{
    for (int i = pe->nt_header->number_of_sections-1; i >= 0; --i) {
        pe_section_header *h = &pe->section_headers[i];
        if (rva >= h->virtual_address)
            return rva - h->virtual_address + h->pointer_to_raw_data;
    }
    return 0;
}

static inline void set_pe_off_of_rva(pe_file *pe, u8 rva)
{
    pe->bin->cur_off = rva_to_offset(pe, rva);
}

#endif //GARLIC_PE_TOOLS_H
