/*
 *  VS_BuiltInFont.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/03/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_BuiltInFont.h"
#include "VS_DisplayList.h"
#include "VS_Fragment.h"

static float GetDisplayListWidth( vsDisplayList *list )
{
	vsVector2D topLeft, bottomRight;

	list->GetBoundingBox(topLeft, bottomRight);
	topLeft.x = 0.f;
	float width = bottomRight.x - topLeft.x;
	return width;
}

int lookup_letterIndex( char letter )
{
	char upperLetter = toupper( letter );
	if( upperLetter >= 'A' && upperLetter <= 'Z' )
	{
		return int( upperLetter - 'A' );
	}
	else if( isdigit( letter ) )
	{
		return 26 + int( letter - '0' );
	}
	else switch( letter )
	{
		case ':': return 36;
		case '.': return 37;
		case '-': return 38;
		case '%': return 39;
		case '+': return 40;
		case '@': return 41;
		case '(': return 42;
		case ')': return 43;
		case '/': return 44;
		case '\\': return 45;
		case '=' : return 46;
		case '!' : return 47;
		case '_' : return 48;
		case '<' : return 49;
		case '>' : return 50;
		case ',' : return 51;
		case '"' : return 52;
		case ';' : return 53;	// actually back-quotes.  Hope I never need to use a semicolon!
		case '?' : return 54;
		case '\'' : return 55;
		case '$' : return 56;
		default:  return -1;
	}
}

#define HEIGHT (-0.75f)

#define W	(0.5f)
#define W2  (W * 0.5f)
#define H	((HEIGHT*0.33f))// + 0.25f)
#define H2	((HEIGHT*0.66f))// + 0.25f)
#define H3	((HEIGHT))// + 0.25f)

vsVector2D baseline(0.f,0.25f);


static vsRenderBuffer *s_P;
static vsRenderBuffer *s_I;


// The points.
vsVector3D P [24] = {
	vsVector2D::Zero,

	vsVector2D(-W,H3),			vsVector2D(0,H3),        	vsVector2D(W,H3),
	vsVector2D(-W,H2),			vsVector2D(0,H2),			vsVector2D(W,H2),
	vsVector2D(-W,H),			vsVector2D(0,H),			vsVector2D(W,H),
	vsVector2D(-W,0),			vsVector2D(0,0),        	vsVector2D(W,0),
	vsVector2D(-W,-H),			vsVector2D(0,-H),        	vsVector2D(W,-H),
	vsVector2D(-W,-H2),        	vsVector2D(0,-H2),        	vsVector2D(W,-H2),
	vsVector2D(-W,-H3),        	vsVector2D(0,-H3),        	vsVector2D(W,-H3),
	              vsVector2D(-W2,-H3),			vsVector2D(W2,-H3)

};
// Vector strokes (indexed into P).
static int st_nick53[57][15] = {
	/* A */ { 13,4,2,6,15,0,10,6,-1 },
	/* B */ { 13,1,2,6,8,12,14,13,-1 },
	/* C */ { 12,14,10,4,2,6,-1 },
	/* D */ { 13,1,2,6,12,14,13,-1 },
	/* E */ { 12,14,10,4,2,6,10,-1 },
	/* F */ { 13,4,2,6,0,7,8,-1 },
	/* G */ { 11,12,14,10,4,2,6,-1 },
	/* H */ { 13,1,0,15,3,0,10,6,-1 },
	/* I */ { 14,2,-1 },
	/* J */ { 10,14,12,3,-1 },
	/* K */ { 13,1,0,7,8,6,3,0,8,12,15,-1 },
	/* L */ { 15,13,1,-1 },
	/* M */ { 13,1,5,3,15,-1 },
	/* N */ { 13,1,9,0,15,3,-1 },
	/* O */ { 14,10,4,2,6,12,14,-1 },
	/* P */ { 13,4,2,6,10,-1 },
	/* Q */ { 14,10,4,2,6,12,14,0,11,15,-1 },
	/* R */ { 13,4,2,6,10,0,8,12,15,-1 },
	/* S */ { 10,14,12,4,2,6,-1 },
	/* T */ { 14,2,0,4,2,6,-1 },
	/* U */ { 1,10,14,12,3,-1 },
	/* V */ { 1,7,14,9,3,-1 },
	/* W */ { 1,13,11,15,3,-1 },
	/* X */ { 1,4,12,15,0,3,6,10,13,-1 },
	/* Y */ { 14,5,0,1,5,3,-1 },
	/* Z */ { 1,3,7,13,15,-1 },

	/* 0 */ { 14,10,4,2,6,12,14,0,10,6,-1 },
	/* 1 */ { 13,15,0,14,2,4,-1 },
	/* 2 */ { 15,13,9,6,2,4,-1 },
	/* 3 */ { 4,2,6,8,12,14,10,-1 },
	/* 4 */ { 14,8,0,12,10,4,2,-1 },
	/* 5 */ { 3,1,7,5,9,12,14,10,-1 },
	/* 6 */ { 3,2,4,10,14,12,9,5,7,-1 },
	/* 7 */ { 1,3,9,13,0,9,8,-1 },
	/* 8 */ { 2,4,12,14,10,6,2,-1 },
	/* 9 */ { 15,6,2,4,7,11,9,-1 },

	/* : */ { 2,4,5,2,0,10,11,13,10,-1 },
	/* . */ { 11,13,14,11,-1 },
	/* - */ { 10,9,-1 },
	/* % */ { 1,2,4,1,0,12,14,15,12,0,13,3,-1 },
	/* + */ { 10,9,0,5,14,-1 },
	/* @ */ { 12,5,11,12,6,2,4,10,14,15,-1 },
	/* ( */ { 2,4,7,10,14,-1 },
	/* ) */ { 2,6,9,12,14,-1 },
	/* / */ { 10,3,-1 },
	/* \ */ { 1,12,-1 },
	/* = */ { 4,6,0,7,9,-1 },
	/* ! */ { 11,13,14,11,0,8,2,-1 },
	/* _ */ { 13,15,-1 },
	/* < */ { 6,7,12,-1 },
	/* > */ { 4,9,10,-1 },
	/* , */ { 11,13,20,11,-1 },
	/* " */ { 1,2,5,1,0,2,3,6,2,-1 },
	/* ; */ { 1,2,4,1,0,2,3,5,2,-1 },
	/* ? */ { 4,2,6,8,0,11,13,14,11,-1 },
	/* ' */ { 1, 2, 4, 1, -1 },
	/* $ */ { 10,14,12,4,2,6,0,2,17,-1 }
	///* @ */ { 12,5,11,6,2,4,10,14,15,-1 }
	///* @ */ { 12,5,11,12,9,2,4,10,14,15,-1 }
};


