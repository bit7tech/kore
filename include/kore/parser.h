//----------------------------------------------------------------------------------------------------------------------
// Parser
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <kore/kore.h>

#if !defined(PARSER_KEYWORD_INDEX)
#   define PARSER_KEYWORD_INDEX 20
#endif

#if !defined(PARSER_OPERATOR_INDEX)
#   define PARSER_OPERATOR_INDEX 500
#endif

#if !defined(PARSER_KEYWORD_HASHTABLE_SIZE)
#   define PARSER_KEYWORD_HASHTABLE_SIZE 16
#endif

//----------------------------------------------------------------------------------------------------------------------
// Data structures
//----------------------------------------------------------------------------------------------------------------------

// NOTE: If you add any entries here, please edit lexDump.

typedef enum
{
    T_Unknown,
    T_Error,
    T_EOF,

    // Literals
    T_NewLine,
    T_Symbol,
    T_Integer,
    T_Real,

    // Keywords (200-299)
    T_KEYWORDS = PARSER_KEYWORD_INDEX,

    // Operators
    T_OPERATORS = PARSER_OPERATOR_INDEX,
}
Token;

typedef enum
{
    LNCT_Invalid,
    LNCT_Valid,
    LNCT_NotInitial,
}
LexNameCharType;

// Used to customise the lexical analyser
typedef struct
{
    //
    // Comment definition
    // Comments take the form of:
    //
    //      Line-based:     01 ..... \n
    //      Block-based:    02 ..... 20
    //
    // Where:
    //
    //      0 = first comment character (same for both types of comments)
    //      1 = second comment character for line-based
    //      2 = second comment character for block-based
    //
    // And notice block-based are terminated by same characters in reverse.
    //
    // For example, for C++ comments:
    //
    //      0 = '/'
    //      1 = '/'
    //      2 = '*'
    //

    char    m_comment0;         // First character for both types of comments (line-based and block)
    char    m_commentLine;      // Second character for line-based comments
    char    m_commentBlock;     // Second character for block comments.
    bool    m_trackNewLines;    // If set to YES, newline tokens are returned.

    //
    // Symbol or keyword characters
    //
    // Value is either 0, 1 or 2:
    //
    //      0 = not a valid symbol/keyword character
    //      1 = Valid symbol/keyword character
    //      2 = Valid symbol/keyword character only for the 2nd character onwards (cannot start with this character).
    //

    u8      m_nameChars[128];

    //
    // Keyword hashes
    //
    // A keyword hash is generated an the bottom 4 bits are used to index this table.  Each 8-bits in the 64-bits can
    // represent a Token enum that matches that hash.
    //

    u64                 m_keywordHashes[PARSER_KEYWORD_HASHTABLE_SIZE];
    Array(StringToken)  m_keywords;
    Array(i64)          m_keywordLengths;
    StringTable         m_nameStore;

    //
    // Operators
    //

    Array(StringToken)  m_operators;
}
LexConfig;

typedef struct 
{
    i64     m_lineOffset;   // Beginning of current line
    i32     m_line;         // Line #
    i32     m_col;          // Column #
}
LexPos;

typedef struct 
{
    Token           m_token;
    StringToken     m_symbol;
    union {
        i64             m_integer;
        f64             m_real;
    };

    const i8*       m_s0;           // Start reference to source material
    const i8*       m_s1;           // End reference to source material
    LexPos          m_position;     // Position into source material
}
LexInfo;

typedef void (*LexOutputFunc) (const i8* msg);

typedef struct  
{
    // External source
    String          m_source;
    LexConfig       m_config;
    LexOutputFunc   m_outputFunc;
    Arena           m_scratch;
    Arena*          m_symbols;
    const i8*       m_start;
    const i8*       m_end;

    // Generated data
    Array(LexInfo)  m_info;

    // Parsing state
    const i8*       m_cursor;
    const i8*       m_lastCursor;
    LexPos          m_position;
    LexPos          m_lastPosition;
}
Lex;

//----------------------------------------------------------------------------------------------------------------------
// Lexical Analyser
//----------------------------------------------------------------------------------------------------------------------

//
// Configuration initialisation
//

