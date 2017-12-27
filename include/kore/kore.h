//----------------------------------------------------------------------------------------------------------------------
// Kore C base API
// Copyright (C)2018 Matt Davies, all rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Platform detection
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#define YES (1)
#define NO (0)

// Compiler defines
#define K_COMPILER_MSVC     NO

// OS defines
#define K_OS_WIN32          NO

// CPU defines
#define K_CPU_X86           NO
#define K_CPU_X64           NO

// Configuration defines
#define K_DEBUG             NO
#define K_RELEASE           NO

// Endianess
#define K_LITTLE_ENDIAN     YES
#define K_BIG_ENDIAN        NO

//----------------------------------------------------------------------------------------------------------------------
// Compiler determination

#ifdef _MSC_VER
#   undef K_COMPILER_MSVC
#   define K_COMPILER_MSVC YES
#else
#   error Unknown compiler.  Please define COMPILE_XXX macro for your compiler.
#endif

//----------------------------------------------------------------------------------------------------------------------
// OS determination

#ifdef _WIN32
#   undef K_OS_WIN32
#   define K_OS_WIN32 YES
#else
#   error Unknown OS.  Please define OS_XXX macro for your operating system.
#endif

//----------------------------------------------------------------------------------------------------------------------
// CPU determination

#if K_COMPILER_MSVC
#   if defined(_M_X64)
#       undef K_CPU_X64
#       define K_CPU_X64 YES
#   elif defined(_M_IX86)
#       undef K_CPU_X86
#       define K_CPU_X86 YES
#   else
#       error Can not determine processor - something's gone very wrong here!
#   endif
#else
#   error Add CPU determination code for your compiler.
#endif

//----------------------------------------------------------------------------------------------------------------------
// Configuration

#ifdef _DEBUG
#   undef K_DEBUG
#   define K_DEBUG YES
#else
#   undef K_RELEASE
#   define K_RELEASE YES
#endif

//----------------------------------------------------------------------------------------------------------------------
// Standard headers

#if K_OS_WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <Windows.h>
#   ifdef _DEBUG
#       include <crtdbg.h>
#   endif
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32 && K_DEBUG
#   define K_BREAK() DebugBreak()
#else
#   define K_BREAK()
#endif

// Break on the nth allocation.
void debugBreakOnAlloc(int n);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Basic types and definitions
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

typedef char        bool;

#define K_BOOL(b) ((b) ? YES : NO)

#define K_KB(x) (1024 * (x))
#define K_MB(x) (1024 * K_KB(x))
#define K_GB(x) (1024 * K_MB(x))

#define K_MIN(a, b) ((a) < (b) ? (a) : (b))
#define K_MAX(a, b) ((a) < (b) ? (b) : (a))
#define K_ABS(a) ((a) < 0 ? -(a) : (a))

#define K_ASSERT(x, ...) assert(x)

#define internal static

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Memory API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifndef K_ARENA_INCREMENT
#   define K_ARENA_INCREMENT   4096
#endif

#ifndef K_ARENA_ALIGN
#   define K_ARENA_ALIGN       8
#endif

//----------------------------------------------------------------------------------------------------------------------
// Basic allocation

void* memoryAlloc(i64 numBytes, const char* file, int line);
void* memoryAllocClear(i64 numBytes, const char* file, int line);
void* memoryRealloc(void* address, i64 oldNumBytes, i64 newNumBytes, const char* file, int line);
void memoryFree(void* address, i64 numBytes, const char* file, int line);

void memoryCopy(const void* src, void* dst, i64 numBytes);
void memoryMove(const void* src, void* dst, i64 numBytes);
int memoryCompare(const void* mem1, const void* mem2, i64 numBytes);
void memoryClear(void* mem, i64 numBytes);

#define K_ALLOC(numBytes) memoryAlloc((numBytes), __FILE__, __LINE__)
#define K_ALLOC_CLEAR(numBytes) memoryAllocClear((numBytes), __FILE__, __LINE__)
#define K_REALLOC(address, oldNumBytes, newNumBytes) memoryRealloc((address), (oldNumBytes), (newNumBytes), __FILE__, __LINE__)
#define K_FREE(address, oldNumBytes) memoryFree((address), (oldNumBytes), __FILE__, __LINE__)

//----------------------------------------------------------------------------------------------------------------------
// Arena allocation

typedef struct
{
    u8*     start;
    u8*     end;
    i64     cursor;
    i64     restore;
}
Arena;

// Create a new Arena.
void arenaInit(Arena* arena, i64 initialSize);

// Deallocate the memory used by the arena.
void arenaDone(Arena* arena);

// Allocate some memory on the arena.
void* arenaAlloc(Arena* arena, i64 numytes);

// Ensure that the next allocation is aligned to 16 bytes boundary.
void* arenaAlign(Arena* arena);

// Combine alignment and allocation into one function for convenience.
void* arenaAlignedAlloc(Arena* arena, i64 numBytes);

// Create a restore point so that any future allocations can be deallocated in one go.
void arenaPush(Arena* arena);

// Deallocate memory from the previous restore point.
void arenaPop(Arena* arena);

#define K_ARENA_ALLOC(arena, t, count) (t *)arenaAlignedAlloc((arena), sizeof(t) * (count))

//----------------------------------------------------------------------------------------------------------------------
// Arrays

#define Array(T) T*

#define K_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

// Destroy an array
#define arrayDone(a) ((a) ? K_FREE((u8 *)a - (sizeof(i64) * 2), (sizeof(*a) * __arrayCapacity(a)) + (sizeof(i64) * 2)), 0 : 0)

// Add an element to the end of an array
#define arrayAdd(a, v) (__arrayMayGrow(a, 1), (a)[__arrayCount(a)++] = (v))

// Return the number of elements in an array
#define arrayCount(a) ((a) ? __arrayCount(a) : 0)

// Add n uninitialised elements to the array
#define arrayExpand(a, n) (__arrayMayGrow(a, n), __arrayCount(a) += (n), &(a)[__arrayCount(a) - (n)])

// Reserve capacity for n extra items to the array
#define arrayReserve(a, n) (__arrayMayGrow(a, n))

// Clear the array
#define arrayClear(a) (arrayCount(a) = 0)

// Delete an array entry
#define arrayDelete(a, i) (memoryMove(&(a)[(i)+1], &(a)[(i)], (__arrayCount(a) - (i) - 1) * sizeof(*a)), --__arrayCount(a), (a))

// Index of element
#define arrayIndexOf(a, e) (((e) - (a)) / sizeof(*(a)))

//
// Internal routines
//

#define __arrayRaw(a) ((i64 *)(a) - 2)
#define __arrayCount(a) __arrayRaw(a)[1]
#define __arrayCapacity(a) __arrayRaw(a)[0]

#define __arrayNeedsToGrow(a, n) ((a) == 0 || __arrayCount(a) + (n) > __arrayCapacity(a))
#define __arrayMayGrow(a, n) (__arrayNeedsToGrow(a, (n)) ? __arrayGrow(a, n) : 0)
#define __arrayGrow(a, n) ((a) = __arrayInternalGrow((a), (n), sizeof(*(a))))

internal void* __arrayInternalGrow(void* a, i64 increment, i64 elemSize);

//----------------------------------------------------------------------------------------------------------------------
// Pools

#define Pool(T) T*

// Destroy a pool
#define poolDone(p) arrayDone(p)

// Allocate an element from the pool - return index
#define poolAcquire(p) poolIndexOf((p), __poolInternalAcquire((p), (p) ? __poolCapacity(p) : 1, sizeof(*(p))))

// Release an element back to the pool
#define poolRecycle(p, i) __poolInternalRecycle((p), (i), sizeof(*(p)))

// Index of an element
#define poolIndexOf(p, e) ((i64)((u8 *)(e) - (u8 *)(p)) / sizeof(*(p)))

//
// Internal routines
//

#define __poolRaw(p) __arrayRaw(p)
#define __poolFreeList(p) __poolRaw(p)[1]
#define __poolCapacity(p) __poolRaw(p)[0]

internal void* __poolInternalAcquire(void* p, i64 increment, i64 elemSize);
internal void __poolInternalRecycle(void* p, i64 index, i64 elemSize);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Timer functions
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32
#   define TimePoint LARGE_INTEGER
#   define TimePeriod LARGE_INTEGER
#else
#   error Define Time for your platform!
#endif

