#include <stdio.h>
#include <sys/param.h>
#include <stdarg.h>
#include "parser/pe/metadata.h"

#include "libs/memory/mem_pool.h"
#include "parser/pe/pe_tools.h"

#define DEBUG_PE_FILE_PARSER true

static pe_dos_header* init_pe_content(mem_pool *pool, string path)
{
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    pe_file *pe = make_obj_in(pe_file, pool);
    pe->pool = pool;
    pe->bin = make_obj_in(jd_bin, pool);
    pe->bin->buffer_size = file_size;
    pe->bin->buffer = x_alloc_in(pool, file_size);
    pe->bin->cur_off = 0;
    fread(pe->bin->buffer, 1, file_size, file);
    fclose(file);
    return pe;
}

static void write_dos_header(pe_file *pe)
{
    printf("DOS HEADER\n");
    pe_dos_header *header = pe->dos_header;
    printf("\t%-15s: %04x\n", "[magic]",      header->magic);
    printf("\t%-15s: %04x\n", "[e_cblp]",     header->e_cblp);
    printf("\t%-15s: %04x\n", "[e_cp]",       header->e_cp);
    printf("\t%-15s: %04x\n", "[e_crlc]",     header->e_crlc);
    printf("\t%-15s: %04x\n", "[e_cparhdr]",  header->e_cparhdr);
    printf("\t%-15s: %04x\n", "[e_minalloc]", header->e_minalloc);
    printf("\t%-15s: %04x\n", "[e_maxalloc]", header->e_maxalloc);
    printf("\t%-15s: %04x\n", "[e_ss]",       header->e_ss);
    printf("\t%-15s: %04x\n", "[e_sp]",       header->e_sp);
    printf("\t%-15s: %04x\n", "[e_csum]",     header->e_csum);
    printf("\t%-15s: %04x\n", "[e_ip]",       header->e_ip);
    printf("\t%-15s: %04x\n", "[e_cs]",       header->e_cs);
    printf("\t%-15s: %04x\n", "[e_lfarlc]",   header->e_lfarlc);
    printf("\t%-15s: %04x\n", "[e_ovno]",     header->e_ovno);
    printf("\t%-15s:", "[e_res]");
    for (int i = 0; i < 4; ++i) {
        printf(" %04x ", header->e_res[i]);
        if (i == 3) printf("\n");
    }
    printf("\t%-15s: %04x\n", "[e_oemid]",    header->e_oemid);
    printf("\t%-15s: %04x\n", "[e_oeminfo]",  header->e_oeminfo);
    printf("\t%-15s:", "[e_res2]");
    for (int i = 0; i < 10; ++i) {
        printf(" %04x ", header->e_res2[i]);
        if (i == 9) printf("\n");
    }
    printf("\t%-15s: %08x\n", "[e_lfanew]", header->e_lfanew);
}

static void parse_dos_header(pe_file *pe)
{
    pe_dos_header *header = make_obj_in(pe_dos_header, pe->pool);
    pe->dos_header = header;
    jpe_read(pe, header, sizeof(pe_dos_header));

#if DEBUG_PE_FILE_PARSER
    write_dos_header(pe);
#endif
}

static void parse_dos_stub(pe_file *pe)
{
    pe_dos_stub *stub = make_obj_in(pe_dos_stub, pe->pool);
    pe->stub = stub;

    size_t size = pe->dos_header->e_lfanew - pe->bin->cur_off;
    stub->size = size;
    stub->data = x_alloc_in(pe->pool, stub->size);
    jpe_read(pe, stub->data, stub->size);

#if DEBUG_PE_FILE_PARSER
    printf("DOS STUB\n");
    printf("\t[dos stub]: %d\n", stub->size);
#endif
}

static void write_pe_nt_header(pe_file *pe)
{
    pe_nt_header *nt_header = pe->nt_header;
    printf("NT HEADER\n");
    printf("\t%-25s: %08x\n", "[signature]", 
            nt_header->signature);
    printf("\t%-25s: %04x\n", "[machine]", 
            nt_header->machine);
    printf("\t%-25s: %04x\n", "[number_of_sections]", 
            nt_header->number_of_sections);
    printf("\t%-25s: %08x\n", "[time_stamp]", 
            nt_header->time_stamp);
    printf("\t%-25s: %08x\n", "[pointer_to_symbol_table]", 
            nt_header->pointer_to_symbol_table);
    printf("\t%-25s: %08x\n", "[number_of_symbols]", 
            nt_header->number_of_symbols);
    printf("\t%-25s: %04x\n", "[size_of_optional_header]", 
            nt_header->size_of_optional_header);
    printf("\t%-25s: %04x\n", "[characteristics]", 
            nt_header->characteristics);
}

static void parse_nt_header(pe_file *pe)
{
    pe_nt_header *nt_header = make_obj_in(pe_nt_header, pe->pool);
    pe->nt_header = nt_header;
    jpe_read(pe, nt_header, sizeof(pe_nt_header));
#if DEBUG_PE_FILE_PARSER
    write_pe_nt_header(pe);
#endif
}

static void write_optional_header(pe_file *pe)
{
    pe_optional_header *header = pe->optional_header;
    printf("OPTIONAL HEADER\n");
    printf("\t%-35s: %04x\n", "[magic]", header->magic);
    printf("\t%-35s: %02x\n", "[major_linker_version]",
           header->major_linker_version);
    printf("\t%-35s: %02x\n", "[minor_linker_version]",
           header->minor_linker_version);
    printf("\t%-35s: %08x\n", "[size_of_code]",
           header->size_of_code);
    printf("\t%-35s: %08x\n", "[size_of_initialized_data]",
           header->size_of_initialized_data);
    printf("\t%-35s: %08x\n", "[size_of_uninitialized_data]",
           header->size_of_uninitialized_data);
    printf("\t%-35s: %08x\n", "[address_of_entry_point]",
           header->address_of_entry_point);
    printf("\t%-35s: %08x\n", "[base_of_code]",
           header->base_of_code);
    printf("\t%-35s: %08x\n", "[base_of_data]",
           header->base_of_data);
    printf("\t%-35s: %08x\n", "[image_base]", header->image_base);
    printf("\t%-35s: %08x\n", "[section_alignment]",
           header->section_alignment);
    printf("\t%-35s: %08x\n", "[file_alignment]",
           header->file_alignment);
    printf("\t%-35s: %04x\n", "[major_operating_system_version]",
           header->major_operating_system_version);
    printf("\t%-35s: %04x\n", "[minor_operating_system_version]",
           header->minor_operating_system_version);
    printf("\t%-35s: %04x\n", "[major_image_version]",
           header->major_image_version);
    printf("\t%-35s: %04x\n", "[minor_image_version]",
           header->minor_image_version);
    printf("\t%-35s: %04x\n", "[major_subsystem_version]",
           header->major_subsystem_version);
    printf("\t%-35s: %04x\n", "[minor_subsystem_version]",
           header->minor_subsystem_version);
    printf("\t%-35s: %08x\n", "[win32_version_value]",
           header->win32_version_value);
    printf("\t%-35s: %08x\n", "[size_of_image]",
           header->size_of_image);
    printf("\t%-35s: %08x\n", "[size_of_headers]",
           header->size_of_headers);
    printf("\t%-35s: %08x\n", "[checksum]", header->checksum);
    printf("\t%-35s: %04x\n", "[subsystem]", header->subsystem);
    printf("\t%-35s: %04x\n", "[dll_characteristics]",
           header->dll_characteristics);
    printf("\t%-35s: %08llx\n", "[size_of_stack_reserve]",
           header->size_of_stack_reserve);
    printf("\t%-35s: %08llx\n", "[size_of_stack_commit]",
           header->size_of_stack_commit);
    printf("\t%-35s: %08llx\n", "[size_of_heap_reserve]",
           header->size_of_heap_reserve);
    printf("\t%-35s: %08llx\n", "[size_of_heap_commit]",
           header->size_of_heap_commit);
    printf("\t%-35s: %08x\n", "[loader_flags]",
           header->loader_flags);
    printf("\t%-35s: %08x\n", "[number_of_rva_and_sizes]",
           header->number_of_rva_and_sizes);
    printf("\t%-35s:", "[data_directory]");
    for (int i = 0; i < header->number_of_rva_and_sizes; ++i) {
        pe_data_directory *d = &header->data_directory[i];
        printf(" %08x %08x ", d->virtual_address, d->size);
        if (i == 15) printf("\n");
    }
}

