@echo off
setlocal enabledelayedexpansion

rem Compiler configuration (default to gcc)
if "%CC%"=="" set CC=gcc

rem Version — try git, then .version file, then "unknown"
set ZEN_VERSION=unknown
for /f "delims=" %%i in ('git describe --tags --always --dirty 2^>nul') do set ZEN_VERSION=%%i
if "!ZEN_VERSION!"=="unknown" (
    if exist .version (
        set /p ZEN_VERSION=<.version
    )
)

rem Compilation flags
if "%C_STD%"=="" set C_STD=gnu23
set CFLAGS=-std=%C_STD% -Wall -Wextra -Wshadow -Wformat=2 -Wmissing-prototypes ^
 -Wstrict-prototypes -Wnull-dereference -Wundef -Wfloat-equal ^
 -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized ^
 -Wdouble-promotion -Wtautological-compare -Wshift-negative-value ^
 -Wdangling-else -Wreturn-local-addr -Wconversion -Wno-sign-conversion -Wno-float-conversion ^
 -Wswitch-default -Wvla -Wimplicit-fallthrough -Wredundant-decls -Wcast-align ^
 -Wpacked -Wdisabled-optimization -Wduplicated-cond -Wlogical-op -Wformat-signedness ^
 -g ^
 -I./src -I./src/ast -I./src/parser -I./src/codegen -I./plugins -I./src/zen ^
 -I./src/utils -I./src/lexer -I./src/analysis -I./src/lsp -I./src/diagnostics
set CFLAGS=%CFLAGS% -DZEN_VERSION=\"%ZEN_VERSION%\" -DZEN_SHARE_DIR=\".\" -DZC_ALLOW_INTERNAL

rem Feature flags
set CFLAGS=%CFLAGS% -DZC_HAS_LSP=1 -DZC_HAS_REPL=1 -DZC_HAS_PLUGINS=1 -DZC_HAS_ZEN=1
set CFLAGS=%CFLAGS% -DZC_HAS_CPP_BACKEND=1 -DZC_HAS_CUDA_BACKEND=1 -DZC_HAS_OBJC_BACKEND=1
set CFLAGS=%CFLAGS% -DZC_HAS_JSON_BACKEND=1 -DZC_HAS_LISP_BACKEND=1 -DZC_HAS_DOT_BACKEND=1
set CFLAGS=%CFLAGS% -DZC_HAS_ASTDUMP_BACKEND=1

rem The REPL's JIT needs libtcc. Unless ZC_HAS_JIT is given (1 forces it, 0 turns
rem it off), it is enabled only when a small program using libtcc compiles and links.
if not "%ZC_HAS_JIT%"=="" goto jit_done
set ZC_HAS_JIT=0
set TCC_PROBE=%TEMP%\zc_tcc_probe
> "%TCC_PROBE%.c" echo #include ^<libtcc.h^>
>> "%TCC_PROBE%.c" echo int main(void) { tcc_delete(tcc_new()); return 0; }
%CC% "%TCC_PROBE%.c" -o "%TCC_PROBE%.exe" -ltcc >nul 2>&1 && set ZC_HAS_JIT=1
del "%TCC_PROBE%.c" "%TCC_PROBE%.exe" >nul 2>&1
if "%ZC_HAS_JIT%"=="0" echo libtcc not found; building without the REPL JIT.
:jit_done
if "%ZC_HAS_JIT%"=="1" (
    set CFLAGS=%CFLAGS% -DZC_HAS_JIT
    set LIBS=-lws2_32 -ltcc
) else (
    set LIBS=-lws2_32
)

if "%NO_PLUGINS%"=="1" (
    set CFLAGS=%CFLAGS% -DZC_NO_PLUGINS
)

rem Source files — read from src-sources.txt (single source of truth). Each one is compiled
rem once into obj\ and listed in the response file of its group, as the Makefile splits them:
rem the LSP, REPL and doc generator sources only go into their standalone tools, and the
rem driver only into zc. Response files keep the link commands short.
if not exist obj mkdir obj
for %%g in (base common driver lsp repl zen) do type nul > obj\%%g.rsp

echo Building Zen C (%ZEN_VERSION%)...
for /f "usebackq delims=" %%i in ("src-sources.txt") do (
    if not "%%i"=="" (
        %CC% %CFLAGS% -c %%i -o obj\%%~ni.o
        if errorlevel 1 (
            echo Build failed!
            exit /b 1
        )
        set "SRC=%%i"
        set GROUP=base
        if "!SRC:~0,8!"=="src/lsp/" set GROUP=lsp
        if "!SRC:~0,9!"=="src/repl/" set GROUP=repl
        if "!SRC:~0,8!"=="src/zen/" set GROUP=zen
        if "!SRC:~0,10!"=="src/tools/" set GROUP=common
        if "!SRC:~0,11!"=="src/driver/" set GROUP=driver
        if "!SRC!"=="src/main.c" set GROUP=driver
        >> obj\!GROUP!.rsp echo obj/%%~ni.o
    )
)

%CC% %CFLAGS% @obj/base.rsp @obj/common.rsp @obj/driver.rsp -o zc.exe %LIBS%
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)
echo Build success! zc.exe created.

rem Standalone tools: `zc lsp|repl|doc|format` launch them, so they ship next to zc.exe.
rem Each entry is the tool, the group of its own sources and its main.
for %%t in ("zc-lsp lsp main_lsp" "zc-format lsp main_format" "zc-repl repl main_repl" "zc-doc zen main_doc") do (
    for /f "tokens=1-3" %%a in (%%t) do (
        %CC% %CFLAGS% -c src/tools/%%c.c -o obj\%%c.o
        if errorlevel 1 (
            echo Build failed for %%a.
            exit /b 1
        )
        %CC% %CFLAGS% @obj/base.rsp @obj/common.rsp @obj/%%b.rsp obj/%%c.o -o %%a.exe %LIBS%
        if errorlevel 1 (
            echo Build failed for %%a.
            exit /b 1
        )
        echo Build success: %%a.exe created.
    )
)

rem Build plugins
if "%NO_PLUGINS%"=="1" (
    echo Plugins disabled by NO_PLUGINS flag.
    goto end
)
echo Building plugins...
if not exist plugins mkdir plugins
for %%f in (plugins\*.zc) do (
    echo Compiling native plugin %%f...
    .\zc.exe build %%f -shared -o %%~dpnf.dll
    if errorlevel 1 (
        echo Plugin build failed for %%f
        exit /b 1
    )
)

:end
