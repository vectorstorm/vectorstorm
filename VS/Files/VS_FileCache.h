/*
 *  VS_FileCache.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 08/11/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_FILECACHE_H
#define VS_FILECACHE_H

class vsStore;

class vsFileCache
{
public:

	static void Startup();
	static void Shutdown();
	static void Purge();

	static bool IsFileInCache(const vsString& filename);
	static vsStore* GetFileContents(const vsString& filename);
	static void SetFileContents(const vsString& filename, const vsStore &store);
};

#endif // VS_FILECACHE_H

