#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <alloca.h>
#include <buffer.h>
#include <fmt.h>
#include <signal.h>
#include <syslog.h>

// Defines {{{1

#ifndef Bool
#define Bool unsigned short
#endif
#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

#define not(x) (x)?False:True

#define LOG   0
#define DEBUG 0

// Globals {{{1

struct config_s {
  unsigned long seconds;
  int verbose;
} config;

Bool is_paused = False;

// Function declarations {{{1

void usage(const char* prog_name);
char** parse_options(int argc, char* argv[]);
unsigned long parse_seconds(const char* time_string);
Bool streq(const char* left, const char* right);
void out(const char* text, unsigned long number);
void fail(const char* message);
void pause_on_quit(int sig);
int catch_negative(int response_code, const char* message);
int catch_zero(int response_code, const char* message);

// Debugging {{{1

#if DEBUG
void print_config() {
  out("Seconds: %", config.seconds);
  out("Verbose: %", config.verbose);
}
#endif

// Main {{{1

int main(int argc, char *argv[]) {
  if (argc < 3) {
    // not enough arguments
    usage(argv[0]);
    return 21;
  }

  char** new_argv = parse_options(argc, argv);

#if DEBUG
  print_config();
#endif

  // Handle QUIT signal
  signal(SIGQUIT, pause_on_quit);

#if LOG
  // Log start of processing
  openlog("do-after", LOG_PID, LOG_USER);
  syslog(LOG_INFO, "Set to finish in `%u` seconds", config.seconds);
#endif

  while (config.seconds > 0) {
    // Check if a pause has been set
    if (is_paused) {
      if (config.verbose >= 2) {
        out("Seconds left: % (Paused)", config.seconds);
      } else {
        out("Minutes left: % (Paused)", config.seconds / 60);
      }

      while (is_paused) {
        pause();
      }
    }

    if (config.verbose >= 2) {
      out("Seconds left: %", config.seconds);
    } else if (config.verbose >= 1 && config.seconds % 60 == 0) {
      out("Minutes left: %", config.seconds / 60);
    }

    sleep(1);
    config.seconds--;
  }

#if LOG
  // Log end of processing
  syslog(LOG_INFO, "Finished");
  closelog();
#endif

  execvp(new_argv[0], new_argv);

  // we should only reach this area on error
  fail("execvp failed");
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
 * variable. Return the argument list of the program to execute when time runs
 * out.
 */
char** parse_options(int argc, char* argv[]) {
  config.verbose = 0;
  config.seconds = 0;

  int i;
  // iterate, ignoring filename:
  for (i = 1; i < argc; i++) {
    if (streq(argv[i], "-v") || streq(argv[i], "--verbose")) {
      config.verbose += 1;
    } else if (streq(argv[i], "-vv")) {
      config.verbose += 2;
    } else { // it should be the time parameter
      config.seconds = parse_seconds(argv[i]);

      // we don't need to parse anymore
      return argv + (i + 1);
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

  catch_zero(
      scan_ulong(new_time_string, &seconds),
      "scan_ulong failed"
      );
  seconds *= mul;

  return seconds;
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

  for (i = 0; i < l; i++) {
    if (text[i] == '%') {
      bytes = catch_zero(fmt_ulong(number_string, number), "fmt_ulong failed");
      number_string[bytes] = '\0';
      status = buffer_put(buffer_1, number_string, strlen(number_string));
    } else {
      status = buffer_put(buffer_1, text + i, 1);
    }

    catch_negative(status, "buffer_put failed");
  }

  catch_negative(buffer_putnlflush(buffer_1), "buffer_putnlflush failed");
}

void fail(const char* message) {
  int status = errno;
  perror(message);
  exit(status);
}

/**
 * Exit the application with an error if the 'response_code' is negative.
 * Otherwise, return 'response_code'.
 */
int catch_negative(int response_code, const char* message) {
  if (response_code < 0) {
    fail(message);
  }

  return response_code;
}

/**
 * Exit the application with an error if the 'response_code' is zero.
 * Otherwise, return 'response_code'.
 */
int catch_zero(int response_code, const char* message) {
  if (response_code == 0) {
    fail(message);
  }

  return response_code;
}

/**
 * On receiving a QUIT signal, just pause or unpause the counting
 */
void pause_on_quit(int sig) {
  (void)sig;
  is_paused = not(is_paused);
}
