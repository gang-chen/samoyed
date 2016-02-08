BEGIN {
D["PACKAGE_NAME"]=" \"Samoyed\""
D["PACKAGE_TARNAME"]=" \"samoyed\""
D["PACKAGE_VERSION"]=" \"0.0.0\""
D["PACKAGE_STRING"]=" \"Samoyed 0.0.0\""
D["PACKAGE_BUGREPORT"]=" \"\""
D["PACKAGE_URL"]=" \"\""
D["SAMOYED_MAJOR_VERSION"]=" 0"
D["SAMOYED_MINOR_VERSION"]=" 0"
D["SAMOYED_MICRO_VERSION"]=" 0"
D["STDC_HEADERS"]=" 1"
D["HAVE_SYS_TYPES_H"]=" 1"
D["HAVE_SYS_STAT_H"]=" 1"
D["HAVE_STDLIB_H"]=" 1"
D["HAVE_STRING_H"]=" 1"
D["HAVE_MEMORY_H"]=" 1"
D["HAVE_STRINGS_H"]=" 1"
D["HAVE_INTTYPES_H"]=" 1"
D["HAVE_STDINT_H"]=" 1"
D["HAVE_UNISTD_H"]=" 1"
D["__EXTENSIONS__"]=" 1"
D["_ALL_SOURCE"]=" 1"
D["_GNU_SOURCE"]=" 1"
D["_POSIX_PTHREAD_SEMANTICS"]=" 1"
D["_TANDEM_SOURCE"]=" 1"
D["PACKAGE"]=" \"samoyed\""
D["VERSION"]=" \"0.0.0\""
D["LT_OBJDIR"]=" \".libs/\""
D["HOST_OS"]=" \"mingw64\""
D["OS_WINDOWS"]=" /**/"
D["HAVE_ICONV"]=" 1"
D["ENABLE_NLS"]=" 1"
D["HAVE_GETTEXT"]=" 1"
D["HAVE_DCGETTEXT"]=" 1"
D["GETTEXT_PACKAGE"]=" PACKAGE_TARNAME"
D["HAVE_WCHAR_H"]=" 1"
D["HAVE_ROUND"]=" 1"
D["HAVE_BOOST"]=" /**/"
D["HAVE_BOOST_CHRONO"]=" /**/"
D["HAVE_BOOST_REGEX"]=" /**/"
D["HAVE_BOOST_SYSTEM"]=" /**/"
D["HAVE_BOOST_THREAD"]=" /**/"
D["HAVE_LIBCLANG"]=" 1"
D["HAVE_CLANG_C_INDEX_H"]=" 1"
D["HAVE_DB_H"]=" 1"
  for (key in D) D_is_set[key] = 1
  FS = ""
}
/^[\t ]*#[\t ]*(define|undef)[\t ]+[_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]*([\t (]|$)/ {
  line = $ 0
  split(line, arg, " ")
  if (arg[1] == "#") {
    defundef = arg[2]
    mac1 = arg[3]
  } else {
    defundef = substr(arg[1], 2)
    mac1 = arg[2]
  }
  split(mac1, mac2, "(") #)
  macro = mac2[1]
  prefix = substr(line, 1, index(line, defundef) - 1)
  if (D_is_set[macro]) {
    # Preserve the white space surrounding the "#".
    print prefix "define", macro P[macro] D[macro]
    next
  } else {
    # Replace #undef with comments.  This is necessary, for example,
    # in the case of _POSIX_SOURCE, which is predefined and required
    # on some systems where configure will not decide to define it.
    if (defundef == "undef") {
      print "/*", prefix defundef, macro, "*/"
      next
    }
  }
}
{ print }
