/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <err.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#include <tegrabl_debug.h>
#include <var_cmd.h>

#ifndef CONSOLE_ENABLE_HISTORY
#define CONSOLE_ENABLE_HISTORY 1
#endif

#undef printf
#define printf tegrabl_printf
#undef dprintf
#define dprintf pr_debug

/* enable for debugging */
#if 0
#undef pr_debug
#define pr_debug tegrabl_printf
#endif

#define LINE_LEN 128

#define MAX_NUM_ARGS 16

#define HISTORY_LEN 16

#define LOCAL_TRACE 0

/* debug buffer */
static char *debug_buffer;

/* echo commands? */
static bool echo = true;

/* command processor state */
static mutex_t *command_lock;
int lastresult;
static bool abort_script;

#if CONSOLE_ENABLE_HISTORY
/* command history stuff */
static char *history; // HISTORY_LEN rows of LINE_LEN chars a piece
static uint history_next;

static void init_history(void);
static void add_history(const char *line);
static uint start_history_cursor(void);
static const char *next_history(uint *cursor);
static const char *prev_history(uint *cursor);
static void dump_history(void);
#endif

/* list of installed commands */
static cmd_block *command_list = NULL;

/* a linear array of statically defined command blocks,
   defined in the linker script.
 */
extern cmd_block __commands_start;
extern cmd_block __commands_end;

static int cmd_help(int argc, const cmd_args *argv);
static int cmd_echo(int argc, const cmd_args *argv);
static int cmd_exit(int argc, const cmd_args *argv);
#if CONSOLE_ENABLE_HISTORY
static int cmd_history(int argc, const cmd_args *argv);
#endif
static int cmd_setvar(int argc, const cmd_args *argv);
static int cmd_printvar(int argc, const cmd_args *argv);
static int cmd_savevar(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("help", "print command description/usage", &cmd_help)
STATIC_COMMAND("?", "alias for \'help\'", &cmd_help)
STATIC_COMMAND("echo", "echo args to console", &cmd_echo)
STATIC_COMMAND("exit", "exit shell", &cmd_exit)
#if CONSOLE_ENABLE_HISTORY
STATIC_COMMAND("history", "command history", &cmd_history)
STATIC_COMMAND("setvar", "set a variable with user specified value", &cmd_setvar)
STATIC_COMMAND("printvar", "print value of specified variable", &cmd_printvar)
/*STATIC_COMMAND("savevar", "save all the variables in persistent storage", &cmd_savevar)*/ /*TODO*/
#endif
STATIC_COMMAND_END(help);

int console_init(void)
{
	command_lock = calloc(sizeof(mutex_t), 1);
	mutex_init(command_lock);

	/* add all the statically defined commands to the list */
	cmd_block *block;
	for (block = &__commands_start; block != &__commands_end; block++) {
		dprintf("register command: %s\n", block->list->cmd_str);
		console_register_commands(block);
	}

#if CONSOLE_ENABLE_HISTORY
	init_history();
#endif

	return 0;
}

#if CONSOLE_ENABLE_HISTORY
static int cmd_history(int argc, const cmd_args *argv)
{
	dump_history();
	return 0;
}

static inline char *history_line(uint line)
{
	return history + line * LINE_LEN;
}

static inline uint ptrnext(uint ptr)
{
	return (ptr + 1) % HISTORY_LEN;
}

static inline uint ptrprev(uint ptr)
{
	return (ptr - 1) % HISTORY_LEN;
}

static void dump_history(void)
{
	printf("command history:\n");
	uint ptr = 0;
	int i;
	for (i=0; i < HISTORY_LEN; i++) {
		if (history_line(ptr)[0] != 0)
			printf("%s\n", history_line(ptr));
		ptr = ptrnext(ptr);
	}
}

static void init_history(void)
{
	/* allocate and set up the history buffer */
	history = calloc(1, HISTORY_LEN * LINE_LEN);
	history_next = 0;
}

static void add_history(const char *line)
{
	// reject some stuff
	if (line[0] == 0)
		return;

	uint last = ptrprev(history_next);
	if (strcmp(line, history_line(last)) == 0)
		return;

	strlcpy(history_line(history_next), line, LINE_LEN);
	history_next = ptrnext(history_next);
}

static uint start_history_cursor(void)
{
	return ptrprev(history_next);
}

static const char *next_history(uint *cursor)
{
	uint i = ptrnext(*cursor);

	if (i == history_next)
		return ""; // can't let the cursor hit the head

	*cursor = i;
	return history_line(i);
}

static const char *prev_history(uint *cursor)
{
	uint i;
	const char *str = history_line(*cursor);

	/* if we are already at head, stop here */
	if (*cursor == history_next)
		return str;

	/* back up one */
	i = ptrprev(*cursor);

	/* if the next one is gonna be null */
	if (history_line(i)[0] == '\0')
		return str;

	/* update the cursor */
	*cursor = i;
	return str;
}
#endif

static const cmd *match_command(const char *command)
{
	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			if (strcmp(command, curr_cmd[i].cmd_str) == 0) {
				dprintf("%s command found\n", curr_cmd[i].cmd_str);
				return &curr_cmd[i];
			}
		}
	}

	return NULL;
}

