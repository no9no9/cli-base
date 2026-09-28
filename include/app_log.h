#ifndef APP_LOG_H
#define APP_LOG_H

/* Thread-safe, bounded logging. Use these instead of printf/perror in handlers.
 * Live terminal: recent messages stay in the log pane.
 * Plain mode: messages go directly to stdout / stderr. Not signal-safe. */
void app_logf(const char *format, ...);
void app_log_errorf(const char *format, ...);
void app_log_errno(const char *prefix);

#endif