void lexConfigInit(LexConfig* LC);
void lexConfigDone(LexConfig* LC);
void lexConfigInitComments(LexConfig* LC, const i8* lineComment, const i8* blockStartComment, const i8* blockEndComment);
void lexConfigAddNameCharsRange(LexConfig* LC, LexNameCharType type, char start, char end);
void lexConfigAddNameCharsString(LexConfig* LC, LexNameCharType type, const i8* str);
Token lexConfigAddOperator(LexConfig* LC, const i8* operator);
Token lexConfigAddKeyword(LexConfig* LC, const i8* keyword);

//
// Lexical analysis
//

void lex(Lex* L, LexConfig* config, LexOutputFunc outputFunc, StringTable* symbols, String source, const i8* start, const i8* end);
void lexDone(Lex* L);
void lexDump(Lex* L);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef K_IMPLEMENTATION

#include <ctype.h>

//----------------------------------------------------------------------------------------------------------------------
// Configuration
//----------------------------------------------------------------------------------------------------------------------

void lexConfigInit(LexConfig* LC)
{
    memoryClear(LC, sizeof(LexConfig));
    LC->m_comment0 = '/';
    LC->m_commentLine = '/';
    LC->m_commentBlock = '*';
    LC->m_trackNewLines = NO;
    for (int i = 0; i < 128; ++i) LC->m_nameChars[i] = (u8)LNCT_Invalid;
    for (int i = 0; i < PARSER_KEYWORD_HASHTABLE_SIZE; ++i) LC->m_keywordHashes[i] = 0;
    LC->m_keywords = 0;
    LC->m_keywordLengths = 0;
    stringTableInit(&LC->m_nameStore, K_KB(4), 128);
    LC->m_operators = 0;
}

void lexConfigDone(LexConfig* LC)
{
    arrayDone(LC->m_keywords);
    arrayDone(LC->m_keywordLengths);
    arrayDone(LC->m_operators);
    stringTableDone(&LC->m_nameStore);
}

void lexConfigInitComments(LexConfig* LC, const i8* lineComment, const i8* blockStartComment, const i8* blockEndComment)
{
    // Check sizes
    K_ASSERT(lineComment[0] != 0 && lineComment[1] != 0 && lineComment[2] == 0);
    K_ASSERT(blockStartComment[0] != 0 && blockStartComment[1] != 0 && blockStartComment[2] == 0);
    K_ASSERT(blockEndComment[0] != 0 && blockEndComment[1] != 0 && blockEndComment[2] == 0);

    // Check the comment rules
    K_ASSERT(lineComment[0] == blockStartComment[0] && lineComment[0] == blockEndComment[1]);
    K_ASSERT(blockStartComment[1] == blockEndComment[0]);

    LC->m_comment0 = lineComment[0];
    LC->m_commentLine = lineComment[1];
    LC->m_commentBlock = blockStartComment[1];
}

void lexConfigAddNameCharsRange(LexConfig* LC, LexNameCharType type, char start, char end)
{
    for (char i = start; i <= end; ++i)
    {
        LC->m_nameChars[i] = (u8)type;
    }
}

void lexConfigAddNameCharsString(LexConfig* LC, LexNameCharType type, const i8* str)
{
    for (int i = 0; str[i] != 0; ++i)
    {
        LC->m_nameChars[str[i]] = (u8)type;
    }
}

Token lexConfigAddOperator(LexConfig* LC, const i8* operator)
{
    StringToken op = stringTableAdd(&LC->m_nameStore, operator);
    String opStr = stringTableGet(&LC->m_nameStore, op);
    i64 index = arrayCount(LC->m_operators);
    arrayAdd(LC->m_operators, op);
    return (int)(index + PARSER_OPERATOR_INDEX);
}

Token lexConfigAddKeyword(LexConfig* LC, const i8* keyword)
{
    StringToken kw = stringTableAdd(&LC->m_nameStore, keyword);
    String kwStr = stringTableGet(&LC->m_nameStore, kw);
    i64 len = stringLength(kwStr);

    i64 keywordIndex = arrayCount(LC->m_keywords);
    K_ASSERT(keywordIndex < 256);
    arrayAdd(LC->m_keywords, kw);
    arrayAdd(LC->m_keywordLengths, len);

    // Check to see if we've not run out of room.
    u64 index = stringHash(kwStr) & (PARSER_KEYWORD_HASHTABLE_SIZE-1);
    K_ASSERT((LC->m_keywordHashes[index] & 0xff00000000000000) == 0);

    LC->m_keywordHashes[index] <<= 8;
    LC->m_keywordHashes[index] += keywordIndex;

    return (int)(keywordIndex + PARSER_KEYWORD_INDEX);
}