static void write_data_dirctory(pe_file *pe)
{
    pe_optional_header *header = pe->optional_header;
    printf("DATA DIRECTORY\n");
    for (int i = 0; i < header->number_of_rva_and_sizes; ++i) {
        pe_data_directory *d = &header->data_directory[i];
        if (d->size == 0) continue;
        printf("\t[%2d] virtual_address: %08x size: %08x\n", i,
               d->virtual_address, d->size);
    }
}

static void parse_nt_optional_header(pe_file *pe)
{
    pe->optional_header = make_obj_in(pe_optional_header, pe->pool);
    pe_optional_header *header = pe->optional_header;
    jpe_read2(pe, &header->magic);
    jpe_read1(pe, &header->major_linker_version);
    jpe_read1(pe, &header->minor_linker_version);
    jpe_read4(pe, &header->size_of_code);
    jpe_read4(pe, &header->size_of_initialized_data);
    jpe_read4(pe, &header->size_of_uninitialized_data);
    jpe_read4(pe, &header->address_of_entry_point);
    jpe_read4(pe, &header->base_of_code);
    jpe_read4(pe, &header->base_of_data);
    jpe_read4(pe, &header->image_base);
    jpe_read4(pe, &header->section_alignment);
    jpe_read4(pe, &header->file_alignment);
    jpe_read2(pe, &header->major_operating_system_version);
    jpe_read2(pe, &header->minor_operating_system_version);
    jpe_read2(pe, &header->major_image_version);
    jpe_read2(pe, &header->minor_image_version);
    jpe_read2(pe, &header->major_subsystem_version);
    jpe_read2(pe, &header->minor_subsystem_version);
    jpe_read4(pe, &header->win32_version_value);
    jpe_read4(pe, &header->size_of_image);
    jpe_read4(pe, &header->size_of_headers);
    jpe_read4(pe, &header->checksum);
    jpe_read2(pe, &header->subsystem);
    jpe_read2(pe, &header->dll_characteristics);
    if (is32bit(pe)) {
        jpe_read4(pe, &header->size_of_stack_reserve);
        jpe_read4(pe, &header->size_of_stack_commit);
        jpe_read4(pe, &header->size_of_heap_reserve);
        jpe_read4(pe, &header->size_of_heap_commit);
    }
    else {
        jpe_read8(pe, &header->size_of_stack_reserve);
        jpe_read8(pe, &header->size_of_stack_commit);
        jpe_read8(pe, &header->size_of_heap_reserve);
        jpe_read8(pe, &header->size_of_heap_commit);
    }
    jpe_read4(pe, &header->loader_flags); // discard
    jpe_read4(pe, &header->number_of_rva_and_sizes);
    jpe_read(pe, header->data_directory, sizeof(pe_data_directory)*16);


#if DEBUG_PE_FILE_PARSER
    write_optional_header(pe);
    write_data_dirctory(pe);
#endif
}

static void write_section_header(pe_section_header *header)
{
    printf("SECTION HEADER\n");
    printf("\t%-25s: %s\n", "[class_name]",
           header->name);
    printf("\t%-25s: %08x\n", "[virtual_size]",
           header->virtual_size);
    printf("\t%-25s: %08x\n", "[virtual_address]",
           header->virtual_address);
    printf("\t%-25s: %08x\n", "[size_of_raw_data]",
           header->size_of_raw_data);
    printf("\t%-25s: %08x\n", "[pointer_to_raw_data]",
           header->pointer_to_raw_data);
    printf("\t%-25s: %08x\n", "[pointer_to_relocations]",
           header->pointer_to_relocations);
    printf("\t%-25s: %08x\n", "[pointer_to_linenumbers]",
           header->pointer_to_linenumbers);
    printf("\t%-25s: %04x\n", "[number_of_relocations]",
           header->number_of_relocations);
    printf("\t%-25s: %04x\n", "[number_of_linenumbers]",
           header->number_of_linenumbers);
    printf("\t%-25s: %08x\n", "[characteristics]",
           header->characteristics);
    printf("\t%-25s:\n", "[flags]");
    for (int i = 0; i < 25; ++i) {
        if (pe_section_has_flag(header, SECTION_HEADER_FLAGS[i])) {
            printf("\t\t%08x\n", SECTION_HEADER_FLAGS[i]);
        }
    }
}

static void parse_section_header(pe_file *pe)
{
    pe_nt_header *nt_header = pe->nt_header;

    pe->section_headers = make_obj_arr_in(pe_section_header, 
                    nt_header->number_of_sections, pe->pool);

    for (int i = 0; i < nt_header->number_of_sections; ++i) {
        pe_section_header *section_header = &pe->section_headers[i];
        jpe_read(pe, section_header, sizeof(pe_section_header));
#if DEBUG_PE_FILE_PARSER
        write_section_header(section_header);
#endif
    }
}

static void write_import_table(pe_file *pe)
{
    printf("IMPORT TABLE\n");
    for (int i = 0; i < pe->import_table_size; ++i) {
        pe_import_directory *import = &pe->import_table[i];
        string name = &pe->bin->buffer[rva_to_offset(pe, import->name)];
        printf("\t[%2d] %s %08x %08x %08x %08x %08x \n",
               i,
               name,
               import->original_first_thunk,
               import->time_date_stamp,
               import->forwarder_chain,
               import->name,
               import->first_thunk);
        for (int j = 0; j < import->trunk_size; ++j) {
            pe_import_trunk_data *t = &import->trunks[j];
            if (t->name->name == NULL) {
                printf("\t\t%04x NONAME\n", t->name->hint);
            }
            else {
                printf("\t\t%04x %s\n", t->name->hint, t->name->name);
            }
        }
    }
}

static void inline read_import_trunk_data(pe_file *pe, pe_import_trunk_data *data)
{
    if (is32bit(pe)) {
        jpe_read4(pe, &data->u1.forwarder_string);
    }
    else {
        jpe_read8(pe, &data->u1.forwarder_string);
    }
}

