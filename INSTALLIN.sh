#!/bin/sh
 
######################################################################
#
# INSTALLIN.SH - Build the Source Programs and Install Them in the
#                Specified Directory
#
# USAGE   : INSTALLIN.sh directory
# Ret     : $?=0 (when succeeded)
#
# Written by Shell-Shoccar Japan (@shellshoccarjpn) on 2022-06-26
#
# This is a public-domain software (CC0). It means that all of the
# people can use this for any purposes with no restrictions at all.
# By the way, We are fed up with the side effects which are brought
# about by the major licenses.
#
# The latest version is distributed at the following page.
# https://github.com/ShellShoccar-jpn/misc-tools
#
######################################################################


######################################################################
# Initial Configuration
######################################################################

# === Initialize shell environment ===================================
set -u
umask 0022
export LC_ALL=C
type command >/dev/null 2>&1 && type getconf >/dev/null 2>&1 &&
export PATH="$(command -p getconf PATH)${PATH+:}${PATH-}"
export POSIXLY_CORRECT=1 # to make Linux comply with POSIX
export UNIX_STD=2003     # to make HP-UX comply with POSIX

# === Define the functions for printing usage and error message ======
print_usage_and_exit () {
  cat <<-USAGE 1>&2
	Usage   : ${0##*/} directory
	Version : 2022-06-26 12:24:36 JST
	USAGE
  exit 1
}
error_exit() {
  ${2+:} false && echo "${0##*/}: $2" 1>&2
  exit $1
}

# === Get my directory path ==========================================
Homedir=$(d=${0%/*}/; [ "_$d" = "_$0/" ] && d='./'; cd "$d"; pwd)


######################################################################
# Parse arguments
######################################################################

# === Get the options ================================================
# --- initialize option parameters -----------------------------------
yesno=''
#
# --- get them -------------------------------------------------------
case $# in 0) print_usage_and_exit;; esac
while [ $# -gt 0 ]; do
  case $1 in
    -y) yesno='y';;
    -n) yesno='n';;
    -*) print_usage_and_exit;;
     *) break;
  esac
  shift
done
#
# --- validate -------------------------------------------------------
case $# in 1) :;; *) print_usage_and_exit;; esac
#
# --- get the directory ----------------------------------------------
case $1 in
  /*)   Dir_inst=$1    ;;
  ./*)  Dir_inst=$1    ;;
  ../*) Dir_inst=$1    ;;
  *)    Dir_inst="./$1";;
esac


######################################################################
# Main #1 (Compilation)
######################################################################

echo '===== STEP (1/3). Build Commands ====='
echo
[ -d "$Homedir/bin"           ] || {
  error_exit 1 "$Homedir/bin: Command Directory not found"
}
[ -d "$Homedir/c_src"         ] || {
  error_exit 1 "$Homedir/c_src: Source directory not found"
}
[ -f "$Homedir/c_src/MAKE.sh" ] || {
  error_exit 1 "$Homedir/c_src/MAKE.sh: Not found"
}
"$Homedir/c_src/MAKE.sh" -d ../bin || {
  error_exit 1 "$Homedir/c_src/MAKE.sh: Failed to build commands"
}
echo
echo "*** The commands were built successfully. ***"


######################################################################
# Main #2 (Installation)
######################################################################

echo
echo '===== STEP (2/3). Install Commands ====='
echo

# === Make sure of the existence of the install directory ============
if   [ -d "$Dir_inst" ]; then
  touch "$Dir_inst/.check" 2>/dev/null || {
    error_exit 1 "$1: No priviege to write a file into the directory"
  }
  rm -f "$1/.check"
elif [ -e "$Dir_inst" ]; then
  error_exit 1 "$1: Another file having the same name already exists"
else
  mkdir "$Dir_inst" 2>/dev/null || {
    error_exit 1 "$1: No priviege to make the directory"
  }
fi

# === Copy the commands ==============================================
case $(cd "$Dir_inst" && pwd) in "$Homedir") s=1;; *) s=0;; esac
for dir in 755:bin 644:manual; do
  [ -d "$Homedir/${dir#*:}" ] || continue
  case $s in 0)
    cp -pr "$Homedir/${dir#*:}" "$Dir_inst" || {
      error_exit 1 "$Homedir/bin: Failed to copy to the install directory"
    }
  ;; esac
  chmod ${dir%%:*} "$Dir_inst/${dir#*:}/"*
done

# === Display the end of installation ================================
cat <<-MESSAGE
	*** Installation has finished successfully. ***

	You have to do next is add the installed directory into the environment
	variable "PATH." I will explain how to do that.
	MESSAGE
printf '(Press [Enter] key when ready) '
case $yesno in [YyNn]) echo;; *) read s;; esac


######################################################################
# Main #3 (Path Addition)
######################################################################

echo
echo '===== STEP (3/3). Add the additional path into "PATH" ====='
echo

# === Set the path ===================================================
case $Dir_inst/bin in ${HOME}*)
  Dir_inst="\$HOME${Dir_inst#"$HOME"}"
;; esac
set -- "$Dir_inst/bin"

# === Generate the additional path string ============================
paths=''
for dir in "$@"; do
  eval [ -d \""$dir"\" ] #"
  case $? in [!0]*)
    echo "$dir: No such directory" 1>&2
    exit 1
  ;; esac
  paths="${paths}:$dir"
done
paths=${paths#:}

# === Generate the partial script to be added into the run-script ====
startrss=''
case ${SHELL##*/} in
  tcsh)  type='C shell'
         prompt='%'
         createstartts="touch"
         line_to_be_inserted="setenv PATH \"${paths}:\$PATH\""
         startrss="$startrss \$HOME/.tcshrc"
         startrss="$startrss \$HOME/.cshrc"
         ;;
  csh)  type='C shell'
         prompt='%'
         line_to_be_inserted="setenv PATH \"${paths}:\$PATH\""
         createstartts="touch"
         startrss="$startrss \$HOME/.cshrc"
         ;;
  bash)  type='Bourne shell'
         prompt='$'
         line_to_be_inserted="export PATH=\"${paths}:\$PATH\""
         createstartts="echo '[ -f \"\$HOME/.bashrc\" ] && . \"\$HOME/.bashrc\" >'"
         startrss="$startrss \$HOME/.bash_profile"
         startrss="$startrss \$HOME/.profile"
         ;;
  zsh)   type='Bourne shell'
         prompt='$'
         line_to_be_inserted="export PATH=\"${paths}:\$PATH\""
         createstartts="echo '[ -f \"\$HOME/.zshrc\" ] && . \"\$HOME/.zshrc\" >'"
         startrss="$startrss \$HOME/.zpath"
         startrss="$startrss \$HOME/.zshenv"
         startrss="$startrss \$HOME/.zprofile"
         ;;
  *sh)   type='Bourne shell'
         prompt='$'
         createstartts="echo '[ -f \"\$HOME/.shrc\" ] && . \"\$HOME/.shrc\" >'"
         line_to_be_inserted="export PATH=\"${paths}:\$PATH\""
         startrss="$startrss \$HOME/.profile"
         ;;