// Return a time point representing
TimePoint timeNow();

// Return a time period representing the time between time points.
TimePeriod timePeriod(TimePoint a, TimePoint b);

// Advance a time point by a time period.
TimePoint timeFuture(TimePoint t, TimePeriod p);

// Convert a time in secs to a time period.
TimePeriod timeSecs(f64 time);

// Convert a time in msecs to a time period.
TimePeriod timeMsecs(i64 ms);

// Convert a time period into an integer millisecond count.
i64 timeToMSecs(TimePeriod period);

// Convert a time period into seconds
f64 timeToSecs(TimePeriod period);

// Block for a time period
void timeWaitFor(TimePeriod time);

// Block until a time point
void timeWaitUntil(TimePoint time);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Basic files access
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    u8*     bytes;
    i64     size;

#if K_OS_WIN32
    HANDLE  file;
    HANDLE  fileMap;
#endif
} Data;

Data dataLoad(const char* fileName);
Data dataMake(const char* fileName, i64 size);
void dataUnload(Data data);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Strings
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef i8* String;

//----------------------------------------------------------------------------------------------------------------------
// Hashing
// Based on FNV-1a
//----------------------------------------------------------------------------------------------------------------------

u64 hash(const u8* buffer, i64 len);

//----------------------------------------------------------------------------------------------------------------------
// Dynamic strings
//----------------------------------------------------------------------------------------------------------------------

// Create a string.
String stringMake(const i8* str);

// Create a string of a set length.
String stringReserve(i64 len, i8 ch);

// Create a string from a byte range determined by a start and finish.
String stringMakeRange(const i8* start, const i8* end);

// Release a string back to memory.
void stringRelease(String str);

// Create a new string by appending it to another.
String stringAppend(String str1, String str2);
String stringAppendCStr(String str1, const i8* str2);

// Attributes of the string
i64 stringLength(String str);
u64 stringHash(String str);

// String formatting
String stringFormat(const i8* format, ...);
String stringFormatV(const i8* format, va_list args);

// String comparison
int stringCompare(String s1, const i8* s2);

// String comparison based on hashes.
bool stringEqual(String s1, String s2);

//----------------------------------------------------------------------------------------------------------------------
// Arena static strings
// These are strings that are allocated on an arena and are just used for construction.  No operations are intended
// to be run on them after creation.
//----------------------------------------------------------------------------------------------------------------------

// Constructs a static string on the arena.
String arenaStringCopy(Arena* arena, const i8* str);

// Constructs a static string on the arena with a range.
String arenaStringCopyRange(Arena* arena, const i8* str, const i8* end);

// Create a formatted string on the arena with va_list args.
String arenaStringFormatV(Arena* arena, const i8* format, va_list args);

// Create formatted string on the arena with variable arguments.
String arenaStringFormat(Arena* arena, const i8* format, ...);

//----------------------------------------------------------------------------------------------------------------------
// String tables
//
// The string table comprises of a header, a hash table, and an area for string storage.
//
// The header comprises of a single i64 which is the size of the hash table.
//
// Next the hash table is an array of N pointers to the beginning of a linked list.  THe index of the hash table is
// generated by the mod of the string's hash and the size of the table.
//
// The string storage is just string information appended one after the other.  For each string you have this info:
//
// Offset   Size    Description
//  0       8       Link to next string with the same hash index, indexed from beginning of string table
//  8       8       64-bit hash of string (not including null terminator)
//  16      ?       Actual string followed by a null terminator.
//
// Each string block is aligned in memory.  No string is allowed to have a hash of zero.  A string token of 0 is
// always a null string.
//----------------------------------------------------------------------------------------------------------------------

typedef Arena StringTable;
typedef i64 StringToken;

// Initialise a new string table
void stringTableInit(StringTable* table, i64 size, i64 sizeHashTableSize);

// Destroy the string table
void stringTableDone(StringTable* table);

// Add a string to the table and return a unique token representing it.  If it already exists, its token will be
// returned.
StringToken stringTableAdd(StringTable* table, const i8* str);

// Add a string view to the table.
StringToken stringTableAddRange(StringTable* table, const i8* str, const i8* end);

// Convert the token into a string in O(1) time.
String stringTableGet(StringTable* table, StringToken token);

//----------------------------------------------------------------------------------------------------------------------
// Path strings
//
// Given the path "c:\dir1\dir2\file.foo.ext"
//
//  Path        "c:\dir1\dir2\file.foo.ext"
//  Directory   "c:\dir1\dir2"
//  Filename    "file.foo.ext"
//  Extension   "ext"
//  Base        "file.foo"
//
//----------------------------------------------------------------------------------------------------------------------

// Return the directory of a path
String pathDirectory(String path);

// Return a new string with the extension removed
String pathRemoveExtension(String path);

// Return a new string with the extension of the path changed to a new one.
String pathReplaceExtension(String path, const char* ext);

// Join two paths together, the second must be a relative path.
String pathJoin(String p1, String p2);

//----------------------------------------------------------------------------------------------------------------------
// String utilities
//
// stringCompareStringRange     Compare s1..s2 with null terminated s3.
//----------------------------------------------------------------------------------------------------------------------

int stringCompareStringRange(const i8* s1, const i8* s2, const i8* s3);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Random number generation
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#define K_RANDOM_TABLE_SIZE     312

typedef struct
{
    u64     table[K_RANDOM_TABLE_SIZE];
    u64     index;
}
Random;

// Initialise based on time.
void randomInit(Random* R);

// Initialise with a single seed value.
void randomInitSeed(Random* R, u64 seed);

// Initialise with an array of seeds.
void randomInitArray(Random* R, u64* seeds, i64 len);

// Return a 64-bit pseudo-random number.
u64 random64(Random* R);

// Return a random float inclusive [0, 1].
f64 randomFloat(Random* R);

// Return a random float exclusive [0, 1).
f64 randomFloatNo1(Random* R);

// Return a random float really exclusive (0, 1).
f64 randomFloatNo0Or1(Random* R);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// CRC-32 API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Checksum calculation
u32 crc32(void* data, i64 len);
u32 crc32Update(u32 crc, void* data, i64 len);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// SHA-1 API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    u32     state[5];
    u32     count[2];
    u8      digest[20];
    u8      buffer[64];
    bool    finalised;
}
Sha1;

//
// Initialisation
//
void sha1Init(Sha1* s);

// The process functions will initialise, process and finalise in one step.
void sha1ProcessBuffer(Sha1* s, const void* data, i64 numBytes);
void sha1ProcessString(Sha1* s, String str);
void sha1ProcessHexString(Sha1* s, String hexStr);
void sha1ProcessData(Sha1* s, Data data);

//
// Updating & finalisation
//
void sha1Add(Sha1* s, const void* data, i64 numBytes);
void sha1Finalise(Sha1* s);

//
// Conversion
//
String sha1Hex(Sha1* s);
u64 sha1Hash64(Sha1* s);

//
// Testing
//
bool sha1Equal(Sha1* s1, Sha1* s2);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Process spawning API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Start a process
bool processStartAndWait(const i8* fileName, int argc, const i8** argv);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// PNG writing
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

bool pngWrite(const char* fileName, u32* img, int width, int height);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Simple pattern matching
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

bool match(const i8 *regexp, const i8* text);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Console
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

void consoleOpen();
void consolePause();
void consoleEnableANSIColours();

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------{INDEX}
// INDEX
//
//  ARRAY       Array management
//  CONSOLE     Console control
//  CRC32       CRC-32 checksumming
//  DATA        Data loading
//  ENTRY       Entry point
//  HASH        Fast hashing
//  MEMORY      Memory management
//  PATTERN     Pattern matching
//  PLATFORM    Platform-specific code
//  PNG         Simple uncompressed PNG output (for debugging)
//  POOL        Memory pools
//  RANDOM      Random number generation
//  SHA1        SHA-1 checksumming
//  SPAWN       Process spawning API
//  STRING      String processing, arena strings, paths and string tables
//  TIME        Time management
//
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

#include <time.h>

#if K_OS_WIN32
#   include <conio.h>
#   include <fcntl.h>
#   include <io.h>
#endif

