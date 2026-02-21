#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REL/Relocation.h"

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()
#define DEBUG_MODE

using namespace std::literals;