//----------------------------------------------------------------------------------------------------------------------
// Lexical Analysis API
//----------------------------------------------------------------------------------------------------------------------

void lexDone(Lex* L)
{
    arrayDone(L->m_info);
    arenaDone(&L->m_scratch);
}

internal char lexNextChar(Lex* L)
{
    char c;
    L->m_lastPosition = L->m_position;
    L->m_lastCursor = L->m_cursor;
    if (L->m_cursor == L->m_end) return 0;

    c = *L->m_cursor++;
    ++L->m_position.m_col;

    if ('\r' == c || '\n' == c)
    {
        ++L->m_position.m_line;
        L->m_position.m_col = 1;
        if (c == '\r')
        {
            if ((L->m_cursor < L->m_end) && (*L->m_cursor == '\n'))
            {
                ++L->m_cursor;
            }
            c = '\n';
        }
        L->m_position.m_lineOffset = L->m_cursor - L->m_start;
    }

    return c;

}

internal void lexUngetChar(Lex* L)
{
    L->m_position = L->m_lastPosition;
    L->m_cursor = L->m_lastCursor;
}

internal Token lexErrorV(Lex* L, const i8* format, va_list args)
{
    String msg;
    arenaPush(&L->m_scratch);
    msg = arenaStringFormatV(&L->m_scratch, format, args);
    L->m_outputFunc(arenaStringFormat(&L->m_scratch, "%s(%d): Lexical Error: %s\n", L->m_source, L->m_lastPosition.m_line, msg));

    {
        int x = L->m_lastPosition.m_col - 1;
        const i8* lineStart = L->m_start + L->m_lastPosition.m_lineOffset;
        const i8* fileEnd = L->m_end;

        // Print line that token resides in
        char c[2] = { 0 };
        for (const i8* p = lineStart; (p < fileEnd) && (*p != '\r') && (*p != '\n'); ++p)
        {
            c[0] = *p;
            L->m_outputFunc(c);
        }

        L->m_outputFunc("\n");
        for (int j = 0; j < x; ++j) L->m_outputFunc(" ");
        L->m_outputFunc("^\n");
    }

    arenaPop(&L->m_scratch);
    return T_Error;

}

internal Token lexError(Lex* L, const i8* format, ...)
{
    va_list args;
    va_start(args, format);
    lexErrorV(L, format, args);
    va_end(args);
    return T_Error;
}

internal Token lexBuild(LexInfo* li, Token token, LexPos pos, i64 number, StringToken symbol)
{
    li->m_token = token;
    li->m_position = pos;
    li->m_integer = number;
    li->m_symbol = symbol;
    return token;
}