//----------------------------------------------------------------------------------------------------------------------

void debugBreakOnAlloc(int n)
{
#ifdef _DEBUG
    _crtBreakAlloc = n;
#endif
}

//----------------------------------------------------------------------------------------------------------------------{TIME}
//----------------------------------------------------------------------------------------------------------------------
// Time Management
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------


// NOTES:
//  Windows uses QueryPerformanceCounter
//  POSIX uses clock_gettime
//  MacOSX uses mach_get_time
//

#if K_OS_WIN32

TimePoint timeNow()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t;
}

TimePeriod timePeriod(TimePoint a, TimePoint b)
{
    LARGE_INTEGER t;
    t.QuadPart = b.QuadPart - a.QuadPart;
    return t;
}

TimePoint timeFuture(TimePoint t, TimePeriod p)
{
    t.QuadPart += p.QuadPart;
    return t;
}

TimePeriod timeSecs(f64 time)
{
    LARGE_INTEGER t;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    t.QuadPart = (LONGLONG)(time * (f64)freq.QuadPart);
    return t;
}

TimePeriod timeMsecs(i64 ms)
{
    LARGE_INTEGER t;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    t.QuadPart = (LONGLONG)(ms * freq.QuadPart / 1000);
    return t;
}

i64 timeToMSecs(TimePeriod period)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return period.QuadPart * 1000 / freq.QuadPart;
}

f64 timeToSecs(TimePeriod period)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (f64)period.QuadPart / (f64)freq.QuadPart;
}

void timeWaitFor(TimePeriod time)
{
    TimePoint futureTime = timeFuture(timeNow(), time);
    timeWaitUntil(futureTime);
}

void timeWaitUntil(TimePoint time)
{
    TimePeriod t = { 0 };
    while ((t = timeNow()).QuadPart < time.QuadPart);
}

#else
#   error Implement for your platform
#endif

//----------------------------------------------------------------------------------------------------------------------{ENTRY}
//----------------------------------------------------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

extern int kmain(int argc, char** argv);

int main(int argc, char** argv)
{
    return kmain(argc, argv);
}

#if K_OS_WIN32
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow)
{
    int result = kmain(__argc, __argv);

#ifdef K_DEBUG
    if (_CrtDumpMemoryLeaks())
    {
        MessageBoxA(0, "Memory leaks found!", "ERROR", MB_ICONERROR | MB_OK);
    }
#endif

    return result;
}
#endif

//----------------------------------------------------------------------------------------------------------------------{MEMORY}
//----------------------------------------------------------------------------------------------------------------------
// Memory
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

internal void* memoryOp(void* oldAddress, i64 oldNumBytes, i64 newNumBytes, const char* file, int line)
{
    void* p = 0;

    if (newNumBytes)
    {
        p = realloc(oldAddress, newNumBytes);
    }
    else
    {
        free(oldAddress);
    }

    return p;
}

void* memoryAlloc(i64 numBytes, const char* file, int line)
{
    return memoryOp(0, 0, numBytes, file, line);
}

void* memoryAllocClear(i64 numBytes, const char* file, int line)
{
    void* p = memoryOp(0, 0, numBytes, file, line);
    memoryClear(p, numBytes);
    return p;
}

void* memoryRealloc(void* address, i64 oldNumBytes, i64 newNumBytes, const char* file, int line)
{
    return memoryOp(address, oldNumBytes, newNumBytes, file, line);
}

void memoryFree(void* address, i64 numBytes, const char* file, int line)
{
    memoryOp(address, numBytes, 0, file, line);
}

void memoryCopy(const void* src, void* dst, i64 numBytes)
{
    memcpy(dst, src, (size_t)numBytes);
}

void memoryMove(const void* src, void* dst, i64 numBytes)
{
    memmove(dst, src, (size_t)numBytes);
}

int memoryCompare(const void* mem1, const void* mem2, i64 numBytes)
{
    return memcmp(mem1, mem2, (size_t)numBytes);
}

void memoryClear(void* mem, i64 numBytes)
{
    memset(mem, 0, (size_t)numBytes);
}

//----------------------------------------------------------------------------------------------------------------------
// Arena control
//----------------------------------------------------------------------------------------------------------------------

void arenaInit(Arena* arena, i64 initialSize)
{
    u8* buffer = (u8 *)malloc(initialSize);
    if (buffer)
    {
        arena->start = buffer;
        arena->end = arena->start + initialSize;
        arena->cursor = 0;
        arena->restore = -1;
    }
}

void arenaDone(Arena* arena)
{
    free(arena->start);
    arena->start = 0;
    arena->end = 0;
    arena->cursor = 0;
    arena->restore = -1;
}

void* arenaAlloc(Arena* arena, i64 size)
{
    void* p = 0;
    if ((arena->start + arena->cursor + size) > arena->end)
    {
        // We don't have enough room
        i64 currentSize = (i64)(arena->end - (u8 *)arena->start);
        i64 requiredSize = currentSize + size;
        i64 newSize = currentSize + K_MAX(requiredSize, K_ARENA_INCREMENT);

        u8* newArena = (u8 *)realloc(arena->start, newSize);

        if (newArena)
        {
            arena->start = newArena;
            arena->end = newArena + newSize;

            // Try again!
            p = arenaAlloc(arena, size);
        }
    }
    else
    {
        p = arena->start + arena->cursor;
        arena->cursor += size;
    }

    return p;
}

void* arenaAlign(Arena* arena)
{
    i64 mod = arena->cursor % K_ARENA_ALIGN;
    void* p = arena->start + arena->cursor;

    if (mod)
    {
        // We need to align
        arenaAlloc(arena, K_ARENA_ALIGN - mod);
    }

    return p;
}

void* arenaAlignedAlloc(Arena* arena, i64 numBytes)
{
    arenaAlign(arena);
    return arenaAlloc(arena, numBytes);
}

void arenaPush(Arena* arena)
{
    arenaAlign(arena);
    {
        i64* p = arenaAlloc(arena, sizeof(i64) * 2);
        p[0] = 0xaaaaaaaaaaaaaaaa;
        p[1] = arena->restore;
        arena->restore = (i64)((u8 *)p - arena->start);
    }
}

void arenaPop(Arena* arena)
{
    i64* p = 0;
    K_ASSERT(arena->restore != -1, "Make sure we have some restore points left");
    arena->cursor = arena->restore;
    p = (i64 *)(arena->start + arena->cursor);
    p[0] = 0xbbbbbbbbbbbbbbbb;
    arena->restore = p[1];
}

//----------------------------------------------------------------------------------------------------------------------{ARRAY}
//----------------------------------------------------------------------------------------------------------------------
// Arrays
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

internal void* __arrayInternalGrow(void* a, i64 increment, i64 elemSize)
{
    i64 doubleCurrent = a ? 2 * __arrayCapacity(a) : 0;
    i64 minNeeded = arrayCount(a) + increment;
    i64 capacity = doubleCurrent > minNeeded ? doubleCurrent : minNeeded;
    i64 oldBytes = a ? elemSize * arrayCount(a) + sizeof(i64) * 2 : 0;
    i64 bytes = elemSize * capacity + sizeof(i64) * 2;
    i64* p = (i64 *)K_REALLOC(a ? __arrayRaw(a) : 0, oldBytes, bytes);
    if (p)
    {
        if (!a) p[1] = 0;
        p[0] = capacity;
        return p + 2;
    }
    else
    {
        return 0;
    }
}

//----------------------------------------------------------------------------------------------------------------------{POOL}
//----------------------------------------------------------------------------------------------------------------------
// Pools
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

