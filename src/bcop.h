/*
 *
 * Beryl/Compiz options parser
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _BCOP_H
#define _BCOP_H

typedef enum {
    CompBindingTypeNone       = 0,
    CompBindingTypeKey        = 1 << 0,
    CompBindingTypeButton     = 1 << 1,
    CompBindingTypeEdgeButton = 1 << 2
} CompBindingType;

#define N_BINDING_TYPES 3

typedef enum {
    CompActionStateInitKey     = 1 <<  0,
    CompActionStateTermKey     = 1 <<  1,
    CompActionStateInitButton  = 1 <<  2,
    CompActionStateTermButton  = 1 <<  3,
    CompActionStateInitBell    = 1 <<  4,
    CompActionStateInitEdge    = 1 <<  5,
    CompActionStateTermEdge    = 1 <<  6,
    CompActionStateInitEdgeDnd = 1 <<  7,
    CompActionStateTermEdgeDnd = 1 <<  8,
    CompActionStateCommit      = 1 <<  9,
    CompActionStateCancel      = 1 << 10,
    CompActionStateModifier    = 1 << 11, // for things like wobbly's modifier key
} CompActionState;

#define N_ACTION_STATES 12

typedef enum {
	OptionTypeInt,
	OptionTypeBool,
	OptionTypeFloat,
	OptionTypeString,
	OptionTypeStringList,
	OptionTypeEnum,
	OptionTypeSelection,
	OptionTypeColor,
	OptionTypeAction,
	OptionTypeMatch,
	OptionTypeError
} OptionType;

typedef enum {
	CodeBeryl,
	CodeCompiz,
	CodeNone
} CodeType;

#define SCREEN_EDGE_LEFT        0
#define SCREEN_EDGE_RIGHT       1
#define SCREEN_EDGE_TOP         2
#define SCREEN_EDGE_BOTTOM      3
#define SCREEN_EDGE_TOPLEFT     4
#define SCREEN_EDGE_TOPRIGHT    5
#define SCREEN_EDGE_BOTTOMLEFT  6
#define SCREEN_EDGE_BOTTOMRIGHT 7
#define SCREEN_EDGE_NUM         8

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct _subGroup subGroup;
typedef struct _Group Group;
typedef struct _Option Option;
typedef struct _OptionValuesList OptionValuesList;


struct _OptionValuesList
{
	OptionValuesList* next;
	char *name;
	char *uName;
	char *fUName;
	char *value;
	int raw;
	int def;
};

typedef struct _IntOptionInfo
{
    int min;
    int max;
    int def;
} IntOptionInfo;

typedef struct _BoolOptionInfo
{
    int def;
} BoolOptionInfo;

typedef struct _FloatOptionInfo
{
    float min;
    float max;
    float precision;
	float def;
} FloatOptionInfo;

typedef struct _StringOptionInfo
{
    char *def;
} StringOptionInfo;

typedef struct _MatchOptionInfo
{
    char *def;
} MatchOptionInfo;

typedef struct _StringListOptionInfo
{
	OptionValuesList* begin;
	OptionValuesList* end;
} StringListOptionInfo;

typedef struct _ColorOptionInfo
{
	int red;
	int green;
	int blue;
	int alpha;
} ColorOptionInfo;

typedef struct _BindingOptionInfo
{
	int type;
	int state;
	char *key;
	char *mouse;
	int edge;
	int bell;
	int skey;
	int smouse;
	int sedge;
	int sbell;
} BindingOptionInfo;


typedef union _OptionInfo
{
    IntOptionInfo             asInt;
    FloatOptionInfo           asFloat;
	BoolOptionInfo            asBool;
    StringOptionInfo          asString;
    BindingOptionInfo         asBind;
    StringListOptionInfo      asSList;
	ColorOptionInfo           asColor;
	StringOptionInfo          asMatch;
} OptionInfo;

struct _Option {
	Option *next;
	char *name;
	char *uName;
	char *fUName;
	char *shortDesc;
	char *longDesc;
	char *hints;
	int screen;
	int advanced;
	OptionType type;

	OptionInfo data;

	char *group;
	char *subGroup;

	char *temp;
};

typedef struct _Context {
	char *name;
	char *uName;
	char *fUName;
	CodeType mode;
	Option *options;
} Context;

extern Context data;
extern char* programName;
extern char* fileName;
extern int quiet;

typedef struct _ParserState {
	int screen;
	char *group;
	char *subGroup;
} ParserState;

char * strToLower(char *str);
char * strToUpper(char *str);
char * strToFirstUp(char *str);
char * strToCamel(char *str);

int genCode(char *src, char *hdr);

#endif