static inline bool trunk_data_no_name(pe_file *pe, pe_import_trunk_data *data)
{
    if (is32bit(pe)) {
//        return data->u1.forwarder_string & 0x80000000;
        return (u4)data->u1.forwarder_string >> 31;
    }
    else {
        return (u8)data->u1.forwarder_string >> 63;
    }
}

static void write_export_table(pe_file *pe)
{
    pe_export_directory *export = pe->export_table;
    printf("EXPORT TABLE\n");
    printf("\t%-25s: %08x\n", "[characteristics]", export->characteristics);
    printf("\t%-25s: %08x\n", "[time_date_stamp]", export->time_date_stamp);
    printf("\t%-25s: %04x\n", "[major_version]", export->major_version);
    printf("\t%-25s: %04x\n", "[minor_version]", export->minor_version);
    printf("\t%-25s: %08x\n", "[class_name]", export->name);
    printf("\t%-25s: %08x\n", "[base]", export->base);
    printf("\t%-25s: %08x\n", "[number_of_functions]", export->number_of_functions);
    printf("\t%-25s: %08x\n", "[number_of_names]", export->number_of_names);
    printf("\t%-25s: %08x\n", "[address_of_functions]", export->address_of_functions);
    printf("\t%-25s: %08x\n", "[address_of_names]", export->address_of_names);
    printf("\t%-25s: %08x\n", "[address_of_name_ordinals]", export->address_of_name_ordinals);

    for (int i = 0; i < export->number_of_functions; ++i) {
        u4 func = export->functions[i];
        int j;
        for (j = 0; j < export->number_of_names; ++j) {
            if (export->name_ordinals[j] == i)
                break;
        }
        u4 name_offset = rva_to_offset(pe, export->names[j]);
        printf("\t\t%08x %08x %08x %s\n",
               func,
               export->names[j],
               export->name_ordinals[j],
               &pe->bin->buffer[name_offset]);

    }
}

static void parse_export_table(pe_file *pe)
{
    pe_optional_header *header = pe->optional_header;
    pe_data_directory *export_dir = &header->data_directory[0];
    if (export_dir->size == 0)
        return;

    set_pe_off_of_rva(pe, export_dir->virtual_address);

    pe_export_directory *export = make_obj_in(pe_export_directory, pe->pool);
    jpe_read(pe, export, sizeof(u4)*10);

    export->functions = x_alloc_in(pe->pool, export->number_of_functions*sizeof(u4));
    export->names = x_alloc_in(pe->pool, export->number_of_names*sizeof(u4));
    export->name_ordinals = x_alloc_in(pe->pool, export->number_of_names*sizeof(u2));

    set_pe_off_of_rva(pe, export->address_of_functions);
    jpe_read(pe, export->functions, export->number_of_functions*sizeof(u4));

    set_pe_off_of_rva(pe, export->address_of_names);
    jpe_read(pe, export->names, export->number_of_names*sizeof(u4));

    set_pe_off_of_rva(pe, export->address_of_name_ordinals);
    jpe_read(pe, export->name_ordinals, export->number_of_names*sizeof(u2));

    pe->export_table = export;
#if 0
    write_export_table(pe);
#endif
}

static void parse_import_table(pe_file *pe)
{
    pe_optional_header *header = pe->optional_header;
    pe_data_directory *import_dir = &header->data_directory[1];
    if (import_dir->size == 0)
        return;

    set_pe_off_of_rva(pe, import_dir->virtual_address);

    pe_import_directory *import = make_obj_in(pe_import_directory, pe->pool);
    jpe_read(pe, import, sizeof(u4)*5);

    while (import->name) {
        pe->import_table_size++;
        jpe_read(pe, import, sizeof(u4)*5);
    }

    set_pe_off_of_rva(pe, import_dir->virtual_address);
    pe->import_table = make_obj_arr_in(pe_import_directory,
                    pe->import_table_size, pe->pool);

    for (int i = 0; i < pe->import_table_size; ++i) {
        pe_import_directory *imp = &pe->import_table[i];
        jpe_read(pe, imp, sizeof(u4)*5);
    }


    for (int i = 0; i < pe->import_table_size; ++i) {
        pe_import_directory *imp = &pe->import_table[i];
        pe_import_trunk_data *data = make_obj_in(pe_import_trunk_data, pe->pool);
        u2 trunk_size = 0;
        set_pe_off_of_rva(pe, imp->original_first_thunk);
        read_import_trunk_data(pe, data);

        while (data->u1.ordinal) {
            trunk_size++;
            read_import_trunk_data(pe, data);
        }

        imp->trunk_size = trunk_size;
        imp->trunks = make_obj_arr_in(pe_import_trunk_data,
                                      trunk_size, pe->pool);
        set_pe_off_of_rva(pe, imp->original_first_thunk);
        for (int j = 0; j < trunk_size; ++j) {
            pe_import_trunk_data *t = &imp->trunks[j];
            read_import_trunk_data(pe, t);
        }

        for (int j = 0; j < trunk_size; ++j) {
            pe_import_trunk_data *t = &imp->trunks[j];
            if (trunk_data_no_name(pe, t)) {
                t->name = make_obj_in(pe_import_by_name, pe->pool);
                t->name->hint = (t->u1.ordinal & 0xFFFFu);
                t->name->name = NULL;
            }
            else {
                set_pe_off_of_rva(pe, t->u1.function);
                t->name = make_obj_in(pe_import_by_name, pe->pool);
                jpe_read2(pe, &t->name->hint);
                t->name->name = pe->bin->buffer + pe->bin->cur_off;
            }
        }
    }

#if 0
    write_import_table(pe);
#endif
}

/**
 * start for dotnet metadata parser
 **/

static void parse_clr20_data(pe_file *pe)
{
    pe_optional_header *header = pe->optional_header;
    pe_data_directory *clr_dir = &header->data_directory[14];
    if (clr_dir->size == 0)
        return;

    set_pe_off_of_rva(pe, clr_dir->virtual_address);
    printf("CLR20 DATA\n");

    pe_cor20_header *clr = make_obj_in(pe_cor20_header, pe->pool);
    jpe_read4(pe, &clr->cb);
    jpe_read2(pe, &clr->major_runtime_version);
    jpe_read2(pe, &clr->minor_runtime_version);
    jpe_read(pe, &clr->meta_data, sizeof(pe_data_directory));
    jpe_read4(pe, &clr->flags);
    jpe_read4(pe, &clr->eu.entry_point_rva);
    jpe_read(pe, &clr->resources, sizeof(pe_data_directory));
    jpe_read(pe, &clr->strong_name_signature, sizeof(pe_data_directory));
    jpe_read(pe, &clr->code_manager_table, sizeof(pe_data_directory));
    jpe_read(pe, &clr->vtable_fixups, sizeof(pe_data_directory));
    jpe_read(pe, &clr->export_address_table_jumps, sizeof(pe_data_directory));
    jpe_read(pe, &clr->managed_native_header, sizeof(pe_data_directory));


    printf("\t%-25s: %08x\n", "[cb]", clr->cb);
    printf("\t%-25s: %04x\n", "[major_runtime_version]", clr->major_runtime_version);
    printf("\t%-25s: %04x\n", "[minor_runtime_version]", clr->minor_runtime_version);
    printf("\t%-25s: %08x\n", "[meta_data]", clr->meta_data.virtual_address);
    printf("\t%-25s: %08x\n", "[flags]", clr->flags);
    printf("\t%-25s: %08x\n", "[entry_point]", clr->eu.entry_point_rva);
    printf("\t%-25s: %08x\n", "[resources]", clr->resources.virtual_address);
    printf("\t%-25s: %08x\n", "[strong_name_signature]", clr->strong_name_signature.virtual_address);
    printf("\t%-25s: %08x\n", "[code_manager_table]", clr->code_manager_table.virtual_address);
    printf("\t%-25s: %08x\n", "[vtable_fixups]", clr->vtable_fixups.virtual_address);
    printf("\t%-25s: %08x\n", "[export_address_table_jumps]", clr->export_address_table_jumps.virtual_address);
    pe->clr = clr;
}

