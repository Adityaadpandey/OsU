#ifndef COSPY_H
#define COSPY_H

/* CosyPy - A Python-like scripting language for OsU
 *
 * Supported syntax:
 *   x = 5                  - variable assignment
 *   print(expr)            - print value or string
 *   if expr:               - conditional (ends with endif)
 *   else:                  - else branch
 *   endif                  - end if block
 *   while expr:            - while loop (ends with endwhile)
 *   endwhile               - end while block
 *   def name():            - function definition (ends with enddef)
 *   enddef                 - end function
 *   name()                 - function call
 *   # comment              - comment
 *   exit                   - exit REPL
 *
 * Operators: + - * / % == != < > <= >= and or not
 */

/* Start interactive CosyPy REPL */
void cospy_repl(void);

/* Execute a CosyPy script file */
int cospy_run_file(const char *filename);

#endif
