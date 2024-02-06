/*
 *  VS_Backtrace.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/09/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Backtrace.h"

#include "VS_File.h"

#ifdef BACKTRACE_SUPPORTED
#ifdef _WIN32

#include "exchndl.h"
#include <dbghelp.h>

#include <stdio.h>
#include <stdlib.h>

void vsBacktrace()
{
	FILE* f = fopen("crash.rpt", "w");
	if ( f )
		printf("opened file crash.rpt!");
	else
		printf("FAILED TO OPEN file crash.rpt!");

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	CONTEXT context;
	memset(&context, 0, sizeof(CONTEXT));
	context.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&context);

	SymInitialize(process, nullptr, TRUE);

	DWORD image;
	STACKFRAME64 stackframe;
	ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
	image = IMAGE_FILE_MACHINE_I386;
	stackframe.AddrPC.Offset = context.Eip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Ebp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Esp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
	image = IMAGE_FILE_MACHINE_AMD64;
	stackframe.AddrPC.Offset = context.Rip;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.Rsp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.Rsp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	image = IMAGE_FILE_MACHINE_IA64;
	stackframe.AddrPC.Offset = context.StIIP;
	stackframe.AddrPC.Mode = AddrModeFlat;
	stackframe.AddrFrame.Offset = context.IntSp;
	stackframe.AddrFrame.Mode = AddrModeFlat;
	stackframe.AddrBStore.Offset = context.RsBSP;
	stackframe.AddrBStore.Mode = AddrModeFlat;
	stackframe.AddrStack.Offset = context.IntSp;
	stackframe.AddrStack.Mode = AddrModeFlat;
#endif

	for (int i = 0; i < 25; i++) {

		BOOL result = StackWalk64(
				image, process, thread,
				&stackframe, &context, nullptr,
				SymFunctionTableAccess64, SymGetModuleBase64, nullptr);

		if (!result) { continue; }

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		// vsString s = vsFormatString("[%d] %x\n", i, (unsigned int)stackframe.AddrPC.Offset);
		char strin[2000];
		sprintf(strin, "[%i] %#010x\n", i, (unsigned int)stackframe.AddrPC.Offset);
		fwrite( strin, 1, strlen(strin), f );

		// DWORD64 displacement = 0;
		// if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
		// 	printf("[%i] %s\n", i, symbol->Name);
		// } else {
			// printf(strin);
		// }
	}
	fclose(f);

	SymCleanup(process);
}

void vsInstallBacktraceHandler()
{
	ExcHndlInit();
}

#else

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

static int g_signal = 0;

void handler(int sig) {

	g_signal = sig;
	switch(sig)
	{

		case SIGABRT:
			vsLog("Caught SIGABRT: usually caused by abort() or assert()");
			break;
		case SIGFPE:
			vsLog("Caught SIGFPE: arithmetic exception, such as divide by zero.");
			break;
		case SIGILL:
			vsLog("Caugh SIGILL: illegal instruction");
			break;
		case SIGSEGV:
			vsLog("Caught SIGSEGV: segfault");
			break;
		case SIGTERM:
			vsLog("Caught SIGTERM: a termination request was sent to the program");
			break;
		default:
			vsLog("Caught %d\n", sig);
			break;
	}
	// print out all the frames to stderr

	vsBacktrace();
	exit(1);
}



void vsInstallBacktraceHandler()
{
	vsLog("Hooking SIGABRT (%d)", SIGABRT);
	signal(SIGABRT, handler);
	vsLog("Hooking SIGFPE (%d)", SIGFPE);
	signal(SIGFPE, handler);
	vsLog("Hooking SIGILL (%d)", SIGILL);
	signal(SIGILL, handler);
	// signal(SIGINT, handler); // SIGINT is a user interruption, like hitting ctrl-c in a terminal process.  Not a crash.
	vsLog("Hooking SIGSEGV (%d)", SIGSEGV);
	signal(SIGSEGV, handler);
	vsLog("Hooking SIGTERM (%d)", SIGTERM);
	signal(SIGTERM, handler);
}

#include "VS/Files/VS_File.h"

vsString g_crashReportFile = "crash.rpt";

void vsBacktrace()
{
	void *array[20];
	size_t size;

	printf("Backtracing!");

	// get void*'s for all entries on the stack
	size = backtrace(array, 20);

	FILE* f = fopen(g_crashReportFile.c_str(), "w");
	if ( f )
		printf("opened file %s!", g_crashReportFile.c_str());
	else
		printf("FAILED TO OPEN file %s!", g_crashReportFile.c_str());

	fprintf(f, "Caught signal: %d\n", g_signal);
	backtrace_symbols_fd(array, size, fileno(f));
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	fclose(f);
}

#endif
#else

void vsInstallBacktraceHandler()
{
}

void vsBacktrace()
{
}

#endif