void
vsBuiltInFont::Init()
{
	s_P = new vsRenderBuffer;
	s_I = new vsRenderBuffer[57];
	s_P->SetArray( P, 24 );

	for ( int i = 0; i < 57; i++ )
	{
        int *strokes = st_nick53[i];
		int stripLength = 0;
		bool inStrip = false;
		uint16_t indices[100];
		int z = 0;

		int cursorPos;

		while ( strokes[z] >= 0 )
		{
			int newVertID = strokes[z++];

			if ( newVertID == 0 )
			{
				inStrip = false;
			}
			else
			{
				if ( !inStrip )
				{
					cursorPos = newVertID;
					inStrip = true;
				}
				else
				{
					indices[stripLength++] = cursorPos;
					indices[stripLength++] = newVertID;

					cursorPos = newVertID;
				}
			}
		}
		s_I[i].SetArray( indices, stripLength );
	}
}


static const float c_kerningFactor = 0.6f;
static const float c_spaceKerning = 0.8f;	// space width, compared to a regular character
static const float c_lineMarginFactor = 0.4f;	// extra space between lines, relative to caps size

static vsDisplayList s_tempFontList(1024*10);

static bool IsCap( char c )
{
	return !(c >= 'a' && c <= 'z');
}

void
vsBuiltInFont::BuildDisplayListFromCharacter( vsDisplayList *list, char c, float size, float capSize )
{
	char thisChar = c;
	float thisSize = size;

	if ( thisChar == ' ' )
	{
		// half width spaces, because that looks better.
		thisSize *= c_spaceKerning;
	}
	else if ( IsCap(thisChar) )
	{
		thisSize = capSize;
	}

	AppendCharacterToList( thisChar, list, vsVector2D::Zero, thisSize );
}

