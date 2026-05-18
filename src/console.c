#include "console.h"

const char *colors[] = {
	ANSI_BLACK,
	ANSI_BLUE,
	ANSI_GREEN,
	ANSI_CYAN,
	ANSI_RED,
	ANSI_MAGENTA,
	ANSI_YELLOW,
	ANSI_WHITE,

	ANSI_DARKGRAY,
	ANSI_BRIGHTBLUE,
	ANSI_BRIGHTGREEN,
	ANSI_BRIGHTRED,
	ANSI_BRIGHTCYAN,
	ANSI_BRIGHTMAGENTA,
	ANSI_BRIGHTWHITE,
	ANSI_BRIGHTYELLOW,
	ANSI_NONE,
	NULL
};

const char *pr_levels[] = 
{
	[PR_BUG] = "BUG",
	[PR_ALERT] = "alert",
	[PR_CRIT] = "crit",
	[PR_ERR] = "err",
	[PR_WARN] = "warn",
	[PR_NOTICE] = "notice",
	[PR_INFO] = "info",
	[PR_DEBUG] = "debug",
	NULL
};

const char *pr_colors[] = {
	ANSI_BRIGHTRED,    	// BUG
	ANSI_BRIGHTRED,   	 	// ALERT
	ANSI_BRIGHTYELLOW, 	// CRIT
	ANSI_RED,				// ERR
	ANSI_YELLOW,			// WARN
	ANSI_BRIGHTCYAN,		// NOTICE
	ANSI_BRIGHTBLUE,		// INFO
	ANSI_BRIGHTWHITE,		// DEBUG
	NULL
};

/*
 * Provide detailed output if level <= 1 or if debug is enabled.
 * Always print if debug is enabled.
 * Print backtraces if debug >= 2.
 */
inline void _xnec2c_printf(int level, const char *file, const char *func, const int line, char *format, ...) 
{
	va_list args;
	if (rc_config.verbose < level)
		return;

	va_start(args, format);
	if (rc_config.debug || level <= PR_ALERT)
		fprintf(stderr, "%s[%d %s:%s]%s\t%s:%d: ",
			pr_colors[level], getpid(), pr_levels[level], func, 
			ANSI_NONE,
			file, line);
	else
		fprintf(stderr, "%s[%s]%s ",
			pr_colors[level], pr_levels[level], ANSI_NONE);

	// Try to use the local language if possible:
	vfprintf(stderr, _(format), args);

#ifdef HAVE_BACKTRACE
	// Temporarily set PR_DEBUG for print_backtrace.
	// print_backtrace uses PR_DEBUG so prevent recursion.
	// Always backtrace if there is a BUG.
	if (level == PR_BUG || (level != PR_DEBUG && rc_config.debug >= 2))
	{
		int oldverbose = rc_config.verbose;
		rc_config.verbose = PR_DEBUG;
		print_backtrace(NULL);
		fprintf(stderr, "\n");
		rc_config.verbose = oldverbose;
	}
#endif

	va_end(args);
}

