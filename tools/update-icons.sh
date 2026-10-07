#!/usr/bin/env bash
#
# Regenerates the bundled icon assets under resources/icons/.
#
# What it fetches, and from where:
#
#   icons/material-symbols-codepoints.txt
#       google/material-design-icons
#       variablefont/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].codepoints
#       The complete 4299-entry name -> codepoint table. Identical for all three
#       families. Committed.
#
#   icons/classic/24px/<name>.svg
#       google/material-design-icons
#       src/<category>/<name>/materialicons/24px.svg
#       The curated classic baseline in $CLASSIC_ICONS below. Committed.
#
# Pass --fonts to also vendored-download the Material Symbols variable fonts
# into resources/fonts/. Those are 3.5-15 MB each and are NOT committed by
# default; the script prints their size and asks before writing anything.
#
# Both fetches are Apache-2.0. See docs/resources-manifest.md and NOTICE.md.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ICON_DIR="${REPO_ROOT}/resources/icons"
CLASSIC_DIR="${ICON_DIR}/classic/24px"
FONT_DIR="${REPO_ROOT}/resources/fonts"

META_REPO="google/material-design-icons"

# name:category pairs. The category is the directory under src/ — it is not
# always the obvious one (edit lives under image/, mail under content/), so the
# mapping is spelled out rather than guessed.
CLASSIC_ICONS=""
add_icon() { CLASSIC_ICONS="${CLASSIC_ICONS}$1:$2"$'\n'; }

add_icon home action
add_icon menu navigation
add_icon search action
add_icon settings action
add_icon favorite action
add_icon add content
add_icon close navigation
add_icon check navigation
add_icon delete action
add_icon edit image
add_icon share social
add_icon star toggle
add_icon arrow_back navigation
add_icon arrow_forward navigation
add_icon expand_more navigation
add_icon expand_less navigation
add_icon more_vert navigation
add_icon notifications social
add_icon person social
add_icon info action
add_icon warning alert
add_icon error alert
add_icon palette image
add_icon text_fields editor
add_icon straighten image
add_icon animation image
add_icon layers maps
add_icon apps navigation
add_icon refresh navigation
add_icon download file
add_icon content_copy content
add_icon visibility action
add_icon lock action
add_icon mail content
add_icon calendar_today action
add_icon play_arrow av
add_icon pause av
add_icon volume_up av
add_icon wifi notification
add_icon bluetooth device
add_icon location_on communication
add_icon phone communication
add_icon chat communication
add_icon shopping_cart action
add_icon dashboard action
add_icon list action
add_icon color_lens image
add_icon dark_mode device
add_icon light_mode device

# --- fetch helpers ----------------------------------------------------------

# Prefer curl; fall back to the GitHub CLI when a corporate proxy breaks
# TLS for curl (the reason this fallback exists at all).
fetch() {
    local path="$1" out="$2"
    if command -v gh >/dev/null 2>&1 && [ -n "${USE_GH:-}" ]; then
        gh api "repos/${META_REPO}/contents/${path}" --jq '.content' | base64 -d >"$out"
    elif curl -fsSL \
        "https://raw.githubusercontent.com/${META_REPO}/master/${path}" >"$out" 2>/dev/null; then
        :
    elif command -v gh >/dev/null 2>&1; then
        gh api "repos/${META_REPO}/contents/${path}" --jq '.content' | base64 -d >"$out"
    else
        return 1
    fi
}

fetch_icon() {
    local name="$1" category="$2"
    local out="${CLASSIC_DIR}/${name}.svg"
    local tmp="${out}.tmp"
    if fetch "src/${category}/${name}/materialicons/24px.svg" "$tmp" \
        && [ -s "$tmp" ] && head -c 4 "$tmp" | grep -q '<svg'; then
        mv "$tmp" "$out"
        return 0
    fi
    rm -f "$tmp"
    echo "  ! ${name} (src/${category}/${name}) not found" >&2
    return 1
}

# --- codepoints -------------------------------------------------------------

fetch_codepoints() {
    local out="${ICON_DIR}/material-symbols-codepoints.txt"
    echo "==> ${out#${REPO_ROOT}/}"
    mkdir -p "${ICON_DIR}"
    fetch "variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.codepoints" "${out}.tmp"
    mv "${out}.tmp" "${out}"
    echo "    $(grep -c . "${out}") entries"
}

# --- classic SVGs -----------------------------------------------------------

fetch_classic() {
    echo "==> ${CLASSIC_DIR#${REPO_ROOT}/}/"
    mkdir -p "${CLASSIC_DIR}"
    local failed=0
    while IFS=: read -r name category; do
        [ -n "${name}" ] || continue
        fetch_icon "${name}" "${category}" || failed=$((failed + 1))
    done <<<"${CLASSIC_ICONS}"
    echo "    $(ls -1 "${CLASSIC_DIR}"/*.svg | wc -l) icons, ${failed} failed"
    [ "${failed}" -eq 0 ]
}

# --- optional fonts ---------------------------------------------------------

fetch_fonts() {
    echo "==> Material Symbols variable fonts"
    echo "    Outlined / Rounded / Sharp, woff2. Roughly 13 MB in total."
    read -r -p "    Download into resources/fonts/? [y/N] " reply
    case "${reply}" in
    [yY]*) ;;
    *)
        echo "    skipped"
        return 0
        ;;
    esac
    mkdir -p "${FONT_DIR}"
    for family in Outlined Rounded Sharp; do
        local name="MaterialSymbols${family}[FILL,GRAD,opsz,wght].woff2"
        local encoded
        encoded="$(printf '%s' "${name}" | sed 's/\[/%5B/; s/\]/%5D/; s/,/%2C/g')"
        echo "    ${name}"
        fetch "variablefont/${encoded}" "${FONT_DIR}/${name}"
    done
    echo "    Add the files to resources/qt-md3.qrc under the fonts/ prefix, then"
    echo "    MdFont::registerBundledFonts() will pick them up."
    echo "    NOTE: these are large. Do not commit them unless you mean to."
}

# --- main -------------------------------------------------------------------

fetch_codepoints
fetch_classic
if [ "${1:-}" = "--fonts" ]; then
    fetch_fonts
fi

echo
echo "Now regenerate resources/qt-md3.qrc if the icon list changed, and update"
echo "docs/resources-manifest.md."
