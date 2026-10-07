# TestMd3SourceEncoding
#
# Gate: every source file is valid UTF-8, and nothing decodes a UTF-8 literal as
# Latin-1.
#
# Why this exists
# ---------------
# A UTF-8 em dash is three bytes (e2 80 94). Those bytes are correct in the
# file, but feeding them to `QString::fromLatin1` yields "â€”" on screen. The
# source still looks right in an editor, so the failure is invisible in review
# and only appears in a rendered screenshot. That exact bug shipped once in the
# gallery example, so it is a build gate now rather than something to remember.
#
# Rule 1 — files are valid UTF-8.
#   A file saved as cp1252 breaks the compiler's assumption about the input
#   charset and corrupts string literals the other way round. CMake cannot tell
#   you this by round-tripping the content: `file(READ)` hands the bytes back
#   unchanged whatever they are, and `file(STRINGS)` quietly substitutes a
#   replacement character for anything invalid, which hides the evidence. So the
#   check below is a real UTF-8 structure validator working on the raw bytes.
#
# Rule 2 — Latin-1 *decoding* is banned in src/ and examples/.
#   `QString::fromLatin1` and `QString::fromLocal8Bit` turn bytes into a QString,
#   which is how a UTF-8 literal becomes mojibake. For an ASCII input they
#   behave exactly like `QString::fromUtf8`, so banning them costs nothing.
#   Banning the API rather than trying to detect misuse is deliberate: in the bug
#   that motivated this gate the literal lived in a table on one line and the
#   `fromLatin1` call sat forty lines away, so no per-line text scan can see it.
#
# Rule 3 — `QLatin1String(` must be on a pure-ASCII line.
#   `QLatin1String` is a comparison/view type, not a decoder, so it stays
#   allowed — `MdTokens` legitimately compares against ASCII token names from a
#   table. What it must never wrap is a non-ASCII literal, because that compares
#   or renders the wrong characters. Not covered: `QLatin1String(variable)`
#   where the variable holds non-ASCII bytes. That is a different bug class
#   (silent comparison failure rather than mojibake) and no such table exists.
#
#   If a real Latin-1 byte source ever turns up (a binary file format, say),
#   add that one file to QT_MD3_LATIN1_ALLOWED rather than weakening the rule.
#
# Run as:
#   cmake -DSRC_ROOT=<repo root> -P tests/TestMd3SourceEncoding.cmake

if(NOT DEFINED SRC_ROOT)
    message(FATAL_ERROR "TestMd3SourceEncoding: SRC_ROOT is not defined")
endif()

set(_scan_dirs src examples tests)
set(_extensions h hpp hh hxx c cc cpp cxx)

# Files permitted to decode Latin-1 bytes, relative to SRC_ROOT. Expected empty.
set(QT_MD3_LATIN1_ALLOWED "")

# `QLatin1Char` is deliberately absent: it takes a single `char`, so a
# multi-byte literal cannot reach it, and it is the idiomatic spelling for
# character-level work such as QDir filters.
set(_latin1_decoders
    "QString::fromLatin1("
    "QString::fromLocal8Bit("
)

# Checked only for non-ASCII on the same line; see rule 3 above.
set(_latin1_view "QLatin1String(")

# --- Byte-level helpers ----------------------------------------------------
#
# CMake has no byte access, so values are examined through their hex encoding.
#
# Two constraints shape everything below, both learned the hard way.
#
# Alignment. A byte is ASCII exactly when its high nibble is 0-7 (below 0x80).
# Matching a two-character pattern against the hex string is only safe if the
# scan can never resume at an odd offset, because an odd offset means a match
# straddling two bytes. The `ascii` check is anchored so alignment is
# structural; the byte list uses `..`, which matches everywhere and therefore
# also never lands on an odd offset.
#
# Cost. Feeding a 70 KB hex string to an alternation-with-repetition pattern
# makes CMake's regex engine collapse — a 35 KB source file killed the process
# outright. So the UTF-8 grammar is applied to a short pre-filtered list of the
# bytes that actually need checking, never to the whole file. Filtering with
# `MATCHES` over an aligned list is fast; doing the same walk with
# `list(GET)`/`math(EXPR)` per byte took minutes across the tree.

# True when the value is pure ASCII. Equivalent to "valid UTF-8" for ASCII.
function(_md3_is_ascii value out_var)
    string(HEX "${value}" _hex)
    if(_hex MATCHES "^([0-7][0-9a-fA-F])*$")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Validates the UTF-8 structure of a value. Sets <out_var> to an empty string
