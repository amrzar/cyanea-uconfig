#!/bin/bash

export VERSION_CONTROL=none

[[ $# -ne 1 ]] && {
    echo "${0##*/}: input file is missing."
    exit 1
}

#
# These are subset of options that matches the '--linux-style'.
#
# -nbad, --no-blank-lines-after-declarations.
# -bap,  --blank-lines-after-procedures.
# -nbc,  --no-blank-lines-after-commas.
# -nbbo, --break-after-boolean-operator.
# -br,   --braces-on-if-line.
# -brs,  --braces-on-struct-decl-line.
# -cn,   --comment-indentationn.
# -cdn,  --declaration-comment-columnn.
# -ncdb, --no-comment-delimiters-on-blank-lines.
# -ce,   --cuddle-else.
# -cin,  --continuation-indentationn.
# -clin, --case-indentationn.
# -dn,   --line-comments-indentationn.
# -din,  --declaration-indentationn.
# -nfc1, --dont-format-first-column-comments.
# -in,   --indent-leveln.
# -ipn,  --parameter-indentationn.
# -ln,   --line-lengthn.
# -nlp,  --dont-line-up-parentheses.
# -npcs, --no-space-after-function-call-names.
# -nprs, --no-space-after-parentheses.
# -npsl, --dont-break-procedure-type.
# -sai,  --space-after-if.
# -saf,  --space-after-for.
# -saw,  --space-after-while.
# -ncs,  --no-space-after-casts.
# -nsc,  --dont-star-comments.
# -nut,  --no-tabs.
# -sob,  --swallow-optional-blank-lines.
# -nfca, --dont-format-comments.
# -ss,   --space-special-semicolon.
# -iln,  --indent-labeln.
#
#   See https://linux.die.net/man/1/indent
#

indent -nbad -bap -nbc -nbbo -br -brs -c33 -cd33 -ncdb -ce -ci4 \
    -cli0 -d0 -di1 -nfc1 -i4 -ip0 -l80 -nlp -npcs -nprs -npsl -sai \
    -saf -saw -ncs -nsc -nut -sob -nfca -ss -il1 "${1}" -o "${1}.tmp"

cmp -s "${1}" "${1}.tmp"
[[ $? -eq 0 ]] || {
    echo "Formatted: ${1}"
    mv "${1}.tmp" "${1}"
}

rm -f "${1}.tmp"