internal void* __poolInternalAcquire(void* p, i64 increment, i64 elemSize)
{
    // Test for capacity
    // If this assert triggers, the element size is too small for a pool.
    K_ASSERT(elemSize >= sizeof(i64));
    if (!p || (__poolFreeList(p) == __poolCapacity(p)))
    {
        // We have no more room
        i64 oldCapacity = p ? __poolCapacity(p) : 0;
        i64 doubleCurrent = 2 * oldCapacity;
        i64 minNeeded = (p ? __poolCapacity(p) : 0) + 1;
        i64 capacity = doubleCurrent > minNeeded ? doubleCurrent : minNeeded;
        i64 oldBytes = p ? elemSize * __poolCapacity(p) + sizeof(i64) * 2 : 0;
        i64 bytes = elemSize * capacity + sizeof(i64) * 2;
        i64* newP = (i64 *)K_REALLOC(p ? __poolRaw(p) : 0, oldBytes, bytes);
        if (newP)
        {
            u8* b = (u8 *)newP + 2;
            if (!p) newP[0] = capacity;
            for (i64 i = oldCapacity; i < capacity; ++i)
            {
                *(i64 *)(&b[i * elemSize]) = i + 1;
            }
        }
        else
        {
            return 0;
        }

        p = newP + 2;
    }

    // Do the acquiring
    u8* b = (u8 *)p;
    i64 newIndex = __poolFreeList(p);
    i64* b64 = (i64 *)&b[newIndex * elemSize];
    __poolFreeList(p) = *b64;
    return b64;
}

internal void __poolInternalRecycle(void* p, i64 index, i64 elemSize)
{
    u8* b = (u8 *)p;
    i64* b64 = (i64 *)&b[index * elemSize];
    *b64 = __poolFreeList(p);
    __poolFreeList(p) = index;
}

//----------------------------------------------------------------------------------------------------------------------{DATA}
//----------------------------------------------------------------------------------------------------------------------
// Data API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#if K_OS_WIN32

Data dataLoad(const char* fileName)
{
    Data b = { 0 };

    b.file = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (b.file)
    {
        DWORD fileSizeHigh, fileSizeLow;
        fileSizeLow = GetFileSize(b.file, &fileSizeHigh);
        b.fileMap = CreateFileMappingA(b.file, 0, PAGE_READONLY, fileSizeHigh, fileSizeLow, 0);

        if (b.fileMap)
        {
            b.bytes = MapViewOfFile(b.fileMap, FILE_MAP_READ, 0, 0, 0);
            b.size = ((i64)fileSizeHigh << 32) | fileSizeLow;
        }
        else
        {
            dataUnload(b);
        }
    }

    return b;
}

void dataUnload(Data b)
{
    if (b.bytes)        UnmapViewOfFile(b.bytes);
    if (b.fileMap)      CloseHandle(b.fileMap);
    if (b.file)         CloseHandle(b.file);

    b.bytes = 0;
    b.size = 0;
    b.file = INVALID_HANDLE_VALUE;
    b.fileMap = INVALID_HANDLE_VALUE;
}

Data dataMake(const char* fileName, i64 size)
{
    Data b = { 0 };

    b.file = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (b.file)
    {
        DWORD fileSizeLow = (size & 0xffffffff);
        DWORD fileSizeHigh = (size >> 32);
        b.fileMap = CreateFileMappingA(b.file, 0, PAGE_READWRITE, fileSizeHigh, fileSizeLow, 0);

        if (b.fileMap)
        {
            b.bytes = MapViewOfFile(b.fileMap, FILE_MAP_WRITE, 0, 0, 0);
            b.size = size;
        }
        else
        {
            dataUnload(b);
        }
    }

    return b;
}

#else
#   error Please implement for your platform
#endif

//----------------------------------------------------------------------------------------------------------------------{HASH}
//----------------------------------------------------------------------------------------------------------------------
// Hashing
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

u64 hash(const u8* buffer, i64 len)
{
    u64 h = 14695981039346656037;
    for (i64 i = 0; i < len; ++i)
    {
        h ^= *buffer++;
        h *= (u64)1099511628211ull;
    }

    return h;
}

u64 hashString(const i8* str)
{
    u64 h = 14695981039346656037;
    while (*str != 0)
    {
        h ^= *str++;
        h *= (u64)1099511628211ull;
    }

    return h;
}

//----------------------------------------------------------------------------------------------------------------------{STRING}
//----------------------------------------------------------------------------------------------------------------------
// String API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Strings
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    i64     size;
    i64     capacity;
    u64     hash;
    i32     refCount;
    i32     magic;
    i8      str[0];
}
StringHeader;

#define K_STRING_HEADER(str) ((StringHeader *)(str) - 1)
#define K_CHECK_STRING(str) K_ASSERT(K_STRING_HEADER(str)->magic == 0xc0deface, "Not valid String (maybe a i8*)!")

internal StringHeader* stringAlloc(i64 size)
{
    StringHeader* hdr = 0;

    hdr = K_ALLOC(sizeof(StringHeader) + size + 1);
    if (hdr)
    {
        hdr->size = size;
        hdr->capacity = size + 1;
        hdr->hash = 0;
        hdr->refCount = 1;
        hdr->magic = 0xc0deface;
        hdr->str[size] = 0;
    }

    return hdr;
}

String stringMake(const i8* str)
{
    i64 size = (i64)strlen(str);
    return stringMakeRange(str, str + size);
}

String stringReserve(i64 len, i8 ch)
{
    StringHeader* hdr = stringAlloc(len);
    if (hdr)
    {
        for (i64 i = 0; i < len; ++i) hdr->str[i] = ch;
        hdr->hash = hash(hdr->str, len);
    }

    return hdr ? hdr->str : 0;
}

String stringMakeRange(const i8* start, const i8* end)
{
    i64 size = (i64)(end - start);
    StringHeader* hdr = stringAlloc(size);
    if (hdr)
    {
        memoryCopy(start, hdr->str, size);
        hdr->hash = hash(hdr->str, size);
    }
    return hdr ? hdr->str : 0;
}

void stringRelease(String str)
{
    StringHeader* hdr = K_STRING_HEADER(str);
    K_CHECK_STRING(str);
    if (hdr->refCount == -1) return;
    if (--hdr->refCount == 0)
    {
        K_FREE(hdr, sizeof(StringHeader) + hdr->capacity);
    }
}

String stringAppend(String str1, String str2)
{
    K_CHECK_STRING(str1);
    K_CHECK_STRING(str2);

    {
        i64 sizeStr1 = stringLength(str1);
        i64 sizeStr2 = stringLength(str2);
        i64 totalSize = sizeStr1 + sizeStr2;
        StringHeader* s = stringAlloc(totalSize);

        if (s)
        {
            memoryCopy(str1, s->str, sizeStr1);
            memoryCopy(str2, s->str + sizeStr1, sizeStr2);
            s->hash = hash(s->str, totalSize);
        }

        return s ? s->str : 0;
    }
}

String stringAppendCStr(String str1, const i8* str2)
{
    K_CHECK_STRING(str1);

    {
        i64 sizeStr1 = stringLength(str1);
        i64 sizeStr2 = (i64)strlen(str2);
        i64 totalSize = sizeStr1 + sizeStr2;
        StringHeader* s = stringAlloc(totalSize);

        if (s)
        {
            memoryCopy(str1, s->str, sizeStr1);
            memoryCopy(str2, s->str + sizeStr1, sizeStr2);
            s->hash = hash(s->str, totalSize);
        }

        return s ? s->str : 0;
    }
}

i64 stringLength(String str)
{
    K_CHECK_STRING(str);
    return str ? K_STRING_HEADER(str)->size : 0;
}

u64 stringHash(String str)
{
    K_CHECK_STRING(str);
    return str ? K_STRING_HEADER(str)->hash : 0;
}

String stringFormatV(const i8* format, va_list args)
{
    int numChars = vsnprintf(0, 0, format, args);
    StringHeader* hdr = stringAlloc(numChars);
    vsnprintf(hdr->str, numChars + 1, format, args);
    hdr->hash = hashString(hdr->str);
    return hdr->str;
}

String stringFormat(const i8* format, ...)
{
    String str;
    va_list args;
    va_start(args, format);
    str = stringFormatV(format, args);
    va_end(args);
    return str;
}

int stringCompare(String s1, const i8* s2)
{
    K_CHECK_STRING(s1);
    StringHeader* hdr = K_STRING_HEADER(s1);
    return stringCompareStringRange(hdr->str, hdr->str + hdr->size, s2);
}

bool stringEqual(String s1, String s2)
{
    K_CHECK_STRING(s1);
    K_CHECK_STRING(s2);
    StringHeader* hdr1 = K_STRING_HEADER(s1);
    StringHeader* hdr2 = K_STRING_HEADER(s2);

    if (hdr1->size != hdr2->size) return NO;
    if (hdr1->hash != hdr2->hash) return NO;

    for (i64 i = 0; i < hdr1->size; ++i)
    {
        if (s1[i] != s2[i]) return NO;
    }

    return YES;
}

