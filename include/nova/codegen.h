#pragma once

#include <stdbool.h>

#include "nova/ir.h"
#include "nova/semantic.h"

bool nova_codegen_emit_object(const NovaIRProgram *program, const NovaSemanticContext *semantics, const char *object_path, char *error_buffer, size_t error_buffer_size);

