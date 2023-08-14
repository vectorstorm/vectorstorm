/*
 *  VS_LocalisationTable.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_LOCALISATION_TABLE_H
#define VS_LOCALISATION_TABLE_H

#include "VS/Utils/VS_Singleton.h"

class vsLocalisationTable : public vsSingleton<vsLocalisationTable>
{

public:

		vsLocalisationTable();
		~vsLocalisationTable();

	void Init();
	void Init(const vsString &language);
	void Deinit();

	void SetKey( const vsString& key, const vsString& translation );

	vsString	GetTranslation( const vsString &key );
};

// Ease-of-use macro to fetch a localisation value
#define vsLoc(x) ( vsLocalisationTable::Instance()->GetTranslation(x) )
#define vsLocP(x,postfix) ( vsLocalisationTable::Instance()->GetTranslation( vsFormatString("%s_%s",x,postfix) ) )


#endif // VS_LOCALISATION_TABLE_H

