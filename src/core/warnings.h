/*
 warnings.h provides macros for selectively disabling compiler warnings
 that conflict with STYLE.md.
*/

/*
 DISABLE_WSHADOW is intended only for constructor initialiser lists
 where parameters intentionally shadow members, e.g.:

   SourceLocation(...)
     : filename(file), line(line), column(column), ...

 This pattern may trigger -Wshadow.

 - Always pair DISABLE_WSHADOW with ENABLE_WSHADOW. Do not rely on
   adjacent suppressions sharing a scope.
 - Do not use this macro for other cases without review.
*/

#if defined(__clang__)
#define DISABLE_WSHADOW \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wshadow\"")
#define ENABLE_WSHADOW \
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define DISABLE_WSHADOW \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")
#define ENABLE_WSHADOW \
    _Pragma("GCC diagnostic pop")
#else
#define DISABLE_WSHADOW
#define ENABLE_WSHADOW
#endif