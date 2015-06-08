#include "silc.h"

int main(int argc, const char** argv) {
  /* create context and display welcome prompt */
  struct silc_ctx_t* c = silc_new_context();
  fputs(";; SilcLisp by Alex Shabanov\n", stdout);

  /* load scripts */
  for (int i = 1; i < argc; ++i) {
    const char* file_name = argv[i];
    fprintf(stdout, ";; Loading %s...\n", file_name);
    silc_obj o = silc_load(c, file_name);
    if (silc_try_get_err_code(o) > 0) {
      fprintf(stderr, ";; Error while loading %s\n", file_name);
      silc_print(c, o, stderr);
      fputc('\n', stderr);
    } else {
      fprintf(stdout, ";; Loaded %s\n", file_name);
    }
  }

  silc_obj eof = silc_err_from_code(1000); /* custom error code that will indicate an end of input */
  for (;;) {
    fputs("\n? ", stdout);
    silc_obj input = silc_read(c, stdin, eof);
    /* end of file reached? */
    if (input == eof) {
      break;
    }

    /* is it a reader error? */
    int error_code = silc_try_get_err_code(input);
    if (error_code < 0) {
      /* evaluate object read from the console */
      silc_obj result = silc_eval(c, input);
      /* is it an evaluation error? */
      error_code = silc_try_get_err_code(result);
      if (error_code < 0) {
        /* not an error, print evaluation result */
        silc_print(c, result, stdout);
        continue;
      }
    }

    if (SILC_ERR_QUIT == error_code) {
      break; /* not an error code, indication of termination */
    }

    fprintf(stderr, ";; error: %s\n", silc_err_code_to_str(error_code));
  }

  /* get exit code and free context */
  int exit_code = silc_get_exit_code(c);
  silc_free_context(c);

  fputs("\n;; Exiting... Good bye!\n", stdout);
  return exit_code;
}