static float	GetSizeForCharacter(char c, float size, float capSize)
{
	float thisSize = size;
	if ( c == ' ' )
	{
		// half width spaces, because that looks better.
		thisSize *= c_spaceKerning;
	}
	/*else if ( c == '\n' )
	{
		thisSize = 0;
		continue;
	}*/
	else if ( IsCap(c) )
	{
		thisSize = capSize;
	}

	return thisSize;
}
/*
static float	GetSizeForCharacter(char c, float size, float capSize)
{
	return 2.f * GetHalfSizeForCharacter(c,size,capSize);
}*/

void
vsBuiltInFont::BuildDisplayListFromString( vsDisplayList *list, const char *string, float size, float capSize, JustificationType j, const vsVector2D &offset_in )
{
	vsVector2D offset = offset_in;
	size_t len = strlen(string);

	if ( j != Justification_Left )
	{
		BuildDisplayListFromString(list, string, size, capSize, Justification_Left);
		float width = GetDisplayListWidth(list);

		if ( j == Justification_Right )
			offset.x = -width;
		if ( j == Justification_Center )
			offset.x = -(width*0.5f);

		list->Clear();
	}

	//float iniXOffset = offset.x;

	list->VertexBuffer(s_P);

	vsTransform2D t;

	for ( size_t i = 0; i < len; i++ )
	{
		char thisChar = string[i];
		float thisSize = GetSizeForCharacter(thisChar, size, capSize);
		int index = lookup_letterIndex(thisChar);

		if ( index < 57 && index >= 0 )
		{
			vsAssert(index < 57, "Unknown char??");

			offset.x += c_kerningFactor * thisSize;

			t.SetTranslation( offset-(thisSize*baseline) );
			t.SetScale( vsVector2D(thisSize,thisSize) );

			list->PushTransform(t);
			list->LineListBuffer( &s_I[index] );
			list->PopTransform();
		}

//		AppendCharacterToList( string[i], list, offset, thisSize );


		offset.x += c_kerningFactor * thisSize;
	}

	list->ClearArrays();
}

vsDisplayList *
vsBuiltInFont::CreateString_Internal(const char* string, float size, float capSize, JustificationType j, float maxWidth)
{
	size *= 2.f;
	vsDisplayList *result = NULL;
	vsDisplayList loader(1024 * 10);

	CreateStringInDisplayList( &loader, string, size, capSize, j, maxWidth );

	size_t displayListSize = loader.GetSize();
	if ( displayListSize )
	{
		result = new vsDisplayList( displayListSize );
		result->Append(loader);
	}

	return result;
}

#define MAX_WRAPPED_LINES (50)

vsString	s_wrappedLine[MAX_WRAPPED_LINES];
int			s_wrappedLineCount = 0;

void
vsBuiltInFont::WrapLine(const vsString &string, float size, float capSize, JustificationType j, float maxWidth)
{
	vsString remainingString = string;

	// okay, check where we need to wordwrap
	size_t lineEnd = 0;
	size_t seekPosition = 0;
	//bool done = false;

	s_wrappedLineCount = 0;

	while ( remainingString != vsEmptyString )
	{
		vsString line = remainingString;
		seekPosition = remainingString.find(' ',seekPosition+1);

		if ( seekPosition != vsString::npos )
		{
			line.erase(seekPosition);
		}

		// check if we want to do a line break here!
		if ( GetStringWidth(line, size, capSize) > maxWidth && lineEnd > 0)
		{
			// we've gone too far, and need to wrap!  Wrap to the last safe wrap point we found.
			line.erase(lineEnd);
			remainingString.erase(0,lineEnd+1);

			s_wrappedLine[s_wrappedLineCount++] = line;

			lineEnd = 0;
			seekPosition = 0;
		}
		else if ( seekPosition == vsString::npos )
		{
			s_wrappedLine[s_wrappedLineCount++] = remainingString;
			remainingString = vsEmptyString;
		}
		else
		{
			lineEnd = seekPosition;
		}
	}
}