static void parse_pe_metadata(pe_file *pe)
{
    if (pe->clr == NULL)
        return;
    pe_data_directory *meta_dir = &pe->clr->meta_data;

    set_pe_off_of_rva(pe, meta_dir->virtual_address);
    pe_metadata_header *meta_header = make_obj_in(pe_metadata_header, pe->pool);

    jpe_read4(pe, &meta_header->signature);
    jpe_read2(pe, &meta_header->major_version);
    jpe_read2(pe, &meta_header->minor_version);
    jpe_read4(pe, &meta_header->reserved);
    jpe_read4(pe, &meta_header->version_length);
    meta_header->version = x_alloc_in(pe->pool, meta_header->version_length);
    jpe_read(pe, meta_header->version, meta_header->version_length);
    jpe_read2(pe, &meta_header->flags);
    jpe_read2(pe, &meta_header->streams_count);
    pe->metadata_header = meta_header;


    printf("METADATA HEADER\n");
    printf("\t%-25s: %08x\n", "[signature]", meta_header->signature);
    printf("\t%-25s: %04x\n", "[major_version]", meta_header->major_version);
    printf("\t%-25s: %04x\n", "[minor_version]", meta_header->minor_version);
    printf("\t%-25s: %08x\n", "[reserved]", meta_header->reserved);
    printf("\t%-25s: %08x\n", "[version_length]", meta_header->version_length);
    printf("\t%-25s: %s\n", "[version]", meta_header->version);
    printf("\t%-25s: %04x\n", "[flags]", meta_header->flags);
    printf("\t%-25s: %04x\n", "[streams_count]", meta_header->streams_count);

    pe_metadata_stream_header *stream_headers = make_obj_arr_in(pe_metadata_stream_header,
                    meta_header->streams_count, pe->pool);
    for (int i = 0; i < meta_header->streams_count; ++i) {
        pe_metadata_stream_header *h = &stream_headers[i];
        jpe_read4(pe, &h->offset);
        jpe_read4(pe, &h->size);
        string name = &pe->bin->buffer[pe->bin->cur_off];
        size_t len = strlen(name)+1;
        h->name = x_alloc_in(pe->pool, len);
        jpe_read(pe, h->name, len);
        if (len%4 != 0)
            pe->bin->cur_off += 4-(len)%4;
        printf("\t\t[%2d] %08x %08x %s\n", i, h->offset, h->size, h->name);
    }

    meta_header->stream_headers = stream_headers;
}

static pe_metadata_stream_header* get_stream_header(pe_file *pe, string name)
{
    for (int i = 0; i < pe->metadata_header->streams_count; ++i) {
        pe_metadata_stream_header *h = &pe->metadata_header->stream_headers[i];
        if (strcmp(h->name, name) == 0)
            return h;
    }
    return NULL;
}

static pe_metadata_table* get_clr_table(pe_file *pe, u8 mask)
{
    for (int i = 0; i < pe->meta_tables_size; ++i) {
        pe_metadata_table *t = &pe->meta_tables[i];
        if (t->table_name_mask == mask)
            return t;
    }
    return NULL;
}

static u4 clr_table_size(pe_file *pe, u8 mask)
{
    for (int i = 0; i < pe->meta_tables_size; ++i) {
        pe_metadata_table *t = &pe->meta_tables[i];
        if (t->table_name_mask == mask)
            return t->size;
    }
    return 0;
}

static inline int clr_table_idx_width(pe_file *pe, u8 mask)
{
    u4 size = clr_table_size(pe, mask);
    return size > 0xFFFF ? 4 : 2;
}

static void parse_sharp_tables_header(pe_file *pe)
{
    pe_metadata_stream_header *header = get_stream_header(pe, "#~");
    if (header == NULL)
        return;

    pe_data_directory *meta_dir = &pe->clr->meta_data;

    set_pe_off_of_rva(pe, header->offset + meta_dir->virtual_address);
    pe_metadata_tables_header *tables_header = make_obj_in(pe_metadata_tables_header, pe->pool);
    jpe_read4(pe, &tables_header->reserved);
    jpe_read1(pe, &tables_header->major_version);
    jpe_read1(pe, &tables_header->minor_version);
    jpe_read1(pe, &tables_header->heap_offset_sizes);
    jpe_read1(pe, &tables_header->reserved2);
    jpe_read8(pe, &tables_header->valid);
    jpe_read8(pe, &tables_header->sorted);
    pe->metadata_tables_header = tables_header;

    printf("TABLES HEADER\n");
    printf("\t%-25s: %08x\n", "[reserved]", tables_header->reserved);
    printf("\t%-25s: %02x\n", "[major_version]", tables_header->major_version);
    printf("\t%-25s: %02x\n", "[minor_version]", tables_header->minor_version);
    printf("\t%-25s: %02x\n", "[heap_offset_sizes]", tables_header->heap_offset_sizes);
    printf("\t%-25s: %02x\n", "[reserved2]", tables_header->reserved2);
    printf("\t%-25s: %08llx\n", "[valid]", tables_header->valid);

    printf("\t%-25s: %08llx\n", "[sorted]", tables_header->sorted);
    for (int i = 0; i < 45; ++i) {
        u8 mask_name = pe_meta_table_flags[i];
        if (mask_name & tables_header->sorted) {
            printf("\t\t%08llx\n", mask_name);
        }
    }

    u2 table_size = 0;
    for (int i = 0; i < 45; ++i) {
        u8 mask_name = pe_meta_table_flags[i];
        if (mask_name & tables_header->valid) {
            table_size++;
        }
    }
    pe->meta_tables_size = table_size;
    pe->meta_tables = make_obj_arr_in(pe_metadata_table, table_size, pe->pool);
    int idx = 0;
    for (int i = 0; i < 45; ++i) {
        u8 mask_name = pe_meta_table_flags[i];
        if (mask_name & tables_header->valid) {
            pe_metadata_table *t = &pe->meta_tables[idx];
            t->table_name_mask = mask_name;
            jpe_read4(pe, &t->size);
            printf("\t\t%08llx size: %d\n", mask_name, t->size);
            idx++;
        }
    }


}

static inline int meta_string_stream_width(pe_file *pe)
{
    pe_metadata_tables_header *tables_header = pe->metadata_tables_header;
    return (tables_header->heap_offset_sizes & 0x01) ? 4 : 2;
}

static inline int meta_guid_stream_width(pe_file *pe)
{
    pe_metadata_tables_header *tables_header = pe->metadata_tables_header;
    return (tables_header->heap_offset_sizes & 0x02) ? 4 : 2;
}

static inline int meta_blob_stream_width(pe_file *pe)
{
    pe_metadata_tables_header *tables_header = pe->metadata_tables_header;
    return (tables_header->heap_offset_sizes & 0x04) ? 4 : 2;
}

static u4 pe_table_max_rows(pe_file *pe, int count, ...) {
    va_list ap;
    int i;
    u4 biggest;
    u4 mask_name;
    u4 x;

    if (count == 0) {
        return 0;
    }

    va_start(ap, count);
    mask_name = va_arg(ap, u4);
    biggest = clr_table_size(pe, mask_name);

    for (i = 1; i < count; i++) {
        mask_name = va_arg(ap, u4);
        x = clr_table_size(pe, mask_name);
        biggest = (x > biggest) ? x : biggest;
    }

    va_end(ap);
    return biggest;
}

static int resolution_scope_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 4,
                               PE_METATABLE_MODULE,
                               PE_METATABLE_MODULE_REF,
                               PE_METATABLE_ASSEMBLY_REF,
                               PE_METATABLE_TYPE_REF);
    if (max >= (1 << (16 - 2)))
        return 4;
    return 2;
}

static int type_def_or_ref_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 3,
                               PE_METATABLE_TYPE_DEF,
                               PE_METATABLE_TYPE_REF,
                               PE_METATABLE_TYPE_SPEC);
    if (max >= (1 << (16 - 2)))
        return 4;
    return 2;
}

static int member_ref_parent_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 5,
                               PE_METATABLE_TYPE_DEF,
                               PE_METATABLE_TYPE_REF,
                               PE_METATABLE_MODULE_REF,
                               PE_METATABLE_METHOD,
                               PE_METATABLE_TYPE_SPEC);
    if (max >= (1 << (16 - 3)))
        return 4;
    return 2;
}

static int has_constant_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 3,
                               PE_METATABLE_FIELD,
                               PE_METATABLE_PARAM,
                               PE_METATABLE_PROPERTY);
    if (max >= (1 << (16 - 2)))
        return 4;
    return 2;
}

static int has_custom_attribute_width(pe_file *pe) {
    u4 max = pe_table_max_rows(pe, 19,
                               PE_METATABLE_METHOD,
                               PE_METATABLE_FIELD,
                               PE_METATABLE_TYPE_REF,
                               PE_METATABLE_TYPE_DEF,
                               PE_METATABLE_PARAM,
                               PE_METATABLE_INTERFACE_IMPL,
                               PE_METATABLE_MEMBER_REF,
                               PE_METATABLE_MODULE,
                               PE_METATABLE_DECL_SECURITY,
                               PE_METATABLE_PROPERTY,
                               PE_METATABLE_EVENT,
                               PE_METATABLE_STAND_ALONE_SIG,
                               PE_METATABLE_MODULE_REF,
                               PE_METATABLE_TYPE_SPEC,
                               PE_METATABLE_ASSEMBLY,
                               PE_METATABLE_ASSEMBLY_REF,
                               PE_METATABLE_FILE,
                               PE_METATABLE_EXPORTED_TYPE,
                               PE_METATABLE_MANIFEST_RESOURCE);
    if (max >= (1 << (16 - 5)))
        return 4;
    return 2;
}

static int custom_attribute_type_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 2,
                               PE_METATABLE_METHOD,
                               PE_METATABLE_MEMBER_REF);
    if (max >= (1 << (16 - 3)))
        return 4;
    return 2;
}

static int has_field_marshall_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 2,
                               PE_METATABLE_FIELD,
                               PE_METATABLE_PARAM);
    if (max >= (1 << (16 - 1)))
        return 4;
    return 2;
}

static int has_decl_security_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 3,
                               PE_METATABLE_TYPE_DEF,
                               PE_METATABLE_METHOD,
                               PE_METATABLE_ASSEMBLY);
    if (max >= (1 << (16 - 2)))
        return 4;
    return 2;
}

static int has_semantics_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 2,
                               PE_METATABLE_EVENT,
                               PE_METATABLE_PROPERTY);
    if (max >= (1 << (16 - 1)))
        return 4;
    return 2;
}

static int method_def_or_ref_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 2,
                               PE_METATABLE_METHOD,
                               PE_METATABLE_MEMBER_REF);
    if (max >= (1 << (16 - 1)))
        return 4;
    return 2;
}

static int member_forwarded_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 2,
                               PE_METATABLE_FIELD,
                               PE_METATABLE_METHOD);
    if (max >= (1 << (16 - 1)))
        return 4;
    return 2;
}

static int implementation_width(pe_file *pe)
{
    u4 max = pe_table_max_rows(pe, 3,
                               PE_METATABLE_FILE,
                               PE_METATABLE_ASSEMBLY_REF,
                               PE_METATABLE_EXPORTED_TYPE);
    if (max >= (1 << (16 - 2)))
        return 4;
    return 2;
}

static void parse_module_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, 0x00000001);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int guid_width = meta_guid_stream_width(pe);

    for (int i = 0; i < table->size; ++i) {
        pe_module_table *mt = make_obj_in(pe_module_table, pe->pool);
        jpe_read2(pe, &mt->generation);
        jpe_read(pe, &mt->name, string_width);
        jpe_read(pe, &mt->mvid, guid_width);
        jpe_read(pe, &mt->enc_id, guid_width);
        jpe_read(pe, &mt->enc_base_id, guid_width);
        printf("\t%08x %08x %08x %08x %08x\n",
               mt->generation,
               mt->name,
               mt->mvid,
               mt->enc_id,
               mt->enc_base_id);
    }
}

static void parse_type_ref_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_TYPE_REF);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    int size = resolution_scope_width(pe);
    printf("TYPE REF TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_type_ref_table *tt = make_obj_in(pe_type_ref_table, pe->pool);
        jpe_read(pe, &tt->resolution_scope, size);
        jpe_read(pe, &tt->name, string_width);
        jpe_read(pe, &tt->namespace, string_width);

        printf("\t%08x %08x %08x\n",
               tt->resolution_scope,
               tt->name,
               tt->namespace);
    }
}

static void parse_type_def_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_TYPE_DEF);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    printf("TYPE DEF TABLE\n");
    int type_def_or_ref = type_def_or_ref_width(pe);
    for (int i = 0; i < table->size; ++i) {
        pe_type_def_table *td = make_obj_in(pe_type_def_table, pe->pool);
        jpe_read4(pe, &td->flags);
        jpe_read(pe, &td->type_name, string_width);
        jpe_read(pe, &td->type_namespace, string_width);
        jpe_read(pe, &td->extends, type_def_or_ref);
        jpe_read(pe, &td->field_list,
                 clr_table_idx_width(pe, PE_METATABLE_FIELD));
        jpe_read(pe, &td->method_list,
                 clr_table_idx_width(pe, PE_METATABLE_METHOD));
        printf("\tflags: %08x class_name: %08x namespace: %08x extends: %08x field_list: %08x method_list: %08x\n",
               td->flags,
               td->type_name,
               td->type_namespace,
               td->extends,
               td->field_list,
               td->method_list);
    }
}

