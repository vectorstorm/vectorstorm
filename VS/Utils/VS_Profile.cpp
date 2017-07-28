/*
 *  VS_Profile.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/07/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Profile.h"

#include "VS_TimerSystem.h"
#include "VS_OpenGL.h"
#include <ctime>
#include <cstdio>


// Declare static variables
vsString VSProfileLib::sDump;
int VSProfileLib::sDisp = 0;
int VSProfileLib::sCurrLevel = -1;
int VSProfileLib::sTotalLevels = 0;
VSProfileLib::level VSProfileLib::sLevels[PROFILE_MAX_LEVELS];
unsigned int VSProfileLib::sBackBuffer = 0;
unsigned int VSProfileLib::sFrontBuffer = 1;

// Timer function defined apart for easier replacement
void
VSProfileLib::GetTicks(pTime *ticks) {

	*ticks = vsTimerSystem::Instance()->GetMicroseconds() / 1000.0;
}


// Constructor
// begin of a profile section
VSProfileLib::VSProfileLib(vsString name, bool profileGL) {

	int found;
	pTime w;
	sCurrLevel++;

	GetTicks(&w);

	// create new level
	if (sCurrLevel == sTotalLevels) {

		sLevels[sCurrLevel].cursor = -1;
		createNewSection(name, w, profileGL);
		// store the size of the largest section name
		int aux = (int)name.size() ;
		if (aux > sDisp)
			sDisp = aux;
		sTotalLevels++;
	}
	else {
		// search for name and parent
		found = searchSection(name);
		if (found != -1)
			updateSection(found, w);
		else {
			// create new section inside current level
			createNewSection(name, w, profileGL);
			// store the size of the largest section name
			// for report formatting purposes
			int aux = (int)name.size() ;
			if (aux > sDisp)
				sDisp = aux;
		}
	}
}

// End Section
VSProfileLib::~VSProfileLib() {
	// add the time spent in the current section
	accumulate();
	// decrease current level
	sCurrLevel--;
}


//////////////////////////////////////////////////////////////////////
// Instance Methods
//////////////////////////////////////////////////////////////////////

// Create a new profile section
void VSProfileLib::createNewSection(const vsString &name, pTime w, bool profileGL) {

	section s;

#if VSPL_PROFILE == VSPL_PROFILE_CPU_AND_GPU
	s.profileGL = profileGL;
#else
	s.profileGL = false;
#endif
	s.parent = (sCurrLevel > 0 ?
		sLevels[sCurrLevel-1].cursor : -1);
	s.name = name;
	s.calls = 1;
	s.totalTime = 0;
	s.totalQueryTime = 0;

	if (profileGL) {
		GL_CHECK_SCOPED("vsProfiler::CreateNewSection");
		pair p;
		glGenQueries(2, p.queries);
		glQueryCounter(p.queries[0], GL_TIMESTAMP);
		s.queriesGL[sBackBuffer].push_back(p);
	}

	GetTicks(&(s.startTime));
	s.wastedTime = s.startTime - w;
	sLevels[sCurrLevel].sec.push_back(s);

	sLevels[sCurrLevel].cursor = sLevels[sCurrLevel].sec.size()-1;
}

// Serach for a profile section
int VSProfileLib::searchSection(const vsString &name) {

	int i,max,par;

	max = (int)sLevels[sCurrLevel].sec.size();
	par = (sCurrLevel==0 ? -1 : sLevels[sCurrLevel-1].cursor);

	for(i=0;i<max;i++) {
		if (( name == sLevels[sCurrLevel].sec[i].name)  &&
			(par == sLevels[sCurrLevel].sec[i].parent))
			return(i);
	}
	return(-1);
}

// updates a profile section
void VSProfileLib::updateSection(int cur, pTime w) {

	section *s;

	s = &(sLevels[sCurrLevel].sec[cur]);
	s->calls++;
	sLevels[sCurrLevel].cursor = cur;

	if (s->profileGL) {
		GL_CHECK_SCOPED("vsProfiler::updateSection");
		pair p;
		glGenQueries(2, p.queries);
		glQueryCounter(p.queries[0], GL_TIMESTAMP);
		s->queriesGL[sBackBuffer].push_back(p);
	}
	GetTicks(&s->startTime);
	s->wastedTime += s->startTime - w;
}


// accumulates the time spent in the section
void VSProfileLib::accumulate() {

	section *s;
	pTime t,t2;
	GetTicks(&t);

	s = &(sLevels[sCurrLevel].sec[sLevels[sCurrLevel].cursor]);

	if (s->profileGL && !s->queriesGL[sBackBuffer].empty()) {
		GL_CHECK_SCOPED("vsProfiler::accumulate");
		vsAssert( !s->queriesGL[sBackBuffer].empty(), "Whaaa??" );
		glQueryCounter(s->queriesGL[sBackBuffer][s->queriesGL[sBackBuffer].size()-1].queries[1], GL_TIMESTAMP);
		vsAssert( !s->queriesGL[sBackBuffer].empty(), "Whaaa??" );
	}
	// to measure wasted time when accumulating
	GetTicks(&t2);
	s->wastedTime += (t2-t);
	s->totalTime += (t - s->startTime);

}


//////////////////////////////////////////////////////////////////////
// Class Methods
//////////////////////////////////////////////////////////////////////

// Resets profile stats
void VSProfileLib::Reset() {
	GL_CHECK_SCOPED("vsProfiler::Reset");

	for(int i=0; i < sTotalLevels; ++i) {

		for (unsigned int s = 0; s < sLevels[i].sec.size(); ++s) {
			for (unsigned int k = 0; k < sLevels[i].sec[s].queriesGL[0].size(); ++k) {
				glDeleteQueries(2, sLevels[i].sec[s].queriesGL[0][k].queries);
			}
			for (unsigned int k = 0; k < sLevels[i].sec[s].queriesGL[1].size(); ++k) {
				glDeleteQueries(2, sLevels[i].sec[s].queriesGL[1][k].queries);
			}
		}
		sLevels[i].sec.clear();
	}
	sTotalLevels = 0;
}



// Builds a string, sDump, with the profile report
const vsString &
VSProfileLib::DumpLevels() {

#if VSPL_PROFILE != VSPL_PROFILE_NONE
	int indent = sTotalLevels * LEVEL_INDENT + sDisp;
	char saux[100];

	char t1[5]="Name";
	char t2[7]="#c";
	// char t3[9]="#tc";
	char t4[8]="CPU(ms)";
	char t41[8] ="GPU(ms)";
	char t5[3]="wt";

	sDump = "";
	sprintf(saux,"%-*s  %s  %s  %s       %s\n",indent+4,t1,t2,t4,t41,t5);
	sDump += saux;
	sprintf(saux,"---- %*s\n",indent+31,"------------------------------------");
	sDump += saux;

	DumpLevels(0,-1,sLevels[0].sec[0].calls);
#else
	sDump = "";
#endif

	return sDump;
}



// private method to recursively
// build the profile report
void
VSProfileLib::DumpLevels(int l, int p, pTime calls) {


	int siz;
	char s[200];
	char s2[2000];
	section *sec;

	siz = (int)sLevels[l].sec.size();

	for(int cur = 0; cur < siz; ++cur) {
		sec = &(sLevels[l].sec[cur]);

		if (l==0)
			calls = sec->calls;

		if ((p == -1) || (sec->parent == p)) {

			// sprintf(s,"%#*s%s", l * LEVEL_INDENT ," ",sec->name.c_str());
			sprintf(s,"%.*s%s", l * LEVEL_INDENT ," ",sec->name.c_str());

			if (sec->profileGL)
				sprintf(s2,"%-*s %5.0f %8.2f %8.2f %8.2f\n",
					sDisp + sTotalLevels * LEVEL_INDENT + 2,
					s,
					(float)(sec->calls/calls),
					(float)(sec->totalTime)/(calls),
					(sec->totalQueryTime/(1000000.0 /** calls*/)),
					(float)(sec->wastedTime/(calls)));
			else
				sprintf(s2,"%-*s %5.0f %8.2f          %8.2f\n",
					sDisp + sTotalLevels * LEVEL_INDENT + 2,
					s,
					(float)(sec->calls/calls),
					(float)(sec->totalTime)/(calls),
					(float)(sec->wastedTime)/(calls));

			sDump += s2;

			if (l+1 < sTotalLevels)
				DumpLevels(l+1,cur,calls);
		}

	}
}


