#ifndef DUMP_H
#define DUMP_H

#include "spu_commands.h"
#include "spu_facilities.h"

spu_error_t  write_code_dump      (spu_t    *spu);
spu_error_t  write_registers_dump (spu_t    *spu);
spu_error_t  write_ram_dump       (spu_t    *spu);

#endif
