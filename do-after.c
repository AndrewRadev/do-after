#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <alloca.h>
#include <buffer.h>
#include <fmt.h>
#include <signal.h>

#ifndef Bool
#define Bool int
#endif
#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

#define not(x) (x)?False:True

// Globals {{{1

struct config_s {
  unsigned long seconds;
  int verbose;
} config;

int new_args_position;
Bool is_paused = False;

// Function declarations {{{1

void usage(const char* prog_name);
void parse_options(int argc, const char* argv[]);
unsigned long parse_seconds(const char* time_string);
void exec_next(int argc, const char* argv[]);
Bool streq(const char* left, const char* right);
void out(const char* text, unsigned long number);
void fail(const char* message);
void pause_on_quit(int sig);

// Debugging {{{1

void print_config() {
  out("Seconds: %", config.seconds);
  out("Verbose: %", config.verbose);
}

// Main {{{1

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    usage(argv[0]);
    return 21;
  }

  parse_options(argc, argv);

  // Debugging:
  // print_config();

  // Handle QUIT signal
  signal(SIGQUIT, pause_on_quit);

  do {
    sleep(1);

    // Check if a pause has been set
    if (is_paused) {
      out("Seconds left: % (Paused)", config.seconds);
      while (is_paused) {
        sleep(1);
      }

      config.seconds -= 1;
    }

    if (config.verbose >= 2) {
      out("Seconds left: %", config.seconds);
    } else if (config.verbose >= 1 && config.seconds % 60 == 0) {
      out("Minutes left: %", config.seconds / 60);
    }
  } while (config.seconds-- > 0);

  exec_next(argc - new_args_position, argv + new_args_position);

  return 30;
}

// Function definitions {{{1

void usage(const char* prog_name) {
  perror("Usage: do-after <time> <program> [<program arg1>, [<program arg2>...]]\n");
}

/**
 * Return True if strings are the same, False otherwise.
 */
Bool streq(const char* left, const char* right) {
  return (strcmp(left, right) == 0 ? True : False);
}

/**
 * Parse the command-line options and place the results in the global 'config'
 * variable.
 */
void parse_options(int argc, const char* argv[]) {
  config.verbose = 0;
  config.seconds = 0;

  int i;
  // iterate, ignoring filename:
  for (i = 1; i < argc; i++) {
    if (streq(argv[i], "-v") || streq(argv[i], "--verbose")) {
      config.verbose += 1;
    } else { // it should be the time parameter
      config.seconds = parse_seconds(argv[i]);

      // we don't need to parse anymore
      new_args_position = i + 1;
      return;
    }
  }
}

/**
 * Return the seconds timeout according to the contents of the 'time_string'
 * parameter. If the 'time_string' ends with "m", the time is taken to be
 * minutes.
 */
unsigned long parse_seconds(const char* time_string) {
  char new_time_string[FMT_ULONG];
  int len = strlen(time_string);
  int mul = 1;
  unsigned long seconds;

  strcpy(new_time_string, time_string);

  if (new_time_string[len - 1] == 'm') { // then the time's in minutes
    new_time_string[len - 1] = '\0';
    mul = 60;
  }

  scan_ulong(new_time_string, &seconds);
  seconds *= mul;

  return seconds;
}

/**
 * Exec the program given after the time string, passing all other parameters
 * along to it.
 */
void exec_next(int argc, const char* argv[]) {
  char** new_argv = (char**)alloca((argc + 1) * sizeof(char**));
  int i;
  for (i = 0; i < argc; i++) {
    new_argv[i] = (char*)argv[i];
  }
  new_argv[i] = '\0';

  execvp(new_argv[0], new_argv);

  // we should only reach this area on error
  fail("execvp failed");
}

/**
 * Custom output function. Uses a 'number' parameter to interpolate in the
 * string -- might never be used.
 * Note: Assumes 'text' size is no more than 1023 characters
 */
void out(const char* text, unsigned long number) {
  int i;
  int bytes;
  int status;
  int l = strlen(text);
  char new_text[1024]; // will be enough for our purposes
  char number_string[FMT_ULONG];
  buffer b = BUFFER_INIT(write, 1, new_text, 1024);
  for (i = 0; i < l; i++) {
    if (text[i] == '%') {
      bytes = fmt_ulong(number_string, number);
      number_string[bytes] = '\0';
      status = buffer_put(&b, number_string, strlen(number_string));
    } else {
      status = buffer_put(&b, text + i, 1);
    }

    if (status < 0) { // then there was an error
      fail("buffer_put failed");
    }
  }

  if (buffer_putnlflush(&b) < 0) {
    fail("buffer_putnlflush failed");
  }
}

void fail(const char* message) {
  int status = errno;
  perror(message);
  exit(status);
}

/**
 * On receiving a QUIT signal, just pause or unpause the counting
 */
void pause_on_quit(int sig) {
  (void)sig;
  is_paused = not(is_paused);
}