// Collect Queries results
void
VSProfileLib::CollectQueryResults() {

#if VSPL_PROFILE == VSPL_PROFILE_CPU_AND_GPU
	int siz;
	section *sec;
	// int availableEnd = 0;
	GLuint64 timeStart=0, timeEnd = 0;
	unsigned long long int aux = 0;

	for (int l = 0; l < sTotalLevels; ++l) {
		siz = sLevels[l].sec.size();

		for(int cur = 0; cur < siz; ++cur) {
			sec = &(sLevels[l].sec[cur]);

			if (sec->profileGL) {
				GL_CHECK_SCOPED("vsProfiler::CollectQueryResults");
				sec->totalQueryTime = 0;
				aux = 0;

				for (unsigned int j = 0; j < sec->queriesGL[sFrontBuffer].size(); ++j) {

				GL_CHECK("vsProfiler::CollectQueryResults");
					glGetQueryObjectui64v(sec->queriesGL[sFrontBuffer][j].queries[0], GL_QUERY_RESULT, &timeStart);
				GL_CHECK("vsProfiler::CollectQueryResults");
					glGetQueryObjectui64v(sec->queriesGL[sFrontBuffer][j].queries[1], GL_QUERY_RESULT, &timeEnd);
				GL_CHECK("vsProfiler::CollectQueryResults");
					aux +=  (timeEnd - timeStart);
					glDeleteQueries(2, sec->queriesGL[sFrontBuffer][j].queries);
				GL_CHECK("vsProfiler::CollectQueryResults");
				}
				sec->totalQueryTime += aux;
				sec->queriesGL[sFrontBuffer].clear();
			}
		}
	}
	// SWAP QUERY BUFFERS
	if (sBackBuffer) {
		sBackBuffer = 0;
		sFrontBuffer = 1;
	}
	else {
		sBackBuffer = 1;
		sFrontBuffer = 0;
	}
#endif
}
