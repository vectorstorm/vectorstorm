/*
 *  VS_Profile.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/07/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

// This profiling library is based upon VSProfileLib:
//
// http://www.lighthouse3d.com/very-simple-libs/vspl/
//
// It contains several bug fixes (primarily to cope with not necessarily
// seeing the same set of profiling categories each frame), and has been
// converted to use VectorStorm's own systems instead of implementing its
// own timers.
//
// It's important to note that this profiler is NOT threadsafe.
//
// This should be considered to be a temporary class;  I'm likely to
// replace it entirely, to cope with the lack of thread safety.  But I
// needed something to track down performance differences between platforms,
// and this was able to do the job today.
//
// If you're going to use profiling functionality, I advise you only
// use the 'PROFILE' and 'PROFILE_GL' macros;  every other interface
// provided here is likely to go away or change before this code reaches the
// 'master' branch.

#ifndef VS_PROFILE_H
#define VS_PROFILE_H

#ifdef VS_TRACY
#undef TRACY_ENABLE
#define TRACY_ENABLE
// #include "VS_OpenGL.h"
#include "tracy/tracy/Tracy.hpp"
// #include "tracy/TracyOpenGL.hpp"

// class VSProfileLib
// {
// 	const char* name;
// public:
// 	/// begin profile section
// 	VSProfileLib (const char* name_in):
// 		name(name_in)
// 	{
// 		Zone
// 	}
// 	/// end profile section
// 	~VSProfileLib();
// };

#define PROFILE(name) ZoneScopedN(name)
// #define PROFILE(name) VSProfileLib __profile(name)
// #define PROFILE_GL(name) TracyGpuZone(name)
#define PROFILE_GL(name) ZoneScopedN(name);

#else

#define PROFILE(name)
#define PROFILE_GL(name)

#endif // TRACY_ENABLE

// #define VSPL_PROFILE_NONE 0
// #define VSPL_PROFILE_CPU 1
// #define VSPL_PROFILE_CPU_AND_GPU 2
//
//
//
// // #define VSPL_PROFILE VSPL_PROFILE_CPU_AND_GPU
// #define VSPL_PROFILE VSPL_PROFILE_CPU
//
// #include "VS_DisableDebugNew.h"
// #include <vector>
// #include "VS_EnableDebugNew.h"
//
// #define PROFILE_MAX_LEVELS 50
// #define LEVEL_INDENT 2
//
// #define pTime double
//
//
// class VSProfileLib
// {
// public:
// 	/// String that contains the profile report
// 	static vsString sDump;
// 	/// create the profile report and store in sDump
// 	static const vsString &DumpLevels();
// 	/// resets profile data
// 	static void Reset();
//
// 	///
// 	static void CollectQueryResults();
//
// 	/// begin profile section
// 	VSProfileLib (vsString name, bool profileGL = false);
// 	/// end profile section
// 	~VSProfileLib();
//
// protected:
// 	/// Contains information about a profiler section
// 	typedef struct {
// 		unsigned int queries[2];
// 	} pair;
// 	typedef struct s {
// 		/** Index of the parent section
// 		  * in the previous level
// 		*/
// 		int parent;
// 		/// name of the section
// 		vsString name;
// 		/// stores the time when the section starts
// 		pTime startTime;
// 		/// query indexes for the beginning
// 		/// and end of the section
// 		std::vector<pair> queriesGL[2];
// 		/** wasted time running the
// 		  * profiler code for the section
// 		*/
// 		unsigned long long int totalQueryTime;
// 		pTime wastedTime;
// 		/// Total number of calls
// 		pTime calls;
// 		/// total time spend in the profiler section
// 		pTime totalTime;
// 		///
// 		bool profileGL;
// 	}section;
//
// 	/// Information about a level of profiling
// 	typedef struct l {
// 		/// set of sections in this level
// 		std::vector <section> sec;
// 		/** stores the current profile section for
// 		  * each level */
// 		int cursor;
// 	}level;
//
// 	/// space displacement for dump string formating
// 	static int sDisp;
//
// 	static unsigned int sBackBuffer, sFrontBuffer;
//
// 	/// list of all levels
// 	static level sLevels[PROFILE_MAX_LEVELS];
// 	/// current level
// 	static int sCurrLevel;
// 	/// total number of levels
// 	static int sTotalLevels;
//
// 	// AUX FUNCTIONS
//
// 	/// Puts the profile result in sDump
// 	static void DumpLevels(int l, int p, pTime calls);
// 	/// Creates a new section
// 	void createNewSection(const vsString &name, pTime w, bool profileGL);
// 	/// returns the index of a section
// 	int searchSection(const vsString &name);
// 	/// updates the times in a section
// 	void updateSection(int cur, pTime w);
// 	/// add the time spent in the current section
// 	void accumulate();
// 	/// Gets the time
// 	static void GetTicks(pTime *ticks);
//
// public:
//
// };
//
// ///
// #if VSPL_PROFILE == VSPL_PROFILE_NONE
// #define PROFILE(name)
// #define PROFILE_GL(name)
// #elif VSPL_PROFILE == VSPL_PROFILE_CPU
// #define PROFILE(name) VSProfileLib __profile(name)
// #define PROFILE_GL(name) VSProfileLib __profile(name, false)
// #elif VSPL_PROFILE == VSPL_PROFILE_CPU_AND_GPU
// #define PROFILE(name) VSProfileLib __profile(name)
// #define PROFILE_GL(name) VSProfileLib __profile(name, true)
// #endif

#endif // VS_PROFILE_H

