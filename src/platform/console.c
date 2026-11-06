// SPDX-License-Identifier: MIT
#include "os.h"
#include <stdio.h>
#include <stdlib.h>

#if ZC_OS_WINDOWS
#include <windows.h>
#include <conio.h>
#include <io.h>

static DWORD orig_console_mode;
static int raw_mode_enabled = 0;

void repl_disable_raw_mode(void)
{
    if (raw_mode_enabled)
    {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(hIn, orig_console_mode);
        raw_mode_enabled = 0;
    }
}

void repl_enable_raw_mode(void)
{
    if (!raw_mode_enabled)
    {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE)
        {
            return;
        }

        if (!GetConsoleMode(hIn, &orig_console_mode))
        {
            return;
        }

        DWORD raw = orig_console_mode;
        raw &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

        if (SetConsoleMode(hIn, raw))
        {
            raw_mode_enabled = 1;
            atexit(repl_disable_raw_mode);
        }
    }
}

int repl_read_char(char *c)
{
    DWORD read;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    if (ReadFile(hIn, c, 1, &read, NULL))
    {
        return read > 0;
    }
    return 0;
}

int repl_input_pending(int timeout_ms)
{
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    ULONGLONG deadline = GetTickCount64() + (ULONGLONG)timeout_ms;

    while (1)
    {
        INPUT_RECORD record;
        DWORD count;
        if (!PeekConsoleInputA(hIn, &record, 1, &count))
        {
            return 0;
        }
        if (count == 0)
        {
            ULONGLONG now = GetTickCount64();
            if (now >= deadline)
            {
                return 0;
            }
            WaitForSingleObject(hIn, (DWORD)(deadline - now));
            continue;
        }
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown &&
            record.Event.KeyEvent.uChar.AsciiChar != 0)
        {
            return 1;
        }
        // Key releases and focus or mouse events signal the handle but give ReadFile no byte.
        ReadConsoleInputA(hIn, &record, 1, &count);
    }
}

int repl_get_window_size(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return 0;
    }
    return -1;
}

#else // POSIX

#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios orig_termios;
static int raw_mode_enabled = 0;

void repl_disable_raw_mode(void)
{
    if (raw_mode_enabled)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled = 0;
    }
}

void repl_enable_raw_mode(void)
{
    if (!raw_mode_enabled)
    {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        {
            return;
        }

        atexit(repl_disable_raw_mode);
        struct termios raw = orig_termios;
        raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= (tcflag_t) ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1)
        {
            raw_mode_enabled = 1;
        }
    }
}

int repl_read_char(char *c)
{
    ssize_t nread = read(STDIN_FILENO, c, 1);
    return nread == 1;
}

int repl_input_pending(int timeout_ms)
{
    struct pollfd in = {.fd = STDIN_FILENO, .events = POLLIN};
    return poll(&in, 1, timeout_ms) > 0;
}

int repl_get_window_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

#endif
