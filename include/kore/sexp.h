//----------------------------------------------------------------------------------------------------------------------
// S-expression library
//----------------------------------------------------------------------------------------------------------------------
//
//  An S-expression is an atom or a sequence of atoms in a linked-list formed by using con-cells.  An atom is a
//  primitive value such as an integer, float, string or symbol.  A cons-cell is a container of two atoms, one called
//  the head, the other called the tail.  Cons-cells can construct linked lists by having the tail be another cons-cell.
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/kore.h>

//----------------------------------------------------------------------------------------------------------------------
// SexprContext
//
// A structure that manages the memory for the S-expression library.
//----------------------------------------------------------------------------------------------------------------------

typedef i64 SxAtom;

typedef struct  
{
    SxAtom  m_head;
    SxAtom  m_tail;
    i64     m_refCount;
}
SxCell;

typedef struct  
{
    StringTable     m_symbols;
    Pool(i64)       m_integers;
    Pool(f64)       m_floats;
    Pool(SxCell)    m_cells;
    Arena           m_scratch;
}
SxContext;

//----------------------------------------------------------------------------------------------------------------------
// Context
//----------------------------------------------------------------------------------------------------------------------

void sxContextInit(SxContext* ctx);
void sxContextDone(SxContext* ctx);

//----------------------------------------------------------------------------------------------------------------------
// Creation of primitives
//----------------------------------------------------------------------------------------------------------------------

SxAtom sxTaggedValue(i64 tag, i64 value);
SxAtom sxTaggedShiftedValue(i64 tag, i64 value);
SxAtom sxTaggedPointer(i64 tag, void* ptr);

SxAtom sxNull();
SxAtom sxInteger(i64 i);
SxAtom sxFloat(f64 f);
SxAtom sxCell(SxContext* ctx, SxAtom head, SxAtom tail);
SxAtom sxSymbol(SxContext* ctx, const char* symbol);
SxAtom sxString(const char* string);

void sxRelease(SxAtom atom);

//----------------------------------------------------------------------------------------------------------------------
// Querying of primitives
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Tags allow encoding the type of atom into the actual value.
//
//  TAG     TYPE        DESCRIPTION
//
//  0000    CELL        Normal address to a SxCell.  If 0, will be interpreted as null.
//  0001    SYMBOL      Address of symbol in symbols string table.
//  0010    STRING      Address of string in interned string table.
//  0011    
//  0100
//  0101
//  0110
//  0111
//  1000    INTEGER     60-bit integer
//  1001    FLOAT       1-bit sign, 11-bit exponent and 48-bit fraction (truncated)
//  1010
//  1011
//  1100
//  1101
//  1110
//  1111

#define K_MIN_SX_INT    ((i64)(u64)0xF800000000000000)
#define K_MAX_SX_INT    ((i64)(u64)0x07ffffffffffffff)
#define K_SX_TAGMASK    (0xf)
#define K_SX_REMOVETAG(x)   ((x) & ~0xf)
#define K_SX_GETTAG(x)      ((x) & 0xf)

typedef enum
{
    // Addresses
    SXT_CELL = 0,
    SXT_SYMBOL = 9,
    SXT_STRING = 10,

    // Values
    SXT_INTEGER = 8,
    SXT_FLOAT,
}
SxType;

SxType sxGetType(SxAtom atom);
bool sxIsNull(SxAtom atom);
bool sxIsInteger(SxAtom atom);
bool sxIsFloat(SxAtom atom);
bool sxIsCell(SxAtom atom);
bool sxIsSymbol(SxAtom atom);
bool sxIsString(SxAtom atom);

//----------------------------------------------------------------------------------------------------------------------
// Conversion of primitives
// Only use sxGet??? functions if sxIs??? are true, otherwise an assertion will fail.
//----------------------------------------------------------------------------------------------------------------------

i64 sxGetInteger(SxAtom atom);
f64 sxGetFloat(SxAtom atom);
const char* sxGetSymbol(SxAtom atom);
const char* sxGetString(SxAtom atom);

//----------------------------------------------------------------------------------------------------------------------
// Simple list creation functions
//----------------------------------------------------------------------------------------------------------------------

SxAtom sxList1(SxContext* ctx, SxAtom e1);
SxAtom sxList2(SxContext* ctx, SxAtom e1, SxAtom e2);
SxAtom sxList3(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3);
SxAtom sxList4(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3, SxAtom e4);
SxAtom sxList5(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3, SxAtom e4, SxAtom e5);
SxAtom sxList6(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3, SxAtom e4, SxAtom e5, SxAtom e6);
SxAtom sxList7(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3, SxAtom e4, SxAtom e5, SxAtom e6, SxAtom e7);
SxAtom sxList8(SxContext* ctx, SxAtom e1, SxAtom e2, SxAtom e3, SxAtom e4, SxAtom e5, SxAtom e6, SxAtom e7, SxAtom e8);

//----------------------------------------------------------------------------------------------------------------------
// List builder
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    SxContext*  m_ctx;
    SxAtom      m_list;
    SxAtom      m_tail;
}
SxBuilder;

void sxBuilderInit(SxBuilder* builder, SxContext* ctx);
void sxBuilderAdd(SxBuilder* builder, SxAtom atom);
SxAtom sxBuilderGet(SxBuilder* builder);

//----------------------------------------------------------------------------------------------------------------------
// Output
//----------------------------------------------------------------------------------------------------------------------

String sxToString(SxAtom atom);
String sxPrettyPrint(SxAtom atom, i64 lineLength);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

//----------------------------------------------------------------------------------------------------------------------
// Context management
//----------------------------------------------------------------------------------------------------------------------

void sxContextInit(SxContext* ctx)
{
    stringTableInit(&ctx->m_symbols, K_KB(16), 256);
    ctx->m_integers = 0;
    ctx->m_floats = 0;
    ctx->m_cells = 0;
    arenaInit(&ctx->m_scratch, K_KB(1));
}

void sxContextDone(SxContext* ctx)
{
    stringTableDone(&ctx->m_symbols);
    poolDone(ctx->m_integers);
    poolDone(ctx->m_floats);
    poolDone(ctx->m_cells);
    arenaDone(&ctx->m_scratch);
}

//----------------------------------------------------------------------------------------------------------------------
// Creation of primitives
//----------------------------------------------------------------------------------------------------------------------

SxAtom sxTaggedValue(i64 tag, i64 value)
{
    K_ASSERT(K_SX_GETTAG(value) == 0);
    K_ASSERT(tag >= 0 && tag < 16);
    return value | tag;
}

SxAtom sxTaggedShiftedValue(i64 tag, i64 value)
{
    K_ASSERT(value >= K_MIN_SX_INT && value <= K_MAX_SX_INT);
    K_ASSERT(tag >= 0 && tag < 16);
    return (value << 4) | tag;
}

SxAtom sxTaggedPointer(i64 tag, void* ptr)
{
    i64 p = K_BITCAST(i64, ptr);
    K_ASSERT(K_SX_GETTAG(p) == 0);
    return sxTaggedValue(tag, p);
}

SxAtom sxNull()
{
    return 0;
}

SxAtom sxInteger(i64 i)
{
    return sxTaggedShiftedValue(SXT_INTEGER, i);
}

SxAtom sxFloat(f64 f)
{
    return sxTaggedValue(SXT_FLOAT, K_SX_REMOVETAG(K_BITCAST(i64, f)));
}

//----------------------------------------------------------------------------------------------------------------------
// Querying of primitives
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Conversion of primitives
//----------------------------------------------------------------------------------------------------------------------

i64 sxGetInteger(SxAtom atom)
{
    return (i64)(atom >> 4);
}

f64 sxGetFloat(SxAtom atom)
{
    SxAtom a = K_SX_REMOVETAG(atom);
    return K_BITCAST(f64, a);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