# when valid, otherwise to a short description of the first fault, with the
# byte numbered among the non-ASCII ones so the report is actionable.
function(_md3_utf8_problem value out_var)
    string(HEX "${value}" _hex)
    string(REGEX MATCHALL ".." _pairs "${_hex}")

    set(_bytes "")
    foreach(_pair IN LISTS _pairs)
        if(_pair MATCHES "^[89a-fA-F]")
            list(APPEND _bytes "${_pair}")
        endif()
    endforeach()
    list(LENGTH _bytes _count)
    if(_count EQUAL 0)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_index 0)
    set(_problem "")
    while(_index LESS _count)
        list(GET _bytes ${_index} _lead_hex)
        string(TOUPPER "${_lead_hex}" _lead_hex)
        math(EXPR _lead "0x${_lead_hex}")

        if(_lead GREATER 193 AND _lead LESS 224)
            set(_need 2) # C2..DF starts a two-byte sequence
        elseif(_lead GREATER 223 AND _lead LESS 240)
            set(_need 3) # E0..EF starts a three-byte sequence
        elseif(_lead GREATER 239 AND _lead LESS 245)
            set(_need 4) # F0..F4 starts a four-byte sequence
        else()
            # 0x80..0xC1 and 0xF5..0xFF can never begin a character. This is the
            # signature of a file saved in a single-byte encoding: 0x97, 0x92,
            # 0x93 and friends land here.
            set(_problem "non-ASCII byte #${_index} is 0x${_lead_hex}, which cannot begin a UTF-8 sequence")
            set(_index ${_count})
            continue()
        endif()

        math(EXPR _last "${_index} + ${_need}")
        if(_last GREATER _count)
            set(_problem "truncated UTF-8 sequence: 0x${_lead_hex} at non-ASCII byte #${_index} needs ${_need} bytes")
            set(_index ${_count})
            continue()
        endif()

        math(EXPR _next "${_index} + 1")
        while(_next LESS _last)
            list(GET _bytes ${_next} _cont_hex)
            string(TOUPPER "${_cont_hex}" _cont_hex)
            math(EXPR _cont "0x${_cont_hex}")
            if(_cont LESS 128 OR _cont GREATER 191)
                set(_problem "byte 0x${_cont_hex} at non-ASCII byte #${_next} does not continue the sequence started by 0x${_lead_hex}")
                set(_index ${_count})
                continue()
            endif()
            math(EXPR _next "${_next} + 1")
        endwhile()
        if(NOT _problem STREQUAL "")
            continue()
        endif()

        math(EXPR _index "${_index} + ${_need}")
    endwhile()

    set(${out_var} "${_problem}" PARENT_SCOPE)
endfunction()

# --- Scan ------------------------------------------------------------------

set(_scanned 0)
set(_pure_ascii 0)
set(_encoding_violations "")
set(_latin1_violations "")

foreach(_dir IN LISTS _scan_dirs)
    set(_root "${SRC_ROOT}/${_dir}")
    if(NOT EXISTS "${_root}")
        continue()
    endif()
    foreach(_ext IN LISTS _extensions)
        file(GLOB_RECURSE _files "${_root}/*.${_ext}")
        foreach(_file IN LISTS _files)
            math(EXPR _scanned "${_scanned} + 1")
            file(RELATIVE_PATH _rel "${SRC_ROOT}" "${_file}")

            # --- Rule 1: the file must be valid UTF-8. Uses file(READ), which
            # preserves the raw bytes; file(STRINGS) is not used for this
            # because it silently drops bytes it cannot decode, which would hide
            # exactly the fault being looked for.
            file(READ "${_file}" _content)
            _md3_is_ascii("${_content}" _is_ascii)
            if(_is_ascii)
                math(EXPR _pure_ascii "${_pure_ascii} + 1")
            else()
                _md3_utf8_problem("${_content}" _problem)
                if(NOT _problem STREQUAL "")
                    list(APPEND _encoding_violations "${_rel}  ->  ${_problem}")
                endif()
            endif()

            # --- Rules 2 and 3: per line, as text.
            list(FIND QT_MD3_LATIN1_ALLOWED "${_rel}" _allowed)
            if(NOT _allowed EQUAL -1)
                continue()
            endif()
            file(STRINGS "${_file}" _lines)
            set(_lineno 0)
            foreach(_line IN LISTS _lines)
                math(EXPR _lineno "${_lineno} + 1")
                string(STRIP "${_line}" _stripped)
                if(_stripped MATCHES "^//")
                    continue()
                endif()

                foreach(_api IN LISTS _latin1_decoders)
                    string(FIND "${_line}" "${_api}" _pos)
                    if(NOT _pos EQUAL -1)
                        list(APPEND _latin1_violations "${_rel}:${_lineno}  ->  '${_api}'")
                    endif()
                endforeach()

                string(FIND "${_line}" "${_latin1_view}" _view_pos)
                if(NOT _view_pos EQUAL -1)
                    _md3_is_ascii("${_line}" _line_is_ascii)
                    if(NOT _line_is_ascii)
                        list(APPEND _latin1_violations
                             "${_rel}:${_lineno}  ->  '${_latin1_view}' on a non-ASCII line")
                    endif()
                endif()
            endforeach()
        endforeach()
    endforeach()
endforeach()

# --- Report ----------------------------------------------------------------

if(_encoding_violations)
    list(REMOVE_DUPLICATES _encoding_violations)
    string(REPLACE ";" "\n  - " _report "${_encoding_violations}")
    message(FATAL_ERROR
        "TestMd3SourceEncoding: file is not valid UTF-8.\n"
        "Re-save it as UTF-8 (no BOM needed; CMake and GCC both assume UTF-8).\n"
        "Offending files:\n  - ${_report}"
    )
endif()
if(_latin1_violations)
    list(REMOVE_DUPLICATES _latin1_violations)
    string(REPLACE ";" "\n  - " _report "${_latin1_violations}")
    message(FATAL_ERROR
        "TestMd3SourceEncoding: Latin-1 decoding detected.\n"
        "The source file is UTF-8, so use QString::fromUtf8() to build a string\n"
        "from bytes, QStringLiteral() for literals, and QLatin1Char() for single\n"
        "characters. QLatin1String() is fine only where the line is pure ASCII.\n"
        "Offending lines:\n  - ${_report}"
    )
endif()

message(STATUS "TestMd3SourceEncoding: scanned ${_scanned} file(s) "
               "(${_pure_ascii} pure ASCII), all UTF-8, no Latin-1 decoding.")
