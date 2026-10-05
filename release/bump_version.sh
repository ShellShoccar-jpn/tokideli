#!/bin/sh

######################################################################
#
# BUMP_VERSION.SH - Synchronize the Project-Wide "Version" Line Across
#                   All c_src/*.c and cmd_scripts/*.sh Files
#
# *** This is a maintainer-only tool. ***
# It is used by whoever is cutting a new release of this project, to
# update the single "Version : X.Y.Z" line that is duplicated across
# every command's "-h"/usage banner. If you just want to USE one of
# the commands in this repository (charts, linets, tscat, ...), you
# do not need this script at all.
#
# USAGE   : release/bump_version.sh <new-version>
# Args    : <new-version> ... The new semantic version, e.g. "1.1.0"
#                              (must be of the form X.Y.Z)
# Output  : Updates the root "VERSION" file, and the "Version" line
#           (NOT the "Last Updated" line) inside every c_src/*.c and
#           cmd_scripts/*.sh file, to the given version.
# Retuen  : Return 0 only when every file was updated successfully
#
# Note: this script deliberately never touches any "Last Updated"
# line. Bumping the project-wide version number does not by itself
# mean that a given file's own logic changed, so pretending every
# file was "just edited" would be misleading.
#
# Written by Shell-Shoccar Japan (@shellshoccarjpn) on 2026-10-06
#
# This is a public-domain software (CC0). It means that all of the
# people can use this for any purposes with no restrictions at all.
# By the way, we are fed up with the side effects which are brought
# about by the major licenses.
#
# The latest version is distributed at the following page.
# https://github.com/ShellShoccar-jpn/tokideli
#
######################################################################


######################################################################
# Initial Configuration
######################################################################

set -eu
umask 0022
export LC_ALL=C
IFS='
'

print_usage_and_exit() {
  cat <<-USAGE 1>&2
	USAGE   : ${0##*/} <new-version>
	Args    : <new-version> ... The new semantic version, e.g. "1.1.0"
	                             (must be of the form X.Y.Z)
	USAGE
  exit 1
}
error_exit() {
  echo "${0##*/}: $2" 1>&2
  exit "$1"
}

# --- locate the repository root (this script lives in <root>/release) ---
Dir_release=$(cd "$(dirname "$0")" && pwd)
Dir_root=$(cd "$Dir_release/.." && pwd)


######################################################################
# Parse Arguments
######################################################################

case $# in 1) :;; *) print_usage_and_exit;; esac
New_version=$1
case "$New_version" in
  [0-9]*.[0-9]*.[0-9]*) :;;
  *) error_exit 1 "$New_version: not a valid semantic version (expected X.Y.Z)";;
esac


######################################################################
# Main Routine
######################################################################

# === Update the root VERSION file (the single source of truth) =====
printf '%s\n' "$New_version" > "$Dir_root/VERSION"
echo "updated: VERSION"

# === Update the "Version" line in every c_src/*.c file ==============
# (the literal line is: "Version      : X.Y.Z\n" -- the padding keeps
#  it aligned with the "Last Updated : ..." line right below it, which
#  this script never touches)
for f in "$Dir_root"/c_src/*.c; do
  sed -i.bak \
    -e 's/"Version      : [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\\n"/"Version      : '"$New_version"'\\n"/' \
    "$f"
  rm -f "$f.bak"
  echo "updated: ${f#"$Dir_root"/}"
done

# === Update the "Version" line in every cmd_scripts/*.sh file =======
# (same padding convention, but as plain heredoc text rather than a
#  C string literal, so no trailing "\n" token to match)
for f in "$Dir_root"/cmd_scripts/*.sh; do
  sed -i.bak \
    -e 's/^\(	*\)Version      : [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$/\1Version      : '"$New_version"'/' \
    "$f"
  rm -f "$f.bak"
  echo "updated: ${f#"$Dir_root"/}"
done

cat <<-NEXTSTEPS

	Done. Next steps:
	  1. cd "$Dir_root/c_src" && sh MAKE.sh -u    # confirm everything still builds
	  2. git diff                                 # review the changes
	  3. git add VERSION c_src/*.c cmd_scripts/*.sh
	  4. git commit -m "Bump version to $New_version"
	  5. git tag -a "v$New_version" -m "v$New_version"
	  6. git push && git push --tags
	  7. gh release create "v$New_version" --notes-file <(echo "...")
NEXTSTEPS