internal Token lexNext(Lex* L)
{
    // Find the next meaningful character, skipping whitespace and comments.  Comments are delimited by
    // COMMENT0 followed by COMMENT_LINE until the end of the line, or between the nested comment start
    // (COMMENT0 followed by COMMENT_BLOCK), and the nested comment end (COMMENT_BLOCK followed by a
    // COMMENT0).

    char c = lexNextChar(L);

    for (;;)
    {
        if (0 == c) return T_EOF;

        if ((c == '\n' && !L->m_config.m_trackNewLines) || (c != '\n' && iswspace(c)))
        {
            c = lexNextChar(L);
            continue;
        }

        // Check for comments
        if (L->m_config.m_comment0 == c)
        {
            LexPos prevPos = L->m_lastPosition;
            const i8* prevCursor = L->m_lastCursor;
            c = lexNextChar(L);
            if (L->m_config.m_commentBlock == c)
            {
                // Nestable multi-line comment
                int depth = 1;
                while (c != 0 && depth)
                {
                    c = lexNextChar(L);
                    if (L->m_config.m_comment0 == c)
                    {
                        // Check for next nested block comment
                        c = lexNextChar(L);
                        if (L->m_config.m_commentBlock == c)
                        {
                            if (++depth == 256)
                            {
                                return lexError(L, "Comments nested too deep.");
                            }
                        }
                    }
                    else if (L->m_config.m_commentBlock == c)
                    {
                        // Check for end of nested block comment
                        c = lexNextChar(L);
                        if (L->m_config.m_comment0 == c)
                        {
                            --depth;
                        }
                    }
                }
            }
            else if (L->m_config.m_commentLine == c)
            {
                // Line based comment
                while (c != 0 && c != '\n') c = lexNextChar(L);
                c = lexNextChar(L);
                continue;
            }
            else
            {
                // Actual possible operator
                L->m_position = prevPos;
                L->m_cursor = prevCursor;
                c = lexNextChar(L);
            }
        }

        // If we've reached this point, we have a meaningful character.
        break;
    }

    {
        LexInfo* li = arrayExpand(L->m_info, 1);
        LexPos pos = L->m_lastPosition;

        li->m_s0 = L->m_cursor - 1;
        li->m_s1 = L->m_cursor;

        //--------------------------------------------------------------------------------------------------------------
        // Check for newlines
        //--------------------------------------------------------------------------------------------------------------

        if (c == '\n')
        {
            return lexBuild(li, T_NewLine, pos, 0, 0);
        }

        //--------------------------------------------------------------------------------------------------------------
        // Check for symbols and keywords
        //--------------------------------------------------------------------------------------------------------------

        else if (L->m_config.m_nameChars[c] == LNCT_Valid)
        {
            i64 sizeToken = 0;
            u64 h = 0;
            u64 tokens = 0;

            while (L->m_config.m_nameChars[c]) c = lexNextChar(L);
            lexUngetChar(L);

            li->m_s1 = L->m_cursor;
            sizeToken = (i64)(li->m_s1 - li->m_s0);
            h = hash(li->m_s0, sizeToken);

            // Determine if it is a keyword or a symbol
            tokens = L->m_config.m_keywordHashes[h & (PARSER_KEYWORD_HASHTABLE_SIZE-1)];
            while (tokens != 0)
            {
                int index = (tokens & 0xff);
                tokens >>= 8;

                if (L->m_config.m_keywordLengths[index] == sizeToken)
                {
                    // The length of the tokens match with the keywords we're currently checking against.
                    if (memoryCompare(li->m_s0, stringTableGet(&L->m_config.m_nameStore, L->m_config.m_keywords[index]),
                        sizeToken) == 0)
                    {
                        // It is a keyword
                        return lexBuild(li, (Token)(PARSER_KEYWORD_INDEX + index), pos, 0, 0);
                    }
                }
            }

            // It's a symbol
            return lexBuild(li, T_Symbol, pos, 0, stringTableAddRange(L->m_symbols, li->m_s0, li->m_s1));
        }

        //--------------------------------------------------------------------------------------------------------------
        // Check for numbers
        //--------------------------------------------------------------------------------------------------------------

        else if (// Check for digit
            (c >= '0' && c <= '9') ||
            // Check for integer starting with '-' or '+'
            (('-' == c || '+' == c) && (L->m_cursor < L->m_end) &&
                ((*L->m_cursor >= '0' && *L->m_cursor <= '9') || *L->m_cursor == '.')) ||
            // Check for number starting with '.'
            (('.' == c) && (L->m_cursor < L->m_end) && ((*L->m_cursor >= '0' && *L->m_cursor <= '9'))))
        {
            int state = 0;
            bool isFloat = NO;
            const i8* floatStart = L->m_cursor - 1;

            i64 sign = 1;
            i64 exponent = 0;
            i64 intPart = 0;
            i64 base = 10;

            i8 floatBuffer[32];
            i8* floatText = floatBuffer;
            Token result;

            // Each state always fetches the next character for the next state
            for (;;)
            {
                switch (state)
                {
                case 0:     // START
                    if ('-' == c || '+' == c) state = 1;
                    else if ('.' == c) state = 2;
                    else state = 3;
                    break;

                case 1:     // +/-
                    if ('-' == c) sign = -1;
                    c = lexNextChar(L);
                    state = 3;
                    break;

                case 2:     // .
                    isFloat = YES;
                    state = 4;
                    c = lexNextChar(L);
                    break;

                case 3:     // Decide if we have 0 (hex or octal), 1-9 (decimal), or '.' (float)
                    if ('.' == c) state = 2;
                    else if ('0' == c) state = 5;
                    else if (c >= '1' && c <= '9') state = 6;
                    else goto bad;
                    break;

                case 4:     // Digits 0-9 in float part
                {
                    while ((c >= '0') && (c <= '9'))
                    {
                        c = lexNextChar(L);
                    }
                    if ('e' == c || 'E' == c) state = 7;
                    else state = 100;
                }
                break;

                case 5:     // '0' - decide whether we are octal or hexadecimal
                    c = lexNextChar(L);
                    if ('x' == c || 'X' == c)
                    {
                        c = lexNextChar(L);
                        base = 16;
                    }
                    else if ('.' == c)
                    {
                        state = 2;
                        break;
                    }
                    else if ((c >= '0') || (c <= '9'))
                    {
                        base = 8;
                    }
                    else
                    {
                        // Value is 0
                        li->m_s1 = L->m_cursor;
                        lexBuild(li, T_Integer, pos, 0, 0);
                        state = 100;
                        break;
                    }

                    state = 8;
                    break;

                case 6:     // Integer digits
                    while ((c >= '0') && (c <= '9'))
                    {
                        i64 last = intPart;
                        intPart *= 10;
                        intPart += (c - '0');
                        if (intPart < last) goto overflow;
                        c = lexNextChar(L);
                    }
                    if ('.' == c) state = 2;
                    else if ('e' == c || 'E' == c) state = 7;
                    else state = 100;
                    break;

                case 7:     // Exponent part
                    c = lexNextChar(L);
                    if ('-' == c || '+' == c) state = 9;
                    else if ((c >= '0') && (c <= '9')) state = 10;
                    else goto bad;
                    break;

                case 8:     // Non-decimal integer digits
                {
                    i64 last;
                    i64 x = ((c >= '0') && (c <= '9'))
                        ? (c - '0')
                        : ((c >= 'a') && (c <= 'f'))
                        ? (c - 'a' + 10)
                        : ((c >= 'A') && (c <= 'F'))
                        ? (c - 'A' + 10)
                        : -1;
                    if ((-1 == x) || (x >= base)) state = 100;
                    else
                    {
                        last = intPart;
                        intPart *= base;
                        intPart += x;
                        if (intPart < last) goto overflow;
                        c = lexNextChar(L);
                    }
                }
                break;

                case 9:     // Exponent sign
                    if ('-' == c) exponent = -exponent;
                    c = lexNextChar(L);
                    state = 10;
                    break;

                case 10:    // Exponent digits
                    while ((c >= '0') && (c <= '9'))
                    {
                        i64 last = exponent;
                        exponent *= 10;
                        exponent += (c - '0');
                        if (exponent < last) goto overflow;
                        c = lexNextChar(L);
                    }
                    state = 100;
                    break;

                case 100:   // End of number (possibly)
                    li->m_s1 = L->m_lastCursor;
                    if (isFloat)
                    {
                        i64 len = (i64)(L->m_cursor - floatStart - 1);
                        f64 f;

                        lexUngetChar(L);
                        if (len > 31)
                        {
                            floatText = (i8*)K_ALLOC(len + 1);
                        }
                        memoryCopy(floatStart, floatText, len);
                        floatText[len] = 0;

                        f = strtod(floatText, 0);
                        result = lexBuild(li, T_Real, pos, *(i64 *)&f, 0);

                        if (floatText != floatBuffer)
                        {
                            K_FREE(floatText, len + 1);
                        }
                        goto finished;
                    }
                    else
                    {
                        // Integrate the exponent and sign if any
                        if (exponent < 0) goto bad;
                        else if (exponent > 0)
                        {
                            for (; exponent != 0; --exponent)
                            {
                                i64 last = intPart;
                                intPart *= 10;
                                if (intPart < last) goto overflow;
                            }
                        }
                        intPart *= sign;

                        result = lexBuild(li, T_Integer, pos, intPart, 0);
                        goto finished;
                    }
                }
            } // for(;;) state machine

        finished:
            lexUngetChar(L);
            return result;

        bad:
            return lexError(L, "Invalid number found.");

        overflow:
            return lexError(L, "Overflow detected in number.  Number is too big.");

        } // if (digit...)

        //--------------------------------------------------------------------------------------------------------------
        // Check for operators
        //--------------------------------------------------------------------------------------------------------------

        for (i64 i = 0; i < arrayCount(L->m_config.m_operators); ++i)
        {
            String op = stringTableGet(&L->m_config.m_nameStore, L->m_config.m_operators[i]);
            const i8* opScan = op;
            const i8* srcScan = li->m_s0;

            while (
                // Check to see if the characters match
                (*opScan == *srcScan) &&
                // Not at the end of the operator
                (*opScan != 0) &&
                // Not at the end of the source
                (srcScan < L->m_end))
            {
                ++opScan;
                ++srcScan;
            }
            if (*opScan == 0)
            {
                // We've matched
                li->m_s1 = li->m_s0 + (opScan - op);
                while (L->m_cursor < li->m_s1)
                {
                    lexNextChar(L);
                }
                return lexBuild(li, PARSER_OPERATOR_INDEX + i, pos, 0, 0);
            }
        }

        //
        // Unknown token
        //

        lexBuild(li, T_Unknown, pos, 0, 0);
        return lexError(L, "Unknown token");
    }

}

