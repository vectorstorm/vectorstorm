/*
 *  VS_Log.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_LOG
#define VS_LOG

void vsLog_Start();
void vsLog_End();
void vsLog_Show();

void vsLog(const char *format, ...);
void vsLog(const vsString &str);

void vsErrorLog(const char *format, ...);
void vsErrorLog(const vsString &str);


#endif // VS_LOG