//----------------------------------------------------------------------------------------------------------------------
// Static Arena Strings
//----------------------------------------------------------------------------------------------------------------------

String arenaStringCopy(Arena* arena, const i8* str)
{
    i64 len = (i64)(strlen(str));
    return arenaStringCopyRange(arena, str, str + len);
}

String arenaStringCopyRange(Arena* arena, const i8* str, const i8* end)
{
    i64 len = (i64)(end - str);
    i8* buffer = (i8 *)arenaAlignedAlloc(arena, sizeof(StringHeader) + len + 1);
    StringHeader* hdr = (StringHeader *)buffer;

    buffer += sizeof(StringHeader);
    hdr->size = len;
    hdr->capacity = -1;
    hdr->hash = 0;
    hdr->refCount = -1;
    hdr->magic = 0xc0deface;

    memoryCopy(str, buffer, len);
    buffer[len] = 0;

    hdr->hash = hash(buffer, len);

    return buffer;
}

String arenaStringFormatV(Arena* arena, const i8* format, va_list args)
{
    int numChars = vsnprintf(0, 0, format, args);
    i8* buffer = (i8 *)arenaAlignedAlloc(arena, sizeof(StringHeader) + numChars + 1);
    StringHeader* hdr = (StringHeader*)buffer;

    buffer += sizeof(StringHeader);
    vsnprintf(buffer, numChars + 1, format, args);
    hdr->size = numChars;
    hdr->capacity = -1;
    hdr->hash = 0;
    hdr->refCount = -1;
    hdr->magic = 0xc0deface;

    hdr->hash = hash(buffer, numChars);

    return buffer;
}

String arenaStringFormat(Arena* arena, const i8* format, ...)
{
    va_list args;
    String s = 0;

    va_start(args, format);
    s = arenaStringFormatV(arena, format, args);
    va_end(args);

    return s;
}

//----------------------------------------------------------------------------------------------------------------------
// String Table
//----------------------------------------------------------------------------------------------------------------------

#define K_STRINGTABLE_HASHTABLE_SIZE(st) (*(i64 *)((st)->start))
#define K_STRINGTABLE_HASHTABLE(st) ((i64 *)((st)->start))

typedef struct
{
    i64             nextBlock;
    StringHeader    hdr;
}
StringTableHeader;

void stringTableInit(StringTable* st, i64 size, i64 sizeHashTable)
{
    // Ensure we have enough room for the hash table
    K_ASSERT(size > (i64)(sizeof(i64) * sizeHashTable));
    K_ASSERT(size > 0);
    K_ASSERT(sizeHashTable > 1);

    // Create the memory arena
    arenaInit(st, size);
    if (st->start)
    {
        // Index 0 in the hash table holds the size of the hash table
        i64* hashTable = (i64 *)arenaAlloc(st, sizeHashTable * sizeof(i64));
        K_STRINGTABLE_HASHTABLE_SIZE(st) = sizeHashTable;
        for (int i = 1; i < sizeHashTable; ++i) hashTable[i] = 0;
    }
}

void stringTableDone(StringTable* table)
{
    arenaDone(table);
}

internal StringToken __stringTableAdd(StringTable* table, const i8* str, i64 strLen)
{
    u8* b = (u8 *)table->start;
    i64* hashTable = K_STRINGTABLE_HASHTABLE(table);
    u64 h = hash(str, strLen);
    i64 index = (i64)((u64)h % K_STRINGTABLE_HASHTABLE_SIZE(table));
    i64* blockPtr = 0;

    index = index ? index : 1;
    blockPtr = &hashTable[index];
    while (*blockPtr != 0)
    {
        StringTableHeader* hdr = (StringTableHeader*)&b[*blockPtr];
        if (hdr->hdr.hash == h)
        {
            // Possible match
            if (hdr->hdr.size == strLen)
            {
                if (memoryCompare(str, hdr->hdr.str, strLen) == 0)
                {
                    // Found it!
                    return hdr->hdr.str - table->start;
                }
            }
        }

        blockPtr = &hdr->nextBlock;
    }

    // The string has not be found - create a new entry
    {
        StringTableHeader* hdr = arenaAlignedAlloc(table, sizeof(StringTableHeader) + strLen + 1);

        if (!hdr) return 0;

        hdr->nextBlock = 0;

        hdr->hdr.capacity = strLen + 1;
        hdr->hdr.hash = h;
        hdr->hdr.magic = 0xc0deface;
        hdr->hdr.refCount = -1;
        hdr->hdr.size = strLen;
        memoryCopy(str, hdr->hdr.str, strLen);
        hdr->hdr.str[strLen] = 0;

        *blockPtr = (u8 *)hdr - b;

        return hdr->hdr.str - table->start;
    }
}

StringToken stringTableAdd(StringTable* table, const i8* str)
{
    return __stringTableAdd(table, str, (i64)strlen(str));
}

StringToken stringTableAddRange(StringTable* table, const i8* str, const i8* end)
{
    return __stringTableAdd(table, str, (i64)end - (i64)str);
}

String stringTableGet(StringTable* table, StringToken token)
{
    return (String)(table->start + token);
}

//----------------------------------------------------------------------------------------------------------------------
// Paths
//----------------------------------------------------------------------------------------------------------------------

internal const i8* __findLastChar(String path, char c)
{
    K_CHECK_STRING(path);

    {
        const i8* end = path + stringLength(path);
        const i8* s = end;
        while ((*s != c) && (s > path)) --s;
        if (s == path) s = end;

        return s;
    }
}

String pathDirectory(String path)
{
    K_CHECK_STRING(path);

    {
        const i8* endPath = __findLastChar(path, '\\');
        return stringMakeRange(path, endPath);
    }
}

String pathRemoveExtension(String path)
{
    K_CHECK_STRING(path);

    {
        const i8* ext = __findLastChar(path, '.');
        return stringMakeRange(path, ext);
    }
}

String pathReplaceExtension(String path, const char* ext)
{
    K_CHECK_STRING(path);

    {
        const i8* extStart = __findLastChar(path, '.');
        i64 lenExt = (i64)strlen(ext);
        i64 lenBase = (i64)(extStart - path);
        i64 lenStr = lenBase + 1 + lenExt;
        StringHeader* hdr = stringAlloc(lenStr);
        memoryCopy(path, hdr->str, lenBase);
        hdr->str[lenBase] = '.';
        memoryCopy(ext, &hdr->str[lenBase + 1], lenExt);
        return hdr->str;
    }
}

String pathJoin(String p1, String p2)
{
    return stringFormat("%s/%s", p1, p2);
}

//----------------------------------------------------------------------------------------------------------------------
// String utilities
//----------------------------------------------------------------------------------------------------------------------

int stringCompareStringRange(const i8* s1, const i8* s2, const i8* s3)
{
    while ((s1 < s2) && *s3 != 0)
    {
        int d = *s1++ - *s3++;
        if (d != 0) return d;
    }

    return s1 == s2 ? -*s3 : *s1 - *s3;
}

//----------------------------------------------------------------------------------------------------------------------{RANDOM}
//----------------------------------------------------------------------------------------------------------------------
// Random API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL    // Most significant 33 bits
#define LM 0x7FFFFFFFULL            // Least significant 31 bits

void randomInit(Random* R)
{
    __time64_t t = _time64(&t);
    randomInitSeed(R, t);
}

void randomInitSeed(Random* R, u64 seed)
{
    R->table[0] = seed;
    for (int i = 1; i < K_RANDOM_TABLE_SIZE; ++i)
        R->table[i] = (6364136223846793005ULL * (R->table[i - 1] ^ (R->table[i - 1] >> 62)) + i);
    R->index = K_RANDOM_TABLE_SIZE;
}