void lex(Lex* L, LexConfig* config, LexOutputFunc outputFunc, StringTable* symbols, String source, const i8* start, const i8* end)
{
    // Initialise source
    L->m_source = source;
    L->m_config = *config;
    L->m_outputFunc = outputFunc;
    arenaInit(&L->m_scratch, 128);
    L->m_symbols = symbols;
    L->m_start = start;
    L->m_end = end;
    L->m_info = 0;
    L->m_cursor = L->m_start;
    L->m_lastCursor = 0;
    L->m_position.m_lineOffset = 0;
    L->m_position.m_line = 1;
    L->m_position.m_col = 1;
    L->m_lastPosition = L->m_position;

    // Analyse!
    Token t = T_Unknown;
    while ((t = lexNext(L)) != T_EOF && t != T_Error)
    {

    }
}

void lexDump(Lex* L)
{
    const i8* typeNames[] = {
        "UNKNOWN",
        "ERROR",
        "EOF",
        "NEWLINE",
        "SYMBOL",
        "INTEGER",
        "REAL",
    };
    Arena scratch;
    arenaInit(&scratch, K_KB(1));

    for (i64 i = 0; i < arrayCount(L->m_info); ++i)
    {
        LexInfo* li = &L->m_info[i];
        arenaPush(&scratch);

        // Print token information
        const i8* name = "";
        const i8* prefix = "";
        if (li->m_token >= PARSER_OPERATOR_INDEX)
        {
            prefix = "(OPERATOR) ";
            name = stringTableGet(&L->m_config.m_nameStore, L->m_config.m_operators[li->m_token - PARSER_OPERATOR_INDEX]);
        }
        else if (li->m_token >= PARSER_KEYWORD_INDEX)
        {
            prefix = "(KEYWORD) ";
            name = stringTableGet(&L->m_config.m_nameStore, L->m_config.m_keywords[li->m_token - PARSER_KEYWORD_INDEX]);
        }
        else
        {
            name = typeNames[li->m_token];
        }
        L->m_outputFunc(arenaStringFormat(&scratch, "%d: %s%s", L->m_info[i].m_position.m_line, prefix, name));

        // Print interpretation of token
        switch (li->m_token)
        {
        case T_Symbol:
            L->m_outputFunc(arenaStringFormat(&scratch, ": %s", stringTableGet(L->m_symbols, li->m_symbol)));
            break;

        case T_Integer:
            L->m_outputFunc(arenaStringFormat(&scratch, ": %lld", li->m_integer));
            break;

        case T_Real:
            L->m_outputFunc(arenaStringFormat(&scratch, ": %f", li->m_real));
            break;
        }

        L->m_outputFunc("\n");
        if (li->m_token > T_EOF)
        {
            int x = li->m_position.m_col - 1;
            int len = (int)(li->m_s1 - li->m_s0);

            // Print line that the token resides in
            for (const i8* p = (L->m_start + li->m_position.m_lineOffset);
                (p < L->m_end) && (*p != '\r') && (*p != '\n');
                ++p)
            {
                char c[2] = { 0 };
                c[0] = *p;
                L->m_outputFunc(c);
            }

            L->m_outputFunc("\n");
            for (int j = 0; j < x; ++j) L->m_outputFunc(" ");
            L->m_outputFunc("^");
            for (int j = 0; j < len - 1; ++j) L->m_outputFunc("~");
            L->m_outputFunc("\n");
        }
        arenaPop(&scratch);
    }

    arenaDone(&scratch);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // K_IMPLEMENTATION