void
vsBuiltInFont::CreateStringInDisplayList(vsDisplayList *list, const vsString &string, float size, float capSize, JustificationType j, float maxWidth)
{
	if ( maxWidth > 0.f )
	{
		WrapLine(string, size, capSize, j, maxWidth);

		float lineHeight = capSize;
		float lineMargin = capSize * c_lineMarginFactor;

//		float baseOffsetDown = lineHeight * (m_wrappedLineCount * 0.5f);
		float totalHeight = (lineHeight * s_wrappedLineCount) + (lineMargin * (s_wrappedLineCount-1));
		float baseOffsetDown = totalHeight * 0.5f;
		float topLinePosition = baseOffsetDown - totalHeight + lineHeight;

//		float halfTotalHeight = totalHeight * 0.5f;
		list->Clear();

		vsVector2D offset(0.f,topLinePosition);

		for ( int i = 0; i < s_wrappedLineCount; i++ )
		{
			offset.y = topLinePosition + (i*(lineHeight+lineMargin));
			s_tempFontList.Clear();
			BuildDisplayListFromString( &s_tempFontList, s_wrappedLine[i].c_str(), size, capSize, j, offset );
			list->Append(s_tempFontList);
		}
		vsVector2D tl,br;
		list->GetBoundingBox(tl,br);
		/*
		float height = br.y-tl.y;
		list->Clear();

		for ( int i = 0; i < m_wrappedLineCount; i++ )
		{
			offset.Set(0.f,lineHeight*i - 0.5f*height);
			s_tempFontList.Clear();
			BuildDisplayListFromString( &s_tempFontList, m_wrappedLine[i].c_str(), size, capSize, j, offset );
			list->Append(s_tempFontList);
		}*/
	}
	else
	{
		float lineHeight = capSize;
		float totalHeight = lineHeight;
		float baseOffsetDown = totalHeight * 0.5f;
		float topLinePosition = baseOffsetDown - totalHeight + lineHeight;
		list->Clear();

		vsVector2D offset(0.f,topLinePosition);

		list->Clear();
		BuildDisplayListFromString( list, string.c_str(), size, capSize, j, offset );
	}
}

vsDisplayList *
vsBuiltInFont::CreateString(const vsString &string, float size, float capSize, JustificationType j, float maxWidth)
{
	if ( capSize < 0 )	// default param
		capSize = size;

	return CreateString_Internal( string.c_str(), size, capSize, j, maxWidth );
}

vsDisplayList *
vsBuiltInFont::CreateCharacter(char c, float size, float capSize)
{
	if ( capSize < 0 )	// default param
		capSize = size;

	vsDisplayList loader(1024);

	BuildDisplayListFromCharacter( &loader, c, size, capSize );

	vsDisplayList *result = new vsDisplayList( loader.GetSize() );
	result->Append(loader);

	return result;
}

void
vsBuiltInFont::AppendCharacterToList( char c, vsDisplayList *list, const vsVector2D &offset, float size )
{
	int index = lookup_letterIndex(c);

	if ( index >= 0 )
	{
        int *strokes = st_nick53[index];
		int z = 0;
		bool	inLineStrip = false;

		while ( strokes[z] >= 0 )
		{
			int newVertID = strokes[z++];

			if ( newVertID == 0 )
			{
				inLineStrip = false;
			}
			else
			{
				if ( inLineStrip )
				{
					list->LineTo( offset + (size * (P[newVertID]-baseline)) );
				}
				else
				{
					list->MoveTo( offset + (size * (P[newVertID]-baseline)) );
					inLineStrip = true;
				}
			}
		}
	}
}

float
vsBuiltInFont::GetStringWidth( const vsString &string, float size, float capSize )
{
	float width = 0.f;

	s_tempFontList.Clear();
	CreateStringInDisplayList(&s_tempFontList, string, size, capSize);

	{
		vsVector2D topLeft, bottomRight;

		s_tempFontList.GetBoundingBox(topLeft, bottomRight);
		topLeft.x = 0.f;
		width = bottomRight.x - topLeft.x;
	}

	return width;
}

float
vsBuiltInFont::GetCharacterWidth( char c, float size, float capSize )
{
	float width = 0.f;

	vsDisplayList * list = CreateCharacter(c, size, capSize);

	if ( list )
	{
		vsVector2D topLeft, bottomRight;

		list->GetBoundingBox(topLeft, bottomRight);
		delete(list);
		width = bottomRight.x - topLeft.x;
	}

	return width;
}

float
vsBuiltInFont::GetKerningForSize( float size )
{
	return c_kerningFactor * size;
}
