AC_DEFUN([CASC_SPLIT_LIBS_STRING],[
dnl
dnl Macro CASC_SPLIT_LIBS_STRING
dnl
dnl This macro takes an automake-style LIBS string (arg1) and
dnl splits it into the -L part (arg2, what CASC usually calls
dnl LIB_PATH) and -l part (arg3, what CASC calls LIB_NAME).
dnl The rest are also lumped into the LIB_NAME part, for lack
dnl of generality in the CASC distinction.
dnl
dnl
# Split $1 into the LIB_PATH part ($2) and the LIB_NAME part ($3)
if test -n "${$1}"; then
  for i in ${$1}; do
    case "$i" in
    -L*) $2="${$2} $i" ;;
    *) $3="${$3} $i" ;;
    esac
  done
fi
])