static int read_debug_line(const char **outbuffer, void *cookie)
{
	int pos = 0;
	int escape_level = 0;
#if CONSOLE_ENABLE_HISTORY
	uint history_cursor = start_history_cursor();
#endif

	char *buffer = debug_buffer;

	for (;;) {
		/* loop until we get a char */
		int c;
		if ((c = tegrabl_getc()) < 0)
			continue;

		dprintf("c = %x\n", c);

		if (escape_level == 0) {
			switch (c) {
				case '\r':
				case '\n':
					if (echo)
						tegrabl_puts("\n");
					goto done;

				case 0x7f: // backspace or delete
				case 0x8:
					if (pos > 0) {
						pos--;
						tegrabl_puts("\b");
						tegrabl_putc(' ');
						tegrabl_puts("\b"); // move to the left one
					}
					break;

				case 0x1b: // escape
					/* arrow keys with 3 byte code sequence is not supported */
					// escape_level++;
					/* alt + arrow key is supported */
					escape_level = 2;
					break;

				default:
					buffer[pos++] = c;
					if (echo)
						tegrabl_putc(c);
			}
		} else if (escape_level == 1) {
			// inside an escape, look for '['
			if (c == '[') {
				escape_level++;
			} else {
				// we didn't get it, abort
				escape_level = 0;
			}
		} else { // escape_level > 1
			switch (c) {
				case 67: // right arrow
					buffer[pos++] = ' ';
					if (echo) {
						tegrabl_putc(' ');
					}
					break;
				case 68: // left arrow
					if (pos > 0) {
						pos--;
						if (echo) {
							tegrabl_puts("\b"); // move to the left one
							tegrabl_putc(' ');
							tegrabl_puts("\b"); // move to the left one
						}
					}
					break;
#if CONSOLE_ENABLE_HISTORY
				case 65: // up arrow -- previous history
				case 66: // down arrow -- next history
					// wipe out the current line
					while (pos > 0) {
						pos--;
						if (echo) {
							tegrabl_puts("\b"); // move to the left one
							tegrabl_putc(' ');
							tegrabl_puts("\b"); // move to the left one
						}
					}

					if (c == 65)
						strlcpy(buffer, prev_history(&history_cursor), LINE_LEN);
					else
						strlcpy(buffer, next_history(&history_cursor), LINE_LEN);
					pos = strlen(buffer);
					if (echo)
						tegrabl_puts(buffer);
					break;
#endif
				default:
					break;
			}
			escape_level = 0;
		}

		/* end of line. */
		if (pos == (LINE_LEN - 1)) {
			tegrabl_puts("\nerror: line too long\n");
			pos = 0;
			goto done;
		}
	}

done:
//  dprintf("returning pos %d\n", pos);

	// null terminate
	buffer[pos] = 0;

#if CONSOLE_ENABLE_HISTORY
	// add to history
	add_history(buffer);
#endif

	// return a pointer to our buffer
	*outbuffer = buffer;

	return pos;
}

