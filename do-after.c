#include <stdio.h>
#include <string.h>
#include <scan.h>
#include <unistd.h>
#include <errno.h>
#include <alloca.h>

#ifndef Bool
#define Bool int
#endif
#ifndef True
#define True 1
#endif
#ifndef False
#define False 0
#endif

// Globals {{{1

struct config_s {
  unsigned long seconds;
  int verbose;
} config;

int new_args_position;

// Function declarations {{{1

void usage(const char* prog_name);
void parse_options(int argc, const char* argv[]);
unsigned long parse_seconds(const char* time_string);
int exec_next(int argc, const char* argv[]);
Bool streq(const char* left, const char* right);

// Debugging {{{1

void print_config() {
  printf("Seconds: %d\n", config.seconds);
  printf("Verbose: %d\n", config.verbose);
}

// Main {{{1

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    usage(argv[0]);
    return 21;
  }

  parse_options(argc, argv);

  print_config();

  do {
    sleep(1);

    if (config.verbose >= 2) {
      printf("Seconds left: %u\n", config.seconds);
    } else if (config.verbose >= 1 && config.seconds % 60 == 0) {
      printf("Minutes left: %u\n", config.seconds / 60);
    }
  } while (config.seconds-- > 0);

  return exec_next(argc - new_args_position, argv + new_args_position);
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
  char new_time_string[20]; // should be enough for a number
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
int exec_next(int argc, const char* argv[]) {
  char** new_argv = (char**)alloca((argc + 1) * sizeof(char**));
  int i;
  for (i = 0; i < argc; i++) {
    new_argv[i] = (char*)argv[i];
  }
  new_argv[i] = '\0';

  execvp(new_argv[0], new_argv);

  // we should only reach this area on error
  int error_code = errno;
  perror("Error: execvp() failed\n");
  return error_code;
}
