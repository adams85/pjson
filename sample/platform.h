#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef _MSC_VER
#include <io.h>
#define is_tty_stdout (_isatty(_fileno(stdout)))
#define read_from_stdin(buf) (_read(_fileno(stdin), buf, sizeof(buf)))
#else
#include <unistd.h>
#define is_tty_stdout (isatty(STDOUT_FILENO))
#define read_from_stdin(buf) (read(STDIN_FILENO, buf, sizeof(buf)))
#endif