static int tokenize_command(const char *inbuffer, const char **continuebuffer, char *buffer,
	size_t buflen, cmd_args *args, int arg_count)
{
	int inpos;
	int outpos;
	int arg;
	enum {
		INITIAL = 0,
		NEXT_FIELD,
		SPACE,
		IN_SPACE,
		TOKEN,
		IN_TOKEN,
		QUOTED_TOKEN,
		IN_QUOTED_TOKEN,
		VAR,
		IN_VAR,
		COMMAND_SEP,
	} state;
	char varname[128];
	int varnamepos;

	inpos = 0;
	outpos = 0;
	arg = 0;
	varnamepos = 0;
	state = INITIAL;
	*continuebuffer = NULL;

	for (;;) {
		char c = inbuffer[inpos];

//		dprintf("c %#x state %d arg %d inpos %d pos %d\n", c, state, arg, inpos, outpos);

		switch (state) {
			case INITIAL:
			case NEXT_FIELD:
				if (c == '\0')
					goto done;
				if (isspace(c))
					state = SPACE;
				else if (c == ';')
					state = COMMAND_SEP;
				else
					state = TOKEN;
				break;
			case SPACE:
				state = IN_SPACE;
				break;
			case IN_SPACE:
				if (c == '\0')
					goto done;
				if (c == ';') {
					state = COMMAND_SEP;
				} else if (!isspace(c)) {
					state = TOKEN;
				} else {
					inpos++; // consume the space
				}
				break;
			case TOKEN:
				// start of a token
				DEBUG_ASSERT(c != '\0');
				if (c == '"') {
					// start of a quoted token
					state = QUOTED_TOKEN;
				} else if (c == '$') {
					// start of a variable
					state = VAR;
				} else {
					// regular, unquoted token
					state = IN_TOKEN;
					args[arg].str = &buffer[outpos];
				}
				break;
			case IN_TOKEN:
				if (c == '\0') {
					arg++;
					goto done;
				}
				if (isspace(c) || c == ';') {
					arg++;
					buffer[outpos] = 0;
					outpos++;
					/* are we out of tokens? */
					if (arg == arg_count)
						goto done;
					state = NEXT_FIELD;
				} else {
					buffer[outpos] = c;
					outpos++;
					inpos++;
				}
				break;
			case QUOTED_TOKEN:
				// start of a quoted token
				DEBUG_ASSERT(c == '"');

				state = IN_QUOTED_TOKEN;
				args[arg].str = &buffer[outpos];
				inpos++; // consume the quote
				break;
			case IN_QUOTED_TOKEN:
				if (c == '\0') {
					arg++;
					goto done;
				}
				if (c == '"') {
					arg++;
					buffer[outpos] = 0;
					outpos++;
					/* are we out of tokens? */
					if (arg == arg_count)
						goto done;

					state = NEXT_FIELD;
				}
				buffer[outpos] = c;
				outpos++;
				inpos++;
				break;
			case VAR:
				DEBUG_ASSERT(c == '$');

				state = IN_VAR;
				args[arg].str = &buffer[outpos];
				inpos++; // consume the dollar sign

				// initialize the place to store the variable name
				varnamepos = 0;
				break;
			case IN_VAR:
				if (c == '\0' || isspace(c) || c == ';') {
					// hit the end of variable, look it up and stick it inline
					varname[varnamepos] = 0;

					(void)varname[0]; // nuke a warning

					buffer[outpos++] = '0';
					buffer[outpos++] = 0;

					arg++;
					/* are we out of tokens? */
					if (arg == arg_count)
						goto done;

					state = NEXT_FIELD;
				} else {
					varname[varnamepos] = c;
					varnamepos++;
					inpos++;
				}
				break;
			case COMMAND_SEP:
				// we hit a ;, so terminate the command and pass the remainder of the command back in continuebuffer
				DEBUG_ASSERT(c == ';');

				inpos++; // consume the ';'
				*continuebuffer = &inbuffer[inpos];
				goto done;
		}
	}

done:
	buffer[outpos] = 0;
	return arg;
}

static void convert_args(int argc, cmd_args *argv)
{
	int i;

	for (i = 0; i < argc; i++) {
		argv[i].u = atoui(argv[i].str);
		argv[i].i = atoi(argv[i].str);

		if (!strcmp(argv[i].str, "true") || !strcmp(argv[i].str, "on")) {
			argv[i].b = true;
		} else if (!strcmp(argv[i].str, "false") || !strcmp(argv[i].str, "off")) {
			argv[i].b = false;
		} else {
			argv[i].b = (argv[i].u == 0) ? false : true;
		}
	}
}