void randomInitArray(Random* R, u64* seeds, i64 len)
{
    u64 i, j, k;
    randomInitSeed(R, 19650218ULL);
    i = 1; j = 0;
    k = (K_RANDOM_TABLE_SIZE > len ? K_RANDOM_TABLE_SIZE : len);
    for (; k; k--) {
        R->table[i] = (R->table[i] ^ ((R->table[i - 1] ^ (R->table[i - 1] >> 62)) * 3935559000370003845ULL))
            + seeds[j] + j;
        ++i; ++j;
        if (i >= K_RANDOM_TABLE_SIZE) { R->table[0] = R->table[K_RANDOM_TABLE_SIZE - 1]; i = 1; }
        if ((i64)j >= len) j = 0;
    }
    for (k = K_RANDOM_TABLE_SIZE - 1; k; k--) {
        R->table[i] = (R->table[i] ^ ((R->table[i - 1] ^ (R->table[i - 1] >> 62)) * 2862933555777941757ULL)) - i;
        i++;
        if (i >= K_RANDOM_TABLE_SIZE) { R->table[0] = R->table[K_RANDOM_TABLE_SIZE - 1]; i = 1; }
    }

    R->table[0] = 1ULL << 63;
}

u64 random64(Random* R)
{
    int i;
    u64 x;
    static u64 mag01[2] = { 0ULL, MATRIX_A };

    if (R->index >= K_RANDOM_TABLE_SIZE)
    {
        // Generate K_RANDOM_TABLE_SIZE words at one time.
        for (i = 0; i < K_RANDOM_TABLE_SIZE - MM; ++i) {
            x = (R->table[i] & UM) | (R->table[i + 1] & LM);
            R->table[i] = R->table[i + MM] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        for (; i < K_RANDOM_TABLE_SIZE - 1; ++i) {
            x = (R->table[i] & UM) | (R->table[i + 1] & LM);
            R->table[i] = R->table[i + (MM - K_RANDOM_TABLE_SIZE)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        x = (R->table[K_RANDOM_TABLE_SIZE - 1] & UM) | (R->table[0] & LM);
        R->table[K_RANDOM_TABLE_SIZE - 1] = R->table[MM - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];

        R->index = 0;
    }

    x = R->table[R->index++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

#undef MM
#undef MATRIX_A
#undef UM
#undef LM


f64 randomFloat(Random* R)
{
    return (random64(R) >> 11) * (1.0 / 9007199254740991.0);
}

f64 randomFloatNo1(Random* R)
{
    return (random64(R) >> 11) * (1.0 / 9007199254740992.0);
}

f64 randomFloatNo0Or1(Random* R)
{
    return ((random64(R) >> 12) + 0.5) * (1.0 / 4503599627370496.0);
}

//----------------------------------------------------------------------------------------------------------------------{CRC32}
//----------------------------------------------------------------------------------------------------------------------
// CRC-32 API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

// Table of CRCs of all 8-bit messages.
unsigned long kCrcTable[256];

// Flag: has the table been computed? Initially false.
bool gCrcTableComputed = NO;

// Make the table for a fast CRC.
void __crcMakeTable(void)
{
    u32 c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (u32)n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        kCrcTable[n] = c;
    }
    gCrcTableComputed = YES;
}

// Update a running CRC with the bytes data[0..len-1]--the CRC
// should be initialized to all 1's, and the transmitted value
// is the 1's complement of the final running CRC (see the
// crc() routine below). */

u32 crc32Update(u32 crc, void* data, i64 len)
{
    u32 c = crc;
    u8* d = (u8 *)data;

    if (!gCrcTableComputed) __crcMakeTable();
    for (i64 n = 0; n < len; n++) {
        c = kCrcTable[(c ^ d[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
u32 crc32(void* data, i64 len)
{
    return crc32Update(0xffffffffL, data, len) ^ 0xffffffffL;
}

//----------------------------------------------------------------------------------------------------------------------{SHA1}
//----------------------------------------------------------------------------------------------------------------------
// SHA-1 API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#define XTL_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#if K_LITTLE_ENDIAN
#   define XTL_BLK0(i) (block->l[i] = (XTL_ROL(block->l[i], 24) & 0xff00ff00) | (XTL_ROL(block->l[i], 8) & 0x00ff00ff))
#else
#   define XTL_BLK0(i) block->l[i]
#endif

#define XTL_BLK(i) (block->l[i & 15] = XTL_ROL(block->l[(i+13) & 15] ^ block->l[(i+8) & 15] ^ block->l[(i+2) & 15] ^ block->l[i & 15], 1))

#define XTL_R0(v, w, x, y, z, i) z += ((w & (x^y))^y) + XTL_BLK0(i) + 0x5a827999 + XTL_ROL(v, 5); w = XTL_ROL(w, 30);
#define XTL_R1(v, w, x, y, z, i) z += ((w & (x^y))^y)+ XTL_BLK(i) + 0x5a827999 + XTL_ROL(v, 5); w = XTL_ROL(w, 30);
#define XTL_R2(v, w, x, y, z, i) z += (w^x^y) + XTL_BLK(i) + 0x6ed9eba1 + XTL_ROL(v, 5); w = XTL_ROL(w, 30);
#define XTL_R3(v, w, x, y, z, i) z += (((w|x) & y) | (w & x)) + XTL_BLK(i) + 0x8f1bbcdc + XTL_ROL(v, 5); w = XTL_ROL(w, 30);
#define XTL_R4(v, w, x, y, z, i) z += (w^x^y) + XTL_BLK(i) + 0xca62c1d6 + XTL_ROL(v, 5); w = XTL_ROL(w, 30);

void sha1Init(Sha1* s)
{
    s->state[0] = 0x67452301u;
    s->state[1] = 0xefcdab89u;
    s->state[2] = 0x98badcfeu;
    s->state[3] = 0x10325476u;
    s->state[4] = 0xc3d2e1f0u;

    s->count[0] = 0;
    s->count[1] = 0;

    s->finalised = NO;
}


void sha1ProcessBuffer(Sha1* s, const void* data, i64 numBytes)
{
    sha1Init(s);
    sha1Add(s, data, numBytes);
    sha1Finalise(s);
}

void sha1ProcessString(Sha1* s, String str)
{
    sha1ProcessBuffer(s, str, stringLength(str));
}

void sha1ProcessHexString(Sha1* s, String hexStr)
{
    for (int i = 0; i < 20; ++i)
    {
        int byte = 0;
        for (int digit = 0; digit < 2; ++digit)
        {
            int nibble = 0;
            char c = hexStr[i * 2 + digit];
            if (c >= '0' && c <= '9')
            {
                nibble = c - '0';
            }
            else if (c >= 'a' && c <= 'f')
            {
                nibble = c - 'a' + 10;
            }
            else
            {
                K_ASSERT(0);
            }

            byte <<= 4;
            byte += nibble;
        }

        s->digest[i] = byte;
    }

    s->finalised = YES;
}

void sha1ProcessData(Sha1* s, Data data)
{
    sha1ProcessBuffer(s, data.bytes, data.size);
}

internal void __sha1Transform(Sha1* s, const u8* buffer)
{
    u32 a, b, c, d, e;

    typedef union
    {
        u8 c[64];
        u32 l[16];
    }
    char64long16_t;

    char64long16_t block[1];    // using array to appear as a pointer.
    memoryCopy(buffer, block, 64);

    a = s->state[0];
    b = s->state[1];
    c = s->state[2];
    d = s->state[3];
    e = s->state[4];

    XTL_R0(a, b, c, d, e, 0);
    XTL_R0(e, a, b, c, d, 1);
    XTL_R0(d, e, a, b, c, 2);
    XTL_R0(c, d, e, a, b, 3);
    XTL_R0(b, c, d, e, a, 4);
    XTL_R0(a, b, c, d, e, 5);
    XTL_R0(e, a, b, c, d, 6);
    XTL_R0(d, e, a, b, c, 7);
    XTL_R0(c, d, e, a, b, 8);
    XTL_R0(b, c, d, e, a, 9);
    XTL_R0(a, b, c, d, e, 10);
    XTL_R0(e, a, b, c, d, 11);
    XTL_R0(d, e, a, b, c, 12);
    XTL_R0(c, d, e, a, b, 13);
    XTL_R0(b, c, d, e, a, 14);
    XTL_R0(a, b, c, d, e, 15);
    XTL_R1(e, a, b, c, d, 16);
    XTL_R1(d, e, a, b, c, 17);
    XTL_R1(c, d, e, a, b, 18);
    XTL_R1(b, c, d, e, a, 19);
    XTL_R2(a, b, c, d, e, 20);
    XTL_R2(e, a, b, c, d, 21);
    XTL_R2(d, e, a, b, c, 22);
    XTL_R2(c, d, e, a, b, 23);
    XTL_R2(b, c, d, e, a, 24);
    XTL_R2(a, b, c, d, e, 25);
    XTL_R2(e, a, b, c, d, 26);
    XTL_R2(d, e, a, b, c, 27);
    XTL_R2(c, d, e, a, b, 28);
    XTL_R2(b, c, d, e, a, 29);
    XTL_R2(a, b, c, d, e, 30);
    XTL_R2(e, a, b, c, d, 31);
    XTL_R2(d, e, a, b, c, 32);
    XTL_R2(c, d, e, a, b, 33);
    XTL_R2(b, c, d, e, a, 34);
    XTL_R2(a, b, c, d, e, 35);
    XTL_R2(e, a, b, c, d, 36);
    XTL_R2(d, e, a, b, c, 37);
    XTL_R2(c, d, e, a, b, 38);
    XTL_R2(b, c, d, e, a, 39);
    XTL_R3(a, b, c, d, e, 40);
    XTL_R3(e, a, b, c, d, 41);
    XTL_R3(d, e, a, b, c, 42);
    XTL_R3(c, d, e, a, b, 43);
    XTL_R3(b, c, d, e, a, 44);
    XTL_R3(a, b, c, d, e, 45);
    XTL_R3(e, a, b, c, d, 46);
    XTL_R3(d, e, a, b, c, 47);
    XTL_R3(c, d, e, a, b, 48);
    XTL_R3(b, c, d, e, a, 49);
    XTL_R3(a, b, c, d, e, 50);
    XTL_R3(e, a, b, c, d, 51);
    XTL_R3(d, e, a, b, c, 52);
    XTL_R3(c, d, e, a, b, 53);
    XTL_R3(b, c, d, e, a, 54);
    XTL_R3(a, b, c, d, e, 55);
    XTL_R3(e, a, b, c, d, 56);
    XTL_R3(d, e, a, b, c, 57);
    XTL_R3(c, d, e, a, b, 58);
    XTL_R3(b, c, d, e, a, 59);
    XTL_R4(a, b, c, d, e, 60);
    XTL_R4(e, a, b, c, d, 61);
    XTL_R4(d, e, a, b, c, 62);
    XTL_R4(c, d, e, a, b, 63);
    XTL_R4(b, c, d, e, a, 64);
    XTL_R4(a, b, c, d, e, 65);
    XTL_R4(e, a, b, c, d, 66);
    XTL_R4(d, e, a, b, c, 67);
    XTL_R4(c, d, e, a, b, 68);
    XTL_R4(b, c, d, e, a, 69);
    XTL_R4(a, b, c, d, e, 70);
    XTL_R4(e, a, b, c, d, 71);
    XTL_R4(d, e, a, b, c, 72);
    XTL_R4(c, d, e, a, b, 73);
    XTL_R4(b, c, d, e, a, 74);
    XTL_R4(a, b, c, d, e, 75);
    XTL_R4(e, a, b, c, d, 76);
    XTL_R4(d, e, a, b, c, 77);
    XTL_R4(c, d, e, a, b, 78);
    XTL_R4(b, c, d, e, a, 79);

    s->state[0] += a;
    s->state[1] += b;
    s->state[2] += c;
    s->state[3] += d;
    s->state[4] += e;
}

void sha1Add(Sha1* s, const void* data, i64 numBytes)
{
    K_ASSERT(!s->finalised);

    u32 i, j = s->count[0];

    if ((s->count[0] += (u32)(numBytes << 3)) < j)
    {
        ++s->count[1];
    }
    s->count[1] += (u32)(numBytes >> 29);
    j = (j >> 3) & 63;
    if ((j + numBytes) > 63)
    {
        memoryCopy(data, &s->buffer[j], (i = 64 - j));
        __sha1Transform(s, s->buffer);
        for (; i + 64 < numBytes; i += 64)
        {
            __sha1Transform(s, &((const unsigned char *)data)[i]);
        }
        j = 0;
    }
    else
    {
        i = 0;
    }
    memoryCopy(&((const unsigned char *)data)[i], &s->buffer[j], numBytes - i);
}

void sha1Finalise(Sha1* s)
{
    K_ASSERT(!s->finalised);
    u8 finalCount[8];

    for (unsigned i = 0; i < 8; ++i)
    {
        finalCount[i] = (u8)((s->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
    }

    u8 c = 0200;
    sha1Add(s, &c, 1);
    while ((s->count[0] & 504) != 448)
    {
        c = 0000;
        sha1Add(s, &c, 1);
    }
    sha1Add(s, finalCount, 8);
    for (unsigned i = 0; i < 20; ++i)
    {
        s->digest[i] = (u8)((s->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }

    s->finalised = YES;
}

String sha1Hex(Sha1* s)
{
    String str = stringReserve(40, ' ');

    for (int i = 0; i < 20; ++i)
    {
        static char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        unsigned char c = s->digest[i];
        str[i*2] = hexDigits[c / 16];
        str[i*2+1] = hexDigits[c % 16];
    }

    return str;
}

u64 sha1Hash64(Sha1* s)
{
    u64 h = 0;
    for (int i = 0; i < 8; ++i)
    {
        h = (h << 8) + (u64)s->digest[i];
    }

    return h;
}

bool sha1Equal(Sha1* s1, Sha1* s2)
{
    for (int i = 0; i < 20; ++i)
    {
        if (s1->digest[i] != s2->digest[i]) return NO;
    }

    return FALSE;
}

//----------------------------------------------------------------------------------------------------------------------{SPAWN}
//----------------------------------------------------------------------------------------------------------------------
// SPAWN API
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

bool processStartAndWait(const i8* fileName, int argc, const i8** argv)
{
    bool result = NO;
    char* args;
    char* s;

    // Collect information about the arguments
    size_t fileNameSize = strlen(fileName);
    size_t argsSize = fileNameSize + 1;
    bool* hasSpaces = K_ALLOC(sizeof(bool) * argc);

    for (int i = 0; i < argc; ++i)
    {
        hasSpaces[i] = NO;
        for (int j = 0; argv[i][j] != 0; ++j)
        {
            if (argv[i][j] == ' ') hasSpaces[i] = YES;
            ++argsSize;
        }

        if (hasSpaces[i]) argsSize += 2;    // for quotes
        ++argsSize;                         // following space
    }

    // Generate the command line
    s = args = K_ALLOC(argsSize + 1);
    memoryCopy(fileName, s, fileNameSize);
    s += fileNameSize;
    *s++ = ' ';

    for (int i = 0; i < argc; ++i)
    {
        size_t len = strlen(argv[i]);

        if (hasSpaces[i]) *s++ = '"';
        memoryCopy(argv[i], s, len);
        s += len;
        if (hasSpaces[i]) *s++ = '"';
        if (i < (argc - 1)) *s++ = ' ';
    }
    *s = 0;

#if K_OS_WIN32
    {
        PROCESS_INFORMATION process;
        STARTUPINFO si;

        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&process, sizeof(process));

        si.cb = sizeof(si);

        if (CreateProcess(NULL, args, 0, 0, 0, NORMAL_PRIORITY_CLASS, 0, 0, &si, &process))
        {
            WaitForSingleObject(process.hProcess, INFINITE);
            result = YES;
        }
        else
        {
            DWORD err = GetLastError();
            err = err;
        }
    }
#else
#   error Implement processStartAndWait for you OS.
#endif

    K_FREE(args, argsSize + 1);
    K_FREE(hasSpaces, sizeof(bool) * argc);

    return result;
}

//----------------------------------------------------------------------------------------------------------------------{PNG}
//----------------------------------------------------------------------------------------------------------------------
// PNG writing
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#define K_DEFLATE_MAX_BLOCK_SIZE    65536
#define K_BLOCK_HEADER_SIZE         5

internal u32 __pngAdler32(u32 state, const u8* data, i64 len)
{
    u16 s1 = state;
    u16 s2 = state >> 16;
    for (i64 i = 0; i < len; ++i)
    {
        s1 = (s1 + data[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (u32)s2 << 16 | s1;
}

bool pngWrite(const char* fileName, u32* img, int width, int height)
{
    // Swizzle image from ARGB to ABGR
    u32* newImg = K_ALLOC(sizeof(u32)*width*height);
    u8* src = (u8 *)img;
    u8* dst = (u8 *)newImg;

    // Data is BGRABGRA...  should be RGBARGBA...
    for (int yy = 0; yy < height; ++yy)
    {
        for (int xx = 0; xx < width; ++xx)
        {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
            src += 4;
            dst += 4;
        }
    }
    img = newImg;

    // Calculate size of PNG
    i64 fileSize = 0;
    i64 lineSize = width * sizeof(u32) + 1;
    i64 imgSize = lineSize * height;
    i64 overheadSize = imgSize / K_DEFLATE_MAX_BLOCK_SIZE;
    if (overheadSize * K_DEFLATE_MAX_BLOCK_SIZE < imgSize)
    {
        ++overheadSize;
    }
    overheadSize = overheadSize * 5 + 6;
    i64 dataSize = imgSize + overheadSize;      // size of zlib + deflate output
    u32 adler = 1;
    i64 deflateRemain = imgSize;
    u8* imgBytes = (u8 *)img;

    fileSize = 43;
    fileSize += dataSize + 4;       // IDAT deflated data

                                    // Open arena
    Arena m;
    arenaInit(&m, dataSize + K_KB(1));
    u8* p = arenaAlloc(&m, 43);
    u8* start = p;

    // Write file format
    u8 header[] = {
        // PNG header
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
        // IHDR chunk
        0x00, 0x00, 0x00, 0x0d,                                 // length
        0x49, 0x48, 0x44, 0x52,                                 // 'IHDR'
        width >> 24, width >> 16, width >> 8, width,            // width
        height >> 24, height >> 16, height >> 8, height,        // height
        0x08, 0x06, 0x00, 0x00, 0x00,                           // 8-bit depth, true-colour+alpha format
        0x00, 0x00, 0x00, 0x00,                                 // CRC-32 checksum
                                                                // IDAT chunk
        (u8)(dataSize >> 24), (u8)(dataSize >> 16), (u8)(dataSize >> 8), (u8)dataSize,
        0x49, 0x44, 0x41, 0x54,                                 // 'IDAT'
                                                                // Deflate data
        0x08, 0x1d,                                             // ZLib CMF, Flags (Compression level 0)
    };
    memoryCopy(header, p, sizeof(header) / sizeof(header[0]));
    u32 crc = crc32(&p[12], 17);
    p[29] = crc >> 24;
    p[30] = crc >> 16;
    p[31] = crc >> 8;
    p[32] = crc;
    crc = crc32(&p[37], 6);

    // Write out the pixel data compressed
    int x = 0;
    int y = 0;
    i64 count = width * height * sizeof(u32);
    i64 deflateFilled = 0;
    while (count > 0)
    {
        // Start DEFALTE block
        if (deflateFilled == 0)
        {
            u32 size = K_DEFLATE_MAX_BLOCK_SIZE;
            if (deflateRemain < (i64)size)
            {
                size = (u16)deflateRemain;
            }
            u8 blockHeader[K_BLOCK_HEADER_SIZE] = {
                deflateRemain <= K_DEFLATE_MAX_BLOCK_SIZE ? 1 : 0,
                size,
                size >> 8,
                (size) ^ 0xff,
                (size >> 8) ^ 0xff
            };
            p = arenaAlloc(&m, sizeof(blockHeader));
            memoryCopy(blockHeader, p, sizeof(blockHeader));
            crc = crc32Update(crc, blockHeader, sizeof(blockHeader));
        }

        // Calculate number of bytes to write in this loop iteration
        u32 n = 0xffffffff;
        if ((u32)count < n)
        {
            n = (u32)count;
        }
        if ((u32)(lineSize - x) < n)
        {
            n = (u32)(lineSize - x);
        }
        K_ASSERT(deflateFilled < K_DEFLATE_MAX_BLOCK_SIZE);
        if ((u32)(K_DEFLATE_MAX_BLOCK_SIZE - deflateFilled) < n)
        {
            n = (u32)(K_DEFLATE_MAX_BLOCK_SIZE - deflateFilled);
        }
        K_ASSERT(n != 0);

        // Beginning of row - write filter method
        if (x == 0)
        {
            p = arenaAlloc(&m, 1);
            *p = 0;
            crc = crc32Update(crc, p, 1);
            adler = __pngAdler32(adler, p, 1);
            --deflateRemain;
            ++deflateFilled;
            ++x;
            if (count != n) --n;
        }

        // Write bytes and update checksums
        p = arenaAlloc(&m, n);
        memoryCopy(imgBytes, p, n);
        crc = crc32Update(crc, imgBytes, n);
        adler = __pngAdler32(adler, imgBytes, n);
        imgBytes += n;
        count -= n;
        deflateRemain -= n;
        deflateFilled += n;
        if (deflateFilled == K_DEFLATE_MAX_BLOCK_SIZE)
        {
            deflateFilled = 0;
        }
        x += n;
        if (x == lineSize) {
            x = 0;
            ++y;
            if (y == height)
            {
                // Wrap things up
                u8 footer[20] = {
                    adler >> 24, adler >> 16, adler >> 8, adler,    // Adler checksum
                    0, 0, 0, 0,                                     // Chunk crc-32 checksum
                                                                    // IEND chunk
                                                                    0x00, 0x00, 0x00, 0x00,
                                                                    0x49, 0x45, 0x4e, 0x44,
                                                                    0xae, 0x42, 0x60, 0x82,
                };
                crc = crc32Update(crc, footer, 20);
                footer[4] = crc >> 24;
                footer[5] = crc >> 16;
                footer[6] = crc >> 8;
                footer[7] = crc;

                p = arenaAlloc(&m, 20);
                memoryCopy(footer, p, 20);
                break;
            }
        }
    }

    // Transfer file
    K_FREE(newImg, sizeof(u32)*width*height);
    u8* end = arenaAlloc(&m, 0);
    i64 numBytes = (i64)(end - start);
    Data d = dataMake(fileName, numBytes);
    if (d.bytes)
    {
        memoryCopy(start, d.bytes, numBytes);
        dataUnload(d);
        arenaDone(&m);
        return YES;
    }
    else
    {
        return NO;
    }
}

//----------------------------------------------------------------------------------------------------------------------{PATTERN}
//----------------------------------------------------------------------------------------------------------------------
// Simple pattern matching
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

internal bool matchHere(const i8* regexp, const i8* text);
internal bool matchStar(i8 c, const i8* regexp, const i8* text);

bool match(const i8 *regexp, const i8* text)
{
    if (regexp[0] == '^')
    {
        return matchHere(regexp + 1, text);
    }
    do {
        if (matchHere(regexp, text)) return YES;
    } while (*text++ != 0);
    return NO;
}

internal bool matchHere(const i8* regexp, const i8* text)
{
    if (regexp[0] == 0) return YES;
    if (regexp[1] == '*') return matchStar(regexp[0], regexp + 2, text);
    if (regexp[0] == '$' && regexp[1] == 0) return K_BOOL(*text == 0);
    if (*text != 0 && (regexp[0] == '.' || regexp[0] == *text)) return matchHere(regexp + 1, text + 1);
    return NO;
}

internal bool matchStar(i8 c, const i8* regexp, const i8* text)
{
    do {
        if (matchHere(regexp, text)) return YES;
    } while (*text != 0 && (*text++ == c || c == '.'));
    return NO;
}

//----------------------------------------------------------------------------------------------------------------------{CONSOLE}
//----------------------------------------------------------------------------------------------------------------------
// Console control
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

void consoleOpen()
{
#if K_OS_WIN32
    AllocConsole();
    SetConsoleTitleA("Debug Window");

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((intptr_t)handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    freopen("CONOUT$", "w", stdout);

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((intptr_t)handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 0);
    freopen("CONIN$", "r", stdin);
#endif

    consoleEnableANSIColours();
}

void consolePause()
{
    printf("\n\033[33;1mPress any key...\033[0m\n\n");
    _getch();
}

void consoleEnableANSIColours()
{
#if K_OS_WIN32
    DWORD mode;
    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(handle_out, &mode);
    mode |= 0x4;
    SetConsoleMode(handle_out, mode);
#endif
}


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