static void parse_field_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_FIELD);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    printf("FIELD TABLE\n");
    int type_def_or_ref = type_def_or_ref_width(pe);
    for (int i = 0; i < table->size; ++i) {
        pe_field_table *ft = make_obj_in(pe_field_table, pe->pool);
        jpe_read2(pe, &ft->flags);
        jpe_read(pe, &ft->name, string_width);
        jpe_read(pe, &ft->signature, meta_blob_stream_width(pe));
        printf("\tflags: %04x class_name: %08x signature: %08x\n",
               ft->flags,
               ft->name,
               ft->signature);
    }
}

static void parse_method_def_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_METHOD);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int param_width = clr_table_idx_width(pe, PE_METATABLE_PARAM);
    int blob_width = meta_blob_stream_width(pe);

    printf("METHOD DEF TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_method_def_table *md = make_obj_in(pe_method_def_table, pe->pool);
        jpe_read4(pe, &md->rva);
        jpe_read2(pe, &md->impl_flags);
        jpe_read(pe, &md->name, string_width);
        jpe_read(pe, &md->signature, blob_width);
        jpe_read(pe, &md->param_list,param_width);
        printf("\t%08x %04x %04x %08x %08x\n",
               md->rva,
               md->impl_flags,
               md->name,
               md->signature,
               md->param_list);
    }
}

static void parse_param_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_PARAM);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    printf("PARAM TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_param_table *pt = make_obj_in(pe_param_table, pe->pool);
        jpe_read2(pe, &pt->flags);
        jpe_read2(pe, &pt->sequence);
        jpe_read(pe, &pt->name, string_width);
        printf("\t%04x %04x %08x\n",
               pt->flags,
               pt->sequence,
               pt->name);
    }
}

static void parse_interface_impl_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_INTERFACE_IMPL);
    if (table == NULL)
        return;

    printf("INTERFACE IMPL TABLE\n");
    int type_def_or_ref = type_def_or_ref_width(pe);
    int typedef_width = clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF);
    for (int i = 0; i < table->size; ++i) {
        pe_interface_impl_table *ii = make_obj_in(pe_interface_impl_table, pe->pool);
        jpe_read(pe, &ii->class, typedef_width);
        jpe_read(pe, &ii->interface, type_def_or_ref);
        printf("\t%08x %08x\n",
               ii->class,
               ii->interface);
    }
}

static void parse_member_ref_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_MEMBER_REF);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int parent_width = member_ref_parent_width(pe);
    int blob_width = meta_blob_stream_width(pe);

    printf("MEMBER REF TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_member_ref_table *mr = make_obj_in(pe_member_ref_table, pe->pool);
        jpe_read(pe, &mr->klass, parent_width);
        jpe_read(pe, &mr->name, string_width);
        jpe_read(pe, &mr->signature, blob_width);
        printf("\t%08x %08x %08x\n",
               mr->klass,
               mr->name,
               mr->signature);
    }
}

static void parse_constant_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_CONSTANT);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);
    int has_constant = has_constant_width(pe);

    printf("CONSTANT TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_constant_table *ct = make_obj_in(pe_constant_table, pe->pool);
        jpe_read1(pe, &ct->type);
        jpe_read1(pe, &ct->padding);
        jpe_read(pe, &ct->parent, has_constant);
        jpe_read(pe, &ct->value, blob_width);
        printf("\t%02x %02x %08x %08x\n",
               ct->type,
               ct->padding,
               ct->parent,
               ct->value);
    }
}

static void parse_custom_attribute_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_CUSTOM_ATTRIBUTE);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);
    int has_custom_attribute = has_custom_attribute_width(pe);
    int custom_attribute_type = custom_attribute_type_width(pe);

    printf("CUSTOM ATTRIBUTE TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_custom_attribute_table *ct = make_obj_in(pe_custom_attribute_table, pe->pool);
        jpe_read(pe, &ct->parent, has_custom_attribute);
        jpe_read(pe, &ct->type, custom_attribute_type);
        jpe_read(pe, &ct->value, blob_width);
        printf("\t%08x %08x %08x\n",
               ct->parent,
               ct->type,
               ct->value);
    }
}

static void parse_field_marshal_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_FIELD_MARSHAL);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);
    int has_field_marshall = has_field_marshall_width(pe);

    printf("FIELD MARSHAL TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_field_marshal_table *fm = make_obj_in(pe_field_marshal_table, pe->pool);
        jpe_read(pe, &fm->parent, has_field_marshall);
        jpe_read(pe, &fm->native_type, blob_width);
        printf("\t%08x %08x\n",
               fm->parent,
               fm->native_type);
    }
}

static void parse_decl_security_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_DECL_SECURITY);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);
    int has_decl_security = has_decl_security_width(pe);

    printf("DECL SECURITY TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_decl_security_table *ds = make_obj_in(pe_decl_security_table, pe->pool);
        jpe_read2(pe, &ds->action);
        jpe_read(pe, &ds->parent, has_decl_security);
        jpe_read(pe, &ds->permission_set, blob_width);
        printf("\t%04x %08x %08x\n",
               ds->action,
               ds->parent,
               ds->permission_set);
    }
}

static void parse_class_layout_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_CLASS_LAYOUT);
    if (table == NULL)
        return;

    printf("CLASS LAYOUT TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_class_layout_table *cl = make_obj_in(pe_class_layout_table, pe->pool);
        jpe_read2(pe, &cl->packing_size);
        jpe_read4(pe, &cl->class_size);
        jpe_read(pe, &cl->parent, clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF));
        printf("\t%04x %08x %08x\n",
               cl->packing_size,
               cl->class_size,
               cl->parent);
    }
}

static void parse_field_layout_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_FIELD_LAYOUT);
    if (table == NULL)
        return;

    printf("FIELD LAYOUT TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_field_layout_table *fl = make_obj_in(pe_field_layout_table, pe->pool);
        jpe_read4(pe, &fl->offset);
        jpe_read(pe, &fl->field, clr_table_idx_width(pe, PE_METATABLE_FIELD));
        printf("\t%08x %08x\n",
               fl->offset,
               fl->field);
    }
}

static void parse_stand_alone_sig_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_STAND_ALONE_SIG);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);

    printf("STAND ALONE SIG TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_stand_alone_sig_table *sa = make_obj_in(pe_stand_alone_sig_table, pe->pool);
        jpe_read(pe, &sa->signature, blob_width);
        printf("\t%08x\n",
               sa->signature);
    }
}

static void parse_event_map_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_EVENT_MAP);
    if (table == NULL)
        return;

    printf("EVENT MAP TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_event_map_table *em = make_obj_in(pe_event_map_table, pe->pool);
        jpe_read(pe, &em->parent, clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF));
        jpe_read(pe, &em->event_list, clr_table_idx_width(pe, PE_METATABLE_EVENT));
        printf("\t%08x %08x\n",
               em->parent,
               em->event_list);
    }
}

static void parse_event_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_EVENT);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int type_def_or_ref = type_def_or_ref_width(pe);

    printf("EVENT TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_event_table *et = make_obj_in(pe_event_table, pe->pool);
        jpe_read2(pe, &et->event_flags);
        jpe_read(pe, &et->name, string_width);
        jpe_read(pe, &et->event_type, type_def_or_ref);
        printf("\t%04x %08x %08x\n",
               et->event_flags,
               et->name,
               et->event_type);
    }
}

