#ifndef SCRIPT_H
#define SCRIPT_H

/* Execute a shell script file (.sh)
 * Each line is executed as a shell command.
 * Lines starting with # are comments.
 * Blank lines are skipped.
 * Returns 0 on success, -1 on error.
 */
int script_run(const char *filename);

#endif
