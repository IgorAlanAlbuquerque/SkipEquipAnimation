#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REL/Relocation.h"

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()
#define DEBUG_MODE
#define WORK_WITH_MAGIC_OBJECTS

using namespace std::literals;