static void parse_property_map_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_PROPERTY_MAP);
    if (table == NULL)
        return;

    printf("PROPERTY MAP TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_property_map_table *pm = make_obj_in(pe_property_map_table, pe->pool);
        jpe_read(pe, &pm->parent, clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF));
        jpe_read(pe, &pm->property_list, clr_table_idx_width(pe, PE_METATABLE_PROPERTY));
        printf("\t%08x %08x\n",
               pm->parent,
               pm->property_list);
    }
}

static void parse_property_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_PROPERTY);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int blob_width = meta_blob_stream_width(pe);

    printf("PROPERTY TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_property_table *pt = make_obj_in(pe_property_table, pe->pool);
        jpe_read2(pe, &pt->flags);
        jpe_read(pe, &pt->name, string_width);
        jpe_read(pe, &pt->type, blob_width);
        printf("\t%04x %08x %08x\n",
               pt->flags,
               pt->name,
               pt->type);
    }
}

static void parse_method_semantics_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_METHOD_SEMANTICS);
    if (table == NULL)
        return;

    printf("METHOD SEMANTICS TABLE\n");
    int has_semantics = has_semantics_width(pe);
    for (int i = 0; i < table->size; ++i) {
        pe_method_semantics_table *ms = make_obj_in(pe_method_semantics_table, pe->pool);
        jpe_read2(pe, &ms->semantics);
        jpe_read(pe, &ms->method, clr_table_idx_width(pe, PE_METATABLE_METHOD));
        jpe_read(pe, &ms->association, has_semantics);
        printf("\t%04x %08x %08x\n",
               ms->semantics,
               ms->method,
               ms->association);
    }
}

static void parse_method_impl_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_METHOD_IMPL);
    if (table == NULL)
        return;

    printf("METHOD IMPL TABLE\n");
    int method_def_or_ref = method_def_or_ref_width(pe);
    for (int i = 0; i < table->size; ++i) {
        pe_method_impl_table *mi = make_obj_in(pe_method_impl_table, pe->pool);
        jpe_read(pe, &mi->klass, clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF));
        jpe_read(pe, &mi->method_body, method_def_or_ref);
        jpe_read(pe, &mi->method_declaration, method_def_or_ref);
        printf("\t%08x %08x %08x\n",
               mi->klass,
               mi->method_body,
               mi->method_declaration);
    }
}

static void parse_module_ref_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_MODULE_REF);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    printf("MODULE REF TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_module_ref_table *mr = make_obj_in(pe_module_ref_table, pe->pool);
        jpe_read(pe, &mr->name, string_width);
        printf("\t%08x\n",
               mr->name);
    }
}

static void parse_type_spec_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_TYPE_SPEC);
    if (table == NULL)
        return;

    int blob_width = meta_blob_stream_width(pe);

    printf("TYPE SPEC TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_type_spec_table *ts = make_obj_in(pe_type_spec_table, pe->pool);
        jpe_read(pe, &ts->signature, blob_width);
        printf("\t%08x\n",
               ts->signature);
    }
}

static void parse_impl_map_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_IMPL_MAP);
    if (table == NULL)
        return;

    printf("IMPL MAP TABLE\n");
    int string_width = meta_string_stream_width(pe);
    int member_forwarded = member_forwarded_width(pe);
    for (int i = 0; i < table->size; ++i) {
        pe_impl_map_table *im = make_obj_in(pe_impl_map_table, pe->pool);
        jpe_read2(pe, &im->mapping_flags);
        jpe_read(pe, &im->member_forwarded, member_forwarded);
        jpe_read(pe, &im->import_name, string_width);
        jpe_read(pe, &im->import_scope, clr_table_idx_width(pe, PE_METATABLE_MODULE_REF));
        printf("\t%04x %08x %08x %08x\n",
               im->mapping_flags,
               im->member_forwarded,
               im->import_name,
               im->import_scope);
    }
}

static void parse_field_rva_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_FIELD_RVA);
    if (table == NULL)
        return;

    printf("FIELD RVA TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_field_rva_table *fr = make_obj_in(pe_field_rva_table, pe->pool);
        jpe_read4(pe, &fr->rva);
        jpe_read(pe, &fr->field, clr_table_idx_width(pe, PE_METATABLE_FIELD));
        printf("\t%08x %08x\n",
               fr->rva,
               fr->field);
    }
}

static void parse_assembly_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int blob_width = meta_blob_stream_width(pe);

    printf("ASSEMBLY TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_table *at = make_obj_in(pe_assembly_table, pe->pool);
        jpe_read4(pe, &at->hash_alg_id);
        jpe_read2(pe, &at->major_version);
        jpe_read2(pe, &at->minor_version);
        jpe_read2(pe, &at->build_number);
        jpe_read2(pe, &at->revision_number);
        jpe_read4(pe, &at->flags);
        jpe_read(pe, &at->public_key, blob_width);
        jpe_read(pe, &at->name, string_width);
        jpe_read(pe, &at->culture, string_width);
        printf("\t%08x %04x %04x %04x %04x %08x %08x %08x %08x\n",
               at->hash_alg_id,
               at->major_version,
               at->minor_version,
               at->build_number,
               at->revision_number,
               at->flags,
               at->public_key,
               at->name,
               at->culture);
    }
}

static void parse_assembly_processor_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY_PROCESSOR);
    if (table == NULL)
        return;

    printf("ASSEMBLY PROCESSOR TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_processor_table *ap = make_obj_in(pe_assembly_processor_table, pe->pool);
        jpe_read4(pe, &ap->processor);
        printf("\t%08x\n",
               ap->processor);
    }
}

static void parse_assembly_os_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY_OS);
    if (table == NULL)
        return;

    printf("ASSEMBLY OS TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_os_table *aos = make_obj_in(pe_assembly_os_table, pe->pool);
        jpe_read4(pe, &aos->os_platform_id);
        jpe_read4(pe, &aos->os_major_version);
        jpe_read4(pe, &aos->os_minor_version);
        printf("\t%08x %08x %08x\n",
               aos->os_platform_id,
               aos->os_major_version,
               aos->os_minor_version);
    }
}

static void parse_assembly_ref_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY_REF);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int blob_width = meta_blob_stream_width(pe);

    printf("ASSEMBLY REF TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_ref_table *ar = make_obj_in(pe_assembly_ref_table, pe->pool);
        jpe_read4(pe, &ar->major_version);
        jpe_read4(pe, &ar->minor_version);
        jpe_read4(pe, &ar->build_number);
        jpe_read4(pe, &ar->revision_number);
        jpe_read4(pe, &ar->flags);
        jpe_read(pe, &ar->public_key_or_token, blob_width);
        jpe_read(pe, &ar->name, string_width);
        jpe_read(pe, &ar->culture, string_width);
        jpe_read(pe, &ar->hash_value, blob_width);
        printf("\t%08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
               ar->major_version,
               ar->minor_version,
               ar->build_number,
               ar->revision_number,
               ar->flags,
               ar->public_key_or_token,
               ar->name,
               ar->culture,
               ar->hash_value);
    }
}

