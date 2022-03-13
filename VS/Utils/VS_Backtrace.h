/*
 *  VS_Backtrace.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/09/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_BACKTRACE_H
#define VS_BACKTRACE_H

extern vsString g_crashReportFile;

void vsInstallBacktraceHandler();
void vsBacktrace();

#endif // VS_BACKTRACE_H