esac
yourstartrs=''
for startrs in $startrss; do
  eval actualstartrs=\"$startrs\" # "
  [ -f "$actualstartrs" ] && { yourstartrs=$startrs; break; }
done
[ -z "$yourstartrs" ] && yourstartrs=${startrss##* }

# === Explain how to add the additional script into the current run-script
cat <<-MESSAGE
	YOUR SHELL:
	  The shell you are using is "${SHELL##*/}."
	  And, it is a kind of "${type}."

	YOUR RUN-SCRIPT:
	  The run script you should edit is probably "${yourstartrs}."
	  (But it might be different if you use your shell in irregular usage.)

	HOW TO EDIT IT:
	  1) If "${yourstartrs}" doesn't exit, create it
	     with running the following command.
	     ------
	     $prompt [ -f "${yourstartrs}" ] || $createstartts "${yourstartrs}"
	     ------
	  2) Open it with your favorite text editor, like this.
	     ------
	     $prompt vi "${yourstartrs}"
	     ------
	  3) Find the line setting the environment variable "PATH"
	     in the file "${yourstartrs}."
	     - If you found more that one such line, pay attention to
	       the last one of them.
	     - If you could find no such line,  pay attention to
	       the last line of the file.
	  4) Insert the following line just after the line you are paying
	     attention to.
	     ------
	     $line_to_be_inserted
	     ------
	  5) To activate the setting, logout once and login again.
	     Or, run the following command.
	     ------
	     $prompt $line_to_be_inserted
	     ------
	
	... If you aren't confident to do this procedure yourself successfully,
	I could do it instead of you.
	MESSAGE

# === Add the partial script automatically if requested ==============
printf 'Do you want me to do it automatically [y/N]? '
case $yesno in [YyNn]) echo $yesno; answer=$yesno;; *) read answer;; esac
echo
case $answer in Y|y|[Yy][Ee][Ss])
  echo 'Okay, leave the rest to me!'

  backupsuffix=".$(date +%Y%m%d%H%M%S).backup"
  [ -f "$actualstartrs" ] || eval $createstartts "$actualstartrs"
  #n=$(grep ^ "$actualstartrs"                                           |
  #    nl -b a                                                           |
  #    sed 's/[[:blank:]]\{1,\}#.*$//'                                   |
  #    case $type in                                                     #
  #     (B*) grep -E '(^|[[:blank:]])PATH='                             ;;
  #     (C*) grep -E '(^|[[:blank:]])setenv[[:blank:]]+PATH[[:blank:]]+';;
  #    esac                                                              |
  #    awk 'BEGIN{n=0}; {n=$1}; END{print (n>0)?n:"$";}'                 )
  n='$'
  grep ^ "$actualstartrs" |
  sed -n "1,${n}p"        >  "${actualstartrs%/*}/${actualstartrs##*/}.new"
  printf '%s\n' "$line_to_be_inserted" >> "${actualstartrs%/*}/${actualstartrs##*/}.new"
  grep ^ "$actualstartrs" |
  sed    "1,${n}d"        >> "${actualstartrs%/*}/${actualstartrs##*/}.new"
  mv "$actualstartrs" "${actualstartrs%/*}/${actualstartrs##*/}${backupsuffix}" &&
  mv "${actualstartrs%/*}/${actualstartrs##*/}.new" "$actualstartrs"

  cat <<-MESSAGE

	Finished. Now, logout once and login again to make sure of the new PATH.
	If it happens something wrong, type the following command to restore the
	previous setting.
	  -----
	  $prompt mv "${yourstartrs%/*}/${yourstartrs##*/}${backupsuffix}" "${yourstartrs}"
	  -----
	If it works correctly, you may remove the backup file, like this.
	  -----
	  $prompt rm "${yourstartrs%/*}/${yourstartrs##*/}${backupsuffix}"
	  -----
	MESSAGE
;; *)
  echo 'Okay, do them yourself.'
;; esac
echo
echo '*** All steps Finished. Enjoy! ***'


######################################################################
# Finish
######################################################################

exit 0