static void parse_assembly_ref_processor_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY_REF_PROCESSOR);
    if (table == NULL)
        return;

    printf("ASSEMBLY REF PROCESSOR TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_ref_processor_table *arp = make_obj_in(pe_assembly_ref_processor_table, pe->pool);
        jpe_read4(pe, &arp->processor);
        jpe_read(pe, &arp->assembly_ref, clr_table_idx_width(pe, PE_METATABLE_ASSEMBLY_REF));
        printf("\t%08x %08x\n",
               arp->processor,
               arp->assembly_ref);
    }
}

static void parse_assembly_ref_os_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_ASSEMBLY_REF_OS);
    if (table == NULL)
        return;

    printf("ASSEMBLY REF OS TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_assembly_ref_os_table *aros = make_obj_in(pe_assembly_ref_os_table, pe->pool);
        jpe_read4(pe, &aros->os_platform_id);
        jpe_read4(pe, &aros->os_major_version);
        jpe_read4(pe, &aros->os_minor_version);
        jpe_read(pe, &aros->assembly_ref, clr_table_idx_width(pe, PE_METATABLE_ASSEMBLY_REF));
        printf("\t%08x %08x %08x %08x\n",
               aros->os_platform_id,
               aros->os_major_version,
               aros->os_minor_version,
               aros->assembly_ref);
    }
}

static void parse_file_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_FILE);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int blob_width = meta_blob_stream_width(pe);

    printf("FILE TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_file_table *ft = make_obj_in(pe_file_table, pe->pool);
        jpe_read4(pe, &ft->flags);
        jpe_read(pe, &ft->name, string_width);
        jpe_read(pe, &ft->hash_value, blob_width);
        printf("\t%08x %08x %08x\n",
               ft->flags,
               ft->name,
               ft->hash_value);
    }
}

static void parse_exported_type_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_EXPORTED_TYPE);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int impl_width = implementation_width(pe);

    printf("EXPORTED TYPE TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_exported_type_table *et = make_obj_in(pe_exported_type_table, pe->pool);
        jpe_read4(pe, &et->flags);
        jpe_read4(pe, &et->type_def_id);
        jpe_read(pe, &et->type_name, string_width);
        jpe_read(pe, &et->type_namespace, string_width);
        jpe_read(pe, &et->implementation, impl_width);
        printf("\t%08x %08x %08x %08x %08x\n",
               et->flags,
               et->type_def_id,
               et->type_name,
               et->type_namespace,
               et->implementation);
    }
}

static void parse_manifest_resource_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_MANIFEST_RESOURCE);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);
    int impl_width = implementation_width(pe);

    printf("MANIFEST RESOURCE TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_manifest_resource_table *mr = make_obj_in(pe_manifest_resource_table, pe->pool);
        jpe_read4(pe, &mr->offset);
        jpe_read4(pe, &mr->flags);
        jpe_read(pe, &mr->name, string_width);
        jpe_read(pe, &mr->implementation, impl_width);
        printf("\t%08x %08x %08x %08x\n",
               mr->offset,
               mr->flags,
               mr->name,
               mr->implementation);
    }
}

static void parse_nested_class_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_NESTED_CLASS);
    if (table == NULL)
        return;

    printf("NESTED CLASS TABLE\n");
    int type_def_width = clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF);
    for (int i = 0; i < table->size; ++i) {
        pe_nested_class_table *nc = make_obj_in(pe_nested_class_table, pe->pool);
        jpe_read(pe, &nc->nested_class, type_def_width);
        jpe_read(pe, &nc->enclosing_class, type_def_width);
        printf("\t%08x %08x\n",
               nc->nested_class,
               nc->enclosing_class);
    }
}

static void parse_generic_param_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_GENERIC_PARAM);
    if (table == NULL)
        return;

    int string_width = meta_string_stream_width(pe);

    // TODO: error for owner, need fix
    printf("GENERIC PARAM TABLE\n");
    for (int i = 0; i < table->size; ++i) {
        pe_generic_param_table *gp = make_obj_in(pe_generic_param_table, pe->pool);
        jpe_read2(pe, &gp->number);
        jpe_read2(pe, &gp->flags);
        jpe_read(pe, &gp->owner,
                 clr_table_idx_width(pe, PE_METATABLE_TYPE_DEF));
        jpe_read(pe, &gp->name, string_width);
        printf("\t%04x %04x %08x %08x\n",
               gp->number,
               gp->flags,
               gp->owner,
               gp->name);
    }
}

static void parse_generic_param_constraint_table_table(pe_file *pe)
{
    pe_metadata_table *table = get_clr_table(pe, PE_METATABLE_GENERIC_PARAM_CONSTRAINT);
    if (table == NULL)
        return;

    printf("GENERIC PARAM CONSTRAINT TABLE\n");
    int type_def_or_ref = type_def_or_ref_width(pe);
    int generic_param = clr_table_idx_width(pe, PE_METATABLE_GENERIC_PARAM);
    for (int i = 0; i < table->size; ++i) {
        pe_generic_param_constraint_table *gpc = make_obj_in(pe_generic_param_constraint_table, pe->pool);
        jpe_read(pe, &gpc->owner, generic_param);
        jpe_read(pe, &gpc->constraint, type_def_or_ref);
        printf("\t%08x %08x\n",
               gpc->owner,
               gpc->constraint);
    }
}

pe_file* init_pe_file(string path)
{
    mem_pool *pool = mem_create_pool();
    pe_file *pe = init_pe_content(pool, path);

    parse_dos_header(pe);

    parse_dos_stub(pe);

    parse_nt_header(pe);

    parse_nt_optional_header(pe);

    parse_section_header(pe);

    parse_export_table(pe);

    parse_import_table(pe);

    parse_clr20_data(pe);

    parse_pe_metadata(pe);

    parse_sharp_tables_header(pe);

    parse_module_table(pe);

    parse_type_ref_table(pe);

    parse_type_def_table(pe);

    parse_field_table(pe);

    parse_method_def_table(pe);

    parse_param_table(pe);

    parse_interface_impl_table(pe);

    parse_member_ref_table(pe);

    parse_constant_table(pe);

    parse_custom_attribute_table(pe);

    parse_field_marshal_table(pe);

    parse_decl_security_table(pe);

    parse_class_layout_table(pe);

    parse_field_layout_table(pe);

    parse_stand_alone_sig_table(pe);

    parse_event_map_table(pe);

    parse_event_table(pe);

    parse_property_map_table(pe);

    parse_property_table(pe);

    parse_method_semantics_table(pe);

    parse_method_impl_table(pe);

    parse_module_ref_table(pe);

    parse_type_spec_table(pe);

    parse_impl_map_table(pe);

    parse_field_rva_table(pe);

    parse_assembly_table(pe);

    parse_assembly_processor_table(pe);

    parse_assembly_os_table(pe);

    parse_assembly_ref_table(pe);

    parse_assembly_ref_processor_table(pe);

    parse_assembly_ref_os_table(pe);

    parse_file_table(pe);

    parse_exported_type_table(pe);

    parse_manifest_resource_table(pe);

    parse_nested_class_table(pe);

    parse_generic_param_table(pe);

    parse_generic_param_constraint_table_table(pe);

    return pe;
}