static status_t command_loop(int (*get_line)(const char **, void *), void *get_line_cookie, bool showprompt, bool locked)
{
	bool exit;
	cmd_args *args = NULL;
	const char *buffer;
	const char *continuebuffer;
	char *outbuf = NULL;
	const size_t outbuflen = 1024;

	args = (cmd_args *) malloc (MAX_NUM_ARGS * sizeof(cmd_args));
	if (unlikely(args == NULL)) {
		goto no_mem_error;
	}

	outbuf = malloc(outbuflen);
	if (unlikely(outbuf == NULL)) {
		goto no_mem_error;
	}

	exit = false;
	continuebuffer = NULL;
	while (!exit) {
		// read a new line if it hadn't been split previously and passed back from tokenize_command
		if (continuebuffer == NULL) {
			if (showprompt)
				tegrabl_puts("TEGRA194 # ");

			int len = get_line(&buffer, get_line_cookie);
			if (len < 0)
				break;
			if (len == 0)
				continue;
		} else {
			buffer = continuebuffer;
		}

		dprintf("line = '%s'\n", buffer);

		/* tokenize the line */
		int argc = tokenize_command(buffer, &continuebuffer, outbuf, outbuflen,
									args, MAX_NUM_ARGS);
		if (argc < 0) {
			if (showprompt)
				printf("syntax error\n");
			continue;
		} else if (argc == 0) {
			continue;
		}

		dprintf("after tokenize: argc %d\n", argc);
		for (int i = 0; i < argc; i++)
			dprintf("%d: '%s'\n", i, args[i].str);

		/* convert the args */
		convert_args(argc, args);

		/* try to match the command */
		const cmd *command = match_command(args[0].str);
		if (!command) {
			if (showprompt)
				printf("command not found - try \'help\'\n");
			continue;
		}

		if (!locked)
			mutex_acquire(command_lock);

		abort_script = false;
		lastresult = command->cmd_callback(argc, args);

		// if someone intend to abort the shell
		if (abort_script)
			exit = true;
		abort_script = false;

		if (!locked)
			mutex_release(command_lock);
	}

	free(outbuf);
	free(args);
	return NO_ERROR;

no_mem_error:
	if (args)
		free(args);

	printf("%s: not enough memory\n", __func__);
	return ERR_NO_MEMORY;
}

void console_abort_script(void)
{
	abort_script = true;
}

void console_start(void)
{
	debug_buffer = malloc(LINE_LEN);

	dprintf("entering main console loop\n");

	command_loop(&read_debug_line, NULL, true, false);

	dprintf("exiting main console loop\n");

	free (debug_buffer);
}

void console_register_commands(cmd_block *block)
{
	DEBUG_ASSERT(block);
	DEBUG_ASSERT(block->next == NULL);

	block->next = command_list;
	command_list = block;
}

static int cmd_help(int argc, const cmd_args *argv)
{

	printf("command list:\n");

	cmd_block *block;
	size_t i;

	for (block = command_list; block != NULL; block = block->next) {
		const cmd *curr_cmd = block->list;
		for (i = 0; i < block->count; i++) {
			if (curr_cmd[i].help_str)
				printf("%s - %s\n", curr_cmd[i].cmd_str, curr_cmd[i].help_str);
		}
	}

	return 0;
}

static int cmd_echo(int argc, const cmd_args *argv)
{
	int i;

	if (argc > 1) {
		for (i = 1; i < argc; i++)
			printf("%s ", argv[i].str);

		printf("\n");
	}
	return NO_ERROR;
}

static int cmd_exit(int argc, const cmd_args *argv)
{
	dprintf("\tEXIT SHELL\n");
	console_abort_script();

	return NO_ERROR;
}

static int cmd_setvar(int argc, const cmd_args *argv)
{
	dprintf("\tSet variable\n");

	switch (argc) {
	case 1:
		printf("Syntax error, no variable specified\n");
		break;
	case 2:
		dprintf("Clear/delete the variable\n");
		if (is_var_boot_cfg(argv[1].str)) {
			dprintf("Delete/clear boot cfg variable = %s\n", argv[1].str);
			clear_boot_cfg_var(argv[1].str);
		} else {
			dprintf("Delete/clear %s variable.\n", argv[1].str);
			clear_var(argv[1].str);
		}
		break;
	default:
		dprintf("Set the variable = %s\n", argv[1].str);
		if (is_var_boot_cfg(argv[1].str)) {
			dprintf("Add/update boot cfg variable = %s\n", argv[1].str);
			update_boot_cfg_var(argc, argv);
		} else {
			dprintf("Add/update %s variable.\n", argv[1].str);
			update_var(argc, argv);
		}
		break;
	}

	return NO_ERROR;
}

static int cmd_printvar(int argc, const cmd_args *argv)
{
	dprintf("\tPrint variable\n");
	if (argc > 1) {
		dprintf("Print value of %s variable\n", argv[1].str);
		if (is_var_boot_cfg(argv[1].str)) {
			dprintf("Print value of boot cfg variable = %s\n", argv[1].str);
			print_boot_cfg_var(argv[1].str);
		} else {
			dprintf("Print value of %s variable.\n", argv[1].str);
			print_var(argv[1].str);
		}
	} else {
		printf("Syntax error, variable name is missing\n");
	}

	return NO_ERROR;
}

static int cmd_savevar(int argc, const cmd_args *argv)
{
	dprintf("\tSave variable\n");
	/* TODO */
	/* save all variables i.e. write current dtb contents to storage */

	return NO_ERROR;
}

