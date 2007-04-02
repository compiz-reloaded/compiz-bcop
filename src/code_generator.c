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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <libgen.h>
#include <X11/Xlib.h>

#include "bcop.h"

static char *edges[] = {
	"SCREEN_EDGE_LEFT",
	"SCREEN_EDGE_RIGHT",
	"SCREEN_EDGE_TOP",
	"SCREEN_EDGE_BOTTOM",
	"SCREEN_EDGE_TOPLEFT",
	"SCREEN_EDGE_TOPRIGHT",
	"SCREEN_EDGE_BOTTOMLEFT",
	"SCREEN_EDGE_BOTTOMRIGHT",
};

static char* actionStates[] = {
	"CompActionStateInitKey",
	"CompActionStateTermKey",
	"CompActionStateInitButton",
	"CompActionStateTermButton",
	"CompActionStateInitBell",
	"CompActionStateInitEdge",
	"CompActionStateTermEdge",
	"CompActionStateInitEdgeDnd",
	"CompActionStateTermEdgeDnd",
	"CompActionStateCommit",
	"CompActionStateCancel",
	"CompActionStateModifier"
};

static char *bindingTypes[] = {
	"CompBindingTypeKey",
	"CompBindingTypeButton",
	"CompBindingTypeEdgeButton"
};

typedef struct _StringElement
{
	char *data;
	int size;
	int used;
} StringElement;

char *hFile = NULL;

StringElement header;
StringElement file;

StringElement displayStruct;
StringElement screenStruct;
StringElement initScreenOpt;
StringElement initDisplayOpt;
StringElement setScreenOpt;
StringElement setDisplayOpt;
StringElement defines;
StringElement functions;
StringElement hdefines;
StringElement hfunctions;
StringElement initScreen;
StringElement finiScreen;
StringElement initDisplay;
StringElement finiDisplay;


int nOptions = 0;
int nSOptions = 0;
int nDOptions = 0;

static void addString(StringElement *se, const char* format, ...)
{
	char *str = NULL;
	va_list ap;

	va_start(ap, format);
	vasprintf(&str,format,ap);
	va_end(ap);

	while (strlen(str) + se->used >= se->size)
	{
		se->data = realloc(se->data, se->size + 4096);
		se->size += 4096;
	}

	sprintf(se->data + se->used, "%s", str);
	se->used += strlen(str);
	free(str);
}

static void initString(StringElement *se)
{
	se->data = calloc(1,4096);
	se->size = 4096;
	se->used = 0;
	sprintf(se->data," ");
}

static void freeString(StringElement *se)
{
	free(se->data);
	se->size = 0;
	se->used = 0;
}

static void initStrings()
{
	initString(&header);
	initString(&file);
	initString(&displayStruct);
	initString(&screenStruct);
	initString(&initScreenOpt);
	initString(&initDisplayOpt);
	initString(&setScreenOpt);
	initString(&setDisplayOpt);
	initString(&defines);
	initString(&functions);
	initString(&hdefines);
	initString(&hfunctions);
	initString(&initScreen);
	initString(&finiScreen);
	initString(&initDisplay);
	initString(&finiDisplay);
}

struct _Modifier {
        char *name;
        char *modifier;
} const static modifiers[] = {
        { "Shift","ShiftMask"},
        { "Control","ControlMask"},
        { "Mod1","Mod1Mask"},
        { "Mod2","Mod2Mask"},
        { "Mod3","Mod3Mask"},
        { "Mod4","Mod4Mask"},
        { "Mod5","Mod5Mask"},
        { "Alt","CompAltMask"},
        { "Meta","CompMetaMask"},
        { "Super","CompSuperMask"},
        { "Hyper","CompHyperMask"},
        { "ModeSwitch","CompModeSwitchMask"},
};

#define N_MODIFIERS (sizeof (modifiers) / sizeof (struct _Modifier))

static void convertBinding(char *bind, char **modret, char **value)
{
	char *tok = strtok(bind,"+");
	int i;
	int keysym = 0;
	int button = 0;
	int num_mods = 0;
	int changed;
	StringElement mods;
	initString(&mods);
	char str[1024];

	while (tok)
	{
		changed = 0;
		for (i = 0; i < N_MODIFIERS && !changed; i++)
			if (!strcmp(tok,modifiers[i].name))
			{
				addString(&mods,"%s%s",(num_mods)?" | ":"",modifiers[i].modifier);
				num_mods++;
				changed = 1;
			}
		if (!changed && strncmp(tok, "Button", strlen("Button")) == 0)
		{
			int but;
			if (sscanf(tok, "Button%d", &but) == 1)
			{
				button = but;
				changed = 1;
			}
		}
		if (!changed && !keysym)
			keysym = XStringToKeysym(tok);

		tok = strtok(NULL,"+");
	}

	if (num_mods)
	{
		sprintf(str,"%s%s%s",(num_mods > 1)?"(":"",
				mods.data,(num_mods > 1)?")":"");
		*modret = strdup(str);
	}
	else
		*modret = NULL;

	if (button)
	{
		sprintf(str,"Button%d",button);
		*value = strdup(str);
	}
	else if (keysym)
	{
		*value = strdup(XKeysymToString(keysym));
	}
	else
		*value = NULL;
	freeString(&mods);
}


void addGetFunction(Option *o, const char *type, const char *nameadd,
						const char *fetch, const char *prefix)
{
	addString(&hfunctions,"%-16s %sGet%s%s(%s);\n",type,data.name,o->fUName,nameadd,
		   (o->screen)?"CompScreen *s":"CompDisplay *d");
	addString(&functions,"%s %sGet%s%s(%s)\n",type,data.name,o->fUName,nameadd,
			(o->screen)?"CompScreen *s":"CompDisplay *d");
	if (o->screen)
		addString(&functions,"{\n\t%s_OPTIONS_SCREEN(s);\n\treturn %sos->opt[%s]%s;\n"
		   		"}\n\n",data.uName,prefix,o->temp,fetch);
	else
		addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n\treturn %sod->opt[%s]%s;\n"
		   		"}\n\n",data.uName,prefix,o->temp,fetch);
}


void addIntOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.uName,o->uName);
	addString(&defines,"#define %s_DEFAULT %d\n",name,o->data.asInt.def);
	addString(&defines,"#define %s_MIN     %d\n",name,o->data.asInt.min);
	addString(&defines,"#define %s_MAX     %d\n\n",name,o->data.asInt.max);

	addString(out,"\to->value.i = %s_DEFAULT;\n",name);
	addString(out,"\to->rest.i.min = %s_MIN;\n",name);
	addString(out,"\to->rest.i.max = %s_MAX;\n",name);

	addGetFunction(o,"int","",".value.i","");
}

void addFloatOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.uName,o->uName);
	addString(&defines,"#define %s_DEFAULT   %g%sf\n",name,o->data.asFloat.def,
			(o->data.asFloat.def == floor(o->data.asFloat.def))?".0":"");
	addString(&defines,"#define %s_MIN       %g%sf\n",name,o->data.asFloat.min,
			(o->data.asFloat.min == floor(o->data.asFloat.min))?".0":"");
	addString(&defines,"#define %s_MAX       %g%sf\n",name,o->data.asFloat.max,
			(o->data.asFloat.max == floor(o->data.asFloat.max))?".0":"");
	addString(&defines,"#define %s_PRECISION %g%sf\n\n",name,o->data.asFloat.precision,
			(o->data.asFloat.precision == floor(o->data.asFloat.precision))?".0":"");

	addString(out,"\to->value.f = %s_DEFAULT;\n",name);
	addString(out,"\to->rest.f.min = %s_MIN;\n",name);
	addString(out,"\to->rest.f.max = %s_MAX;\n",name);
	addString(out,"\to->rest.f.precision = %s_PRECISION;\n",name);

	addGetFunction(o,"float","",".value.f","");
}

void addBoolOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.uName,o->uName);
	addString(&defines,"#define %s_DEFAULT %s\n\n",name,(o->data.asBool.def)?"TRUE":"FALSE");
	addString(out,"\to->value.b = %s_DEFAULT;\n",name);

	addGetFunction(o,"Bool","",".value.b","");
}

void addMatchOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.uName,o->uName);
	addString(&defines,"#define %s_DEFAULT \"%s\"\n\n",name,o->data.asString.def);

	addString(out,"\tmatchInit (&o->value.match);\n");
    addString(out,"\tmatchAddFromString (&o->value.match, %s_DEFAULT);\n",name);

	addGetFunction(o,"CompMatch *","",".value.match","&");

	if (o->screen)
	{
		addString(&initScreen,"\tmatchUpdate (s->display, &os->opt[%s].value.match);\n",o->temp);
		addString(&finiScreen,"\tmatchFini(&os->opt[%s].value.match);\n",o->temp);
	}
	else
	{
		addString(&initDisplay,"\tmatchUpdate (d, &od->opt[%s].value.match);\n",o->temp);
		addString(&finiDisplay,"\tmatchFini(&od->opt[%s].value.match);\n",o->temp);
	}
}

void addStringOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.uName,o->uName);
	addString(&defines,"#define %s_DEFAULT \"%s\"\n\n",name,o->data.asString.def);
	addString(out,"\to->value.s = strdup(%s_DEFAULT);\n",name);
	addString(out,"\to->rest.s.string = NULL;\n");
	addString(out,"\to->rest.s.nString = 0;\n");

	addGetFunction(o,"char *","",".value.s","");
}

void addStringListOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;

	OptionValuesList *l;
	int i = 0;

	l = o->data.asSList.begin;
	i = 0;
	while (l)
	{
		i++;
		l = l->next;
	}

	if (i)
	{
		addString(&defines,"static const char *%s%s[] = {\n",data.name,o->fUName);

		l = o->data.asSList.begin;
		while (l)
		{
			if (l->raw)
				addString(&defines,"\t%s%s\n",l->value,(l->next)?",":"");
			else
				addString(&defines,"\t\"%s\"%s\n",l->value,(l->next)?",":"");
			l = l->next;
		}

		addString(&defines,"};\n",data.name,o->fUName);

		addString(&defines,"#define N_%s_%s (sizeof(%s%s) / sizeof(%s%s[0]))\n\n",
				data.uName,o->uName,data.name,o->fUName,data.name,o->fUName);

		addString(out,"\to->value.list.type = CompOptionTypeString;\n");
		addString(out,"\to->value.list.nValue = N_%s_%s;\n",data.uName,o->uName);
		addString(out,"\to->value.list.value = "
				"malloc(sizeof(CompOptionValue) * N_%s_%s);\n",
				data.uName,o->uName);

		addString(out,"\tfor (i = 0; i < N_%s_%s; i++)\n"
				"\t\to->value.list.value[i].s = strdup(%s%s[i]);\n",
				data.uName,o->uName,data.name,o->fUName);
	}
	else
	{
		addString(out,"\to->value.list.type = CompOptionTypeString;\n");
		addString(out,"\to->value.list.nValue = 0;\n");
		addString(out,"\to->value.list.value = NULL;\n");
	}

	addString(out,"\to->rest.s.string = NULL;\n");
	addString(out,"\to->rest.s.nString = 0;\n");
}


void addEnumOption(Option *o)
{

	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;

	OptionValuesList *l;
	int i = 0;
	int def = 0;

	addString(&hdefines,"typedef enum {\n");

	l = o->data.asSList.begin;
	i = 0;

	char *defname = NULL;

	while (l)
	{
		addString(&hdefines,"\t%s%s,\n",o->fUName,l->fUName);
		if (l->def || !defname)
		{
			def = i;
			defname = l->fUName;
		}
		l = l->next;
		i++;
	}
	addString(&hdefines,"\t%sNum\n",o->fUName);
	addString(&hdefines,"} %s%sEnum;\n\n",data.fUName,o->fUName);

	addString(&defines,"static const char *%s%s[] = {\n",data.name,o->fUName);

	l = o->data.asSList.begin;
	while (l)
	{
		addString(&defines,"\tN_(\"%s\")%s\n",l->value,(l->next)?",":"");
		l = l->next;
	}

	addString(&defines,"};\n\n",data.name,o->fUName);

	addString(&defines,"#define %s_%s_DEFAULT %s%s\n\n",data.uName,o->uName,o->fUName,defname);

	addString(out,"\to->value.s = strdup(%s%s[%s_%s_DEFAULT]);\n",
			data.name,o->fUName,data.uName,o->uName);

	addString(out,"\to->rest.s.string = (char **)%s%s;\n",data.name,o->fUName);

	addString(out,"\to->rest.s.nString = %sNum;\n",o->fUName);

	addGetFunction(o,"char *","AsString",".value.s","");

	addString((o->screen)?&screenStruct:&displayStruct,"\t%s%sEnum %s;\n",data.fUName,o->fUName,o->name);

	addString((o->screen)?&setScreenOpt:&setDisplayOpt,"\t\t\tint i;\n\t\t\tfor (i = 0; i < %sNum;i++)\n"
				"\t\t\t\tif (!strcmp(o->value.s, o->rest.s.string[i]))\n"
				"\t\t\t\t\t%s->%s = i;\n",o->fUName,(o->screen)?"os":"od",
				o->name);

	addString((o->screen)?&initScreen:&initDisplay,"\t%s->%s = %s%s;\n",(o->screen)?"os":"od",
			o->name,o->fUName,defname);

	char str[1024];

	sprintf(str,"%s%sEnum",data.fUName,o->fUName);

	addString(&hfunctions,"%-16s %sGet%s(%s);\n",str,data.name,o->fUName,
		   (o->screen)?"CompScreen *s":"CompDisplay *d");
	addString(&functions,"%s %sGet%s(%s)\n",str,data.name,o->fUName,
			(o->screen)?"CompScreen *s":"CompDisplay *d");

	if (o->screen)
		addString(&functions,"{\n\t%s_OPTIONS_SCREEN(s);\n\treturn os->%s;\n"
		   		"}\n\n",data.uName,o->name);
	else
		addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n\treturn od->%s;\n"
		   		"}\n\n",data.uName,o->name);
}


void addSelectionOption(Option *o)
{

	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;

	OptionValuesList *l;
	int i = 0;
	int def = 0;

	addString(&defines,"static const char *%s%s[] = {\n",data.name,o->fUName);

	l = o->data.asSList.begin;
	i = 0;
	while (l)
	{
		addString(&defines,"\tN_(\"%s\")%s\n",l->value,(l->next)?",":"");
		addString(&hdefines,"#define %s%sMask (1 << %d)\n",o->fUName,l->fUName,i);
		if (l->def)
			def++;
		i++;
		l = l->next;
	}
	addString(&hdefines,"\n");

	addString(&defines,"};\n",data.name,o->fUName);

	addString(&defines,"#define N_%s_%s (sizeof(%s%s) / sizeof(%s%s[0]))\n\n",
			data.uName,o->uName,data.name,o->fUName,data.name,o->fUName);

	if (def)
	{
		addString(&defines,"static char *%s%sDefault[] = {\n",data.name,o->fUName);

		int def_c = def;
		l = o->data.asSList.begin;
		while (l)
		{
			if (l->def)
			{
				def_c--;
				addString(&defines,"\tN_(\"%s\")%s\n",l->value,(def_c)?",":"");
			}
			l = l->next;
		}

		addString(&defines,"};\n",data.name,o->fUName);

		addString(&defines,"#define N_%s_%s_DEFAULT (sizeof(%s%sDefault)"
 				"/ sizeof(%s%sDefault[0]))\n\n",
				data.uName,o->uName,data.name,o->fUName,data.name,o->fUName);

		addString(out,"\to->value.list.type = CompOptionTypeString;\n");
		addString(out,"\to->value.list.nValue = N_%s_%s_DEFAULT;\n",
				data.uName,o->uName);
		addString(out,"\to->value.list.value = "
				"malloc(sizeof(CompOptionValue) * N_%s_%s_DEFAULT);\n",
				data.uName,o->uName);

		addString(out,"\tfor (i = 0; i < N_%s_%s_DEFAULT; i++)\n"
				"\t\to->value.list.value[i].s = strdup(%s%sDefault[i]);\n",
				data.uName,o->uName,data.name,o->fUName);
	}
	else
	{
		addString(out,"\to->value.list.type = CompOptionTypeString;\n");
		addString(out,"\to->value.list.nValue = 0;\n");
		addString(out,"\to->value.list.value = NULL;\n");
	}

	addString(out,"\to->rest.s.string = (char **)%s%s;\n",data.name,o->fUName);
	addString(out,"\to->rest.s.nString = N_%s_%s;\n",data.uName,o->uName);

	addString((o->screen)?&screenStruct:&displayStruct,"\tunsigned int %s;\n",o->name);

	char obj[4];
	sprintf(obj,"%s",(o->screen)?"os":"od");

	addString((o->screen)?&setScreenOpt:&setDisplayOpt,
			"\t\t\tint i;\n\t\t\tint j;\n\t\t\t%s->%s = 0;\n"
			"\t\t\tfor (i = 0; i < o->value.list.nValue; i++)\n"
			"\t\t\t\tfor (j = 0; j < N_%s_%s; i++)\n"
			"\t\t\t\t\tif (!strcmp(o->value.list.value[i].s, "
			"o->rest.s.string[j]))\n\t\t\t\t\t\t%s->%s |= (1 << j);\n",
			obj,o->name,data.uName,o->uName,obj,o->name);

	if (!def)
	{
		addString((o->screen)?&initScreen:&initDisplay,
				   "\t%s->%s = 0;\n",obj,o->name);
	}

	l = o->data.asSList.begin;
	i = 0;
	while (l)
	{
		if (l->def)
		{
			addString((o->screen)?&initScreen:&initDisplay,
					"\t%s->%s %s= %s%sMask;\n",obj,o->name,
					(i)?"|":" ",o->fUName,l->fUName);
			i++;
		}
		l = l->next;
	}

	addString(&hfunctions,"unsigned int     %sGet%s(%s);\n",data.name,o->fUName,
		   (o->screen)?"CompScreen *s":"CompDisplay *d");
	addString(&functions,"unsigned int %sGet%s(%s)\n",data.name,o->fUName,
			(o->screen)?"CompScreen *s":"CompDisplay *d");

	if (o->screen)
		addString(&functions,"{\n\t%s_OPTIONS_SCREEN(s);\n\treturn os->%s;\n"
		   		"}\n\n",data.uName,o->name);
	else
		addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n\treturn od->%s;\n"
		   		"}\n\n",data.uName,o->name);
}


void addActionOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;

	int i = 0;

	addString(out,"\to->value.action.initiate = NULL;\n");
	addString(out,"\to->value.action.terminate = NULL;\n");

	addString(out,"\to->value.action.bell = %s;\n",
			(o->data.asBind.sbell && o->data.asBind.bell)?"TRUE":"FALSE");

	if (o->data.asBind.sedge && o->data.asBind.edge)
	{
		int num_e = 0;
		for (i = 0; i < SCREEN_EDGE_NUM; i++)
		{
			if (o->data.asBind.edge & (1 << i))
			{
				addString(out,"\to->value.action.edgeMask %s= (1 << %s);\n",
				(num_e)?"|":"",edges[i]);
				num_e++;
			}
		}
	}
	else
		addString(out,"\to->value.action.edgeMask = 0;\n");

	if (o->data.asBind.state)
	{
		int num_s = 0;
		for (i = 0; i < N_ACTION_STATES; i++)
		{
			if (o->data.asBind.state & (1 << i))
			{
				addString(out,"\to->value.action.state %s= %s;\n",
				(num_s)?"|":"",actionStates[i]);
				num_s++;
			}
		}
	}
	else
		addString(out,"\to->value.action.state = 0;\n");

	if (o->data.asBind.type)
	{
		int num_t = 0;
		for (i = 0; i < N_BINDING_TYPES; i++)
		{
			if (o->data.asBind.type & (1 << i))
			{
				addString(out,"\to->value.action.type %s= %s;\n",
				(num_t)?"|":"",bindingTypes[i]);
				num_t++;
			}
		}
	}
	else
		addString(out,"\to->value.action.type = CompBindingTypeNone;\n");


	if (o->data.asBind.skey)
	{
		char *mods = NULL;
		char *key = NULL;
		convertBinding(o->data.asBind.key,&mods,&key);
		if (mods)
		{
			addString(&defines,"#define %s_%s_KEY_MODIFIERS_DEFAULT %s\n",
					data.uName,o->uName,mods);
			addString(out,"\to->value.action.key.modifiers = "
					"%s_%s_KEY_MODIFIERS_DEFAULT;\n",data.uName,o->uName);
		}
		else
			addString(out,"\to->value.action.key.modifiers = 0;\n");

		if (key)
		{
			addString(&defines,"#define %s_%s_KEY_DEFAULT \"%s\"\n",
					data.uName,o->uName,key);
			addString(out,"\to->value.action.key.keycode = \n"
					"\t\tXKeysymToKeycode (display,\n"
					"\n\nXStringToKeysym(%s_%s_KEY_DEFAULT));\n",
					data.uName,o->uName);
		}
		else
		{
			addString(out,"\to->value.action.key.keycode = 0;\n");
		}
	}

	if (o->data.asBind.smouse)
	{
		char *mods = NULL;
		char *button = NULL;
		convertBinding(o->data.asBind.mouse,&mods,&button);
		if (mods)
		{
			addString(&defines,"#define %s_%s_BUTTON_MODIFIERS_DEFAULT %s\n",
					data.uName,o->uName,mods);
			addString(out,"\to->value.action.button.modifiers = "
					"%s_%s_BUTTON_MODIFIERS_DEFAULT;\n",data.uName,o->uName);
		}
		else
			addString(out,"\to->value.action.button.modifiers = 0;\n");

		if (button)
		{
			addString(&defines,"#define %s_%s_BUTTON_DEFAULT %s\n",
					data.uName,o->uName,button);
			addString(out,"\to->value.action.button.button = "
					"%s_%s_BUTTON_DEFAULT;\n",data.uName,o->uName);
		}
		else
			addString(out,"\to->value.action.button.button = 0;\n");
	}

	addGetFunction(o,"CompAction *","",".value.action","&");

	addString(&hfunctions,"void             %sSet%sInitiate"
			"(CompDisplay *d, CompActionCallBackProc init);\n",data.name,o->fUName);
	addString(&functions,"void %sSet%sInitiate"
			"(CompDisplay *d, CompActionCallBackProc init)\n",data.name,o->fUName);

	addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n"
			"\tod->opt[%s].value.action.initiate = init;\n"
		   		"}\n\n",data.uName,o->temp);

	addString(&hfunctions,"void             %sSet%sTerminate"
			"(CompDisplay *d, CompActionCallBackProc term);\n",data.name,o->fUName);
	addString(&functions,"void %sSet%sTerminate"
			"(CompDisplay *d, CompActionCallBackProc term)\n",data.name,o->fUName);

	addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n"
			"\tod->opt[%s].value.action.terminate = term;\n"
		   		"}\n\n",data.uName,o->temp);

	addString(&initScreen,"\taddScreenAction(s, &od->opt[%s].value.action);\n",o->temp);
	addString(&finiScreen,"\tremoveScreenAction(s, &od->opt[%s].value.action);\n",o->temp);
}

void addColorOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	char name[1024];

	sprintf(name,"%s_%s",data.fUName,o->uName);
	addString(&defines,"#define %s_RED_DEFAULT   0x%.4x\n",name,o->data.asColor.red);
	addString(&defines,"#define %s_GREEN_DEFAULT 0x%.4x\n",name,o->data.asColor.green);
	addString(&defines,"#define %s_BLUE_DEFAULT  0x%.4x\n",name,o->data.asColor.blue);
	addString(&defines,"#define %s_ALPHA_DEFAULT 0x%.4x\n\n",name,o->data.asColor.alpha);

	addString(out,"\to->value.c[0] = %s_RED_DEFAULT;\n",name);
	addString(out,"\to->value.c[1] = %s_GREEN_DEFAULT;\n",name);
	addString(out,"\to->value.c[2] = %s_BLUE_DEFAULT;\n",name);
	addString(out,"\to->value.c[3] = %s_ALPHA_DEFAULT;\n",name);

	addGetFunction(o,"unsigned short*","",".value.c","");
	addGetFunction(o,"unsigned short","Red",".value.c[0]","");
	addGetFunction(o,"unsigned short","Green",".value.c[1]","");
	addGetFunction(o,"unsigned short","Blue",".value.c[2]","");
	addGetFunction(o,"unsigned short","Alpha",".value.c[3]","");
}


void addOption(Option *o)
{
	StringElement *out = (o->screen)?&initScreenOpt:&initDisplayOpt;
	StringElement *outs = (o->screen)?&setScreenOpt:&setDisplayOpt;

	addString(out,"\to = &o%s->opt[%s];\n",(o->screen)?"s":"d",o->temp);
	addString(out,"\to->name = \"%s\";\n",o->name);

	if (o->shortDesc)
		addString(out,"\to->shortDesc = N_(\"%s\");\n",o->shortDesc);
	else
		addString(out,"\to->shortDesc = \"%s\";\n",o->name);

	if (o->longDesc)
		addString(out,"\to->longDesc = N_(\"%s\");\n",o->longDesc);
	else
	{
		if (o->shortDesc)
			addString(out,"\to->longDesc = N_(\"%s\");\n",o->shortDesc);
		else
			addString(out,"\to->longDesc = \"%s\";\n",o->name);
	}

	addString(outs,"\tcase %s:\n",o->temp);

	switch (o->type)
	{
		case OptionTypeInt:
			addString(outs,"\t\tif (compSetIntOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeInt;\n");
			addIntOption(o);
			break;
		case OptionTypeBool:
			addString(outs,"\t\tif (compSetBoolOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeBool;\n");
			addBoolOption(o);
			break;
		case OptionTypeFloat:
			addString(outs,"\t\tif (compSetFloatOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeFloat;\n");
			addFloatOption(o);
			break;
		case OptionTypeString:
			addString(outs,"\t\tif (compSetStringOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeString;\n");
			addStringOption(o);
			break;
		case OptionTypeStringList:
			addString(outs,"\t\tif (compSetOptionList(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeList;\n");
			addStringListOption(o);
			break;
		case OptionTypeEnum:
			addString(outs,"\t\tif (compSetStringOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeString;\n");
			addEnumOption(o);
			break;
		case OptionTypeSelection:
			addString(outs,"\t\tif (compSetOptionList(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeList;\n");
			addSelectionOption(o);
			break;
		case OptionTypeColor:
			addString(outs,"\t\tif (compSetColorOption(o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeColor;\n");
			addColorOption(o);
			break;
		case OptionTypeAction:
			addString(outs,"\t\tif (setDisplayAction(display, o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeAction;\n");
			addActionOption(o);
			break;
		case OptionTypeMatch:
			addString(outs,"\t\tif (compSetMatchOption (o, value))\n"
				"\t\t{\n");
			addString(out,"\to->type = CompOptionTypeMatch;\n");
			addMatchOption(o);
			break;
		default:
			break;
	}

	addString(outs,"\t\t\tif (%s->notify[%s])\n"
				"\t\t\t\t(*%s->notify[%s])(%s, o, %s);\n", (o->screen)?"os":"od",
				o->temp,(o->screen)?"os":"od",o->temp,
				(o->screen)?"screen":"display",o->temp);

	addString(outs,"\t\t\treturn TRUE;\n"
				"\t\t}\n"
				"\t\tbreak;\n");

	addGetFunction(o,"CompOption *","Option","","&");

	addString(&hfunctions,"void             %sSet%sNotify"
			"(%s, %s%sOptionChangeNotifyProc notify);\n",
			data.name,o->fUName,(o->screen)?"CompScreen *s":"CompDisplay *d",
			data.name,(o->screen)?"Screen":"Display");
	addString(&functions,"void %sSet%sNotify"
			"(%s, %s%sOptionChangeNotifyProc notify)\n",
			data.name,o->fUName,(o->screen)?"CompScreen *s":"CompDisplay *d",
			data.name,(o->screen)?"Screen":"Display");
	if (o->screen)
		addString(&functions,"{\n\t%s_OPTIONS_SCREEN(s);\n"
				"\tos->notify[%s] = notify;\n"
		   		"}\n\n",data.uName,o->temp);
	else
		addString(&functions,"{\n\t%s_OPTIONS_DISPLAY(d);\n"
				"\tod->notify[%s] = notify;\n"
		   		"}\n\n",data.uName,o->temp);

	addString(out,"\n");
	addString(&hfunctions,"\n");
}


void addDisplayOptions()
{
	char str[1024];

	addString(&displayStruct,"typedef struct _%sOptionsDisplay\n"
				"{\n",data.fUName);
	addString(&displayStruct,"\tint screenPrivateIndex;\n\n");

	if (nDOptions)
	{
		addString(&setDisplayOpt,"static Bool\n"
			"%sOptionsSetDisplayOption(CompDisplay * display, "
			"char *name, CompOptionValue * value)\n{\n"
			"\tCompOption *o;\n\tint index;\n\n"
			"\t%s_OPTIONS_DISPLAY(display);\n\n",data.name,data.uName);

		addString(&initDisplay,"\t%sOptionsDisplayInitOptions"
				"(od, d->display);\n\n", data.name);

		addString(&initDisplayOpt,"static void %sOptionsDisplayInitOptions"
				"(%sOptionsDisplay * od, Display *display)\n{\n"
				"\tCompOption *o;\n\tint i;\n\ti = 0;\n\n",
				data.name,data.fUName);

		addString(&hdefines,"typedef enum\n"
					"{\n");

		Option *o = data.options;
		int i = 0;
		while (o)
		{
			if (!o->screen)
			{
				addString(&hdefines,"\t%sDisplayOption%s%s\n",data.fUName,o->fUName,(i == 0)?" = 0,":",");
				sprintf(str,"%sDisplayOption%s",data.fUName,o->fUName);
				o->temp = strdup(str);
				i++;
			}
			o = o->next;
		}
		addString(&hdefines,"\t%sDisplayOptionNum\n",data.fUName);

		addString(&hdefines,"} %sDisplayOptions;\n\n",data.fUName);

		addString(&hdefines,"typedef void (*%sDisplayOptionChangeNotifyProc)"
				"(CompDisplay *display, CompOption *opt, %sDisplayOptions num);\n\n",
				data.name,data.fUName);

		addString(&displayStruct,"\tCompOption opt[%sDisplayOptionNum];\n",data.fUName);
		addString(&displayStruct,"\tCompOption *mOpt;\n",data.fUName);

		addString(&displayStruct,"\t%sDisplayOptionChangeNotifyProc "
				"notify[%sDisplayOptionNum];\n",
				data.name,data.fUName);

		addString(&setDisplayOpt,"\to = compFindOption(od->opt, %sDisplayOptionNum, name, "
				"&index);\n\n",data.fUName);

		addString(&setDisplayOpt,"\tif (!o)\n\t{\n"
				"\t\tif (%sPluginVTable && %sPluginVTable->setDisplayOption)\n"
				"\t\t\treturn (*%sPluginVTable->setDisplayOption)"
				"(display, name, value);\n\t\treturn FALSE;\n\t}\n\n",
			   data.name,data.name,data.name);

		addString(&setDisplayOpt,"\tswitch (index)\n\t{\n");

		o = data.options;
		while (o)
		{
			if (!o->screen)
			{
				addOption(o);
			}
			o = o->next;
		}

		addString(&setDisplayOpt,"\tdefault:\n\t\tbreak;\n\t}\n\treturn FALSE;\n}\n\n");

		addString(&initDisplayOpt,"}\n\n");


		addString(&setDisplayOpt,"static CompOption *%sOptionsGetDisplayOptions"
				"(CompDisplay * d, int *count)\n{\n"
				"\tCompOption *pOpt = NULL;\n\tint pOptNum = 0;\n"
				"\tif (%sPluginVTable && %sPluginVTable->getDisplayOptions)\n"
				"\t\tpOpt = %sPluginVTable->getDisplayOptions(d,&pOptNum);\n\n"
				"\t%sOptionsDisplay *od;\n"
				"\tod = GET_%s_OPTIONS_DISPLAY(d);\n"
				"\tif (!pOptNum)\n\t{\n\t\t*count = %sDisplayOptionNum;\n"
				"\t\treturn od->opt;\n\t}\n\n"
				"\tint sOptSize = sizeof(CompOption) * %sDisplayOptionNum;\n"
				"\tint pOptSize = sizeof(CompOption) * pOptNum;\n\n"
				"\tif (!od->mOpt)\n"
				"\t\tod->mOpt = malloc(sOptSize + pOptSize);\n\n"
				"\tmemcpy(od->mOpt,od->opt,sOptSize);\n"
				"\tmemcpy(od->mOpt + %sDisplayOptionNum,pOpt,pOptSize);\n\n"
				"\t*count = %sDisplayOptionNum + pOptNum;\n"
				"\treturn od->mOpt;\n}\n\n",
				data.name,data.name,data.name,data.name,data.fUName,data.uName,
				data.fUName,data.fUName,data.fUName,data.fUName);

	}

	addString(&displayStruct,"} %sOptionsDisplay;\n\n",data.fUName);
}

void addScreenOptions()
{
	char str[1024];

	addString(&screenStruct,"typedef struct _%sOptionsScreen\n"
				"{\n",data.fUName);

	if (nSOptions)
	{
		addString(&setScreenOpt,"static Bool\n"
			"%sOptionsSetScreenOption(CompScreen * screen, "
			"char *name, CompOptionValue * value)\n{\n"
			"\tCompOption *o;\n\tint index;\n\n"
			"\t%s_OPTIONS_SCREEN(screen);\n\n",data.name,data.uName);

		addString(&initScreen,"\t%sOptionsScreenInitOptions(os);\n\n",data.name);

		addString(&initScreenOpt,"static void %sOptionsScreenInitOptions"
				"(%sOptionsScreen * os)\n{\n"
				"\tCompOption *o;\n\tint i;\n\ti = 0;\n\n",
				data.name,data.fUName);

		addString(&hdefines,"typedef enum\n"
					"{\n");

		Option *o = data.options;
		int i = 0;
		while (o)
		{
			if (o->screen)
			{
				addString(&hdefines,"\t%sScreenOption%s%s\n",data.fUName,o->fUName,(i == 0)?" = 0,":",");
				sprintf(str,"%sScreenOption%s",data.fUName,o->fUName);
				o->temp = strdup(str);
				i++;
			}
			o = o->next;
		}
		addString(&hdefines,"\t%sScreenOptionNum\n",data.fUName);

		addString(&hdefines,"} %sScreenOptions;\n\n",data.fUName);

		addString(&hdefines,"typedef void (*%sScreenOptionChangeNotifyProc)"
				"(CompScreen *screen, CompOption *opt, %sScreenOptions num);\n\n",
				data.name,data.fUName);

		addString(&screenStruct,"\tCompOption opt[%sScreenOptionNum];\n",data.fUName);
		addString(&screenStruct,"\tCompOption *mOpt;\n",data.fUName);

		addString(&screenStruct,"\t%sScreenOptionChangeNotifyProc "
				"notify[%sScreenOptionNum];\n",
				data.name,data.fUName);

		addString(&setScreenOpt,"\to = compFindOption(os->opt, %sScreenOptionNum, name, "
				"&index);\n\n",data.fUName);

		addString(&setScreenOpt,"\tif (!o)\n\t{\n"
				"\t\tif (%sPluginVTable && %sPluginVTable->setScreenOption)\n"
				"\t\t\treturn (*%sPluginVTable->setScreenOption)"
				"(screen, name, value);\n\t\treturn FALSE;\n\t}\n\n",
				data.name,data.name,data.name);

		addString(&setScreenOpt,"\tswitch (index)\n\t{\n");

		o = data.options;
		i = 0;
		while (o)
		{
			if (o->screen)
			{
				addOption(o);
				i++;
			}
			o = o->next;
		}

		addString(&setScreenOpt,"\tdefault:\n\t\tbreak;\n\t}\n\treturn FALSE;\n}\n\n");

		addString(&initScreenOpt,"}\n\n");

		addString(&setScreenOpt,"static CompOption *%sOptionsGetScreenOptions"
				"(CompScreen * s, int *count)\n{\n"
				"\tCompOption *pOpt = NULL;\n\tint pOptNum = 0;\n"
				"\tif (%sPluginVTable && %sPluginVTable->getScreenOptions)\n"
				"\t\tpOpt = %sPluginVTable->getScreenOptions(s,&pOptNum);\n\n"
				"\t%sOptionsScreen *os;\n"
				"\tif (s)\n\t\tos = GET_%s_OPTIONS_SCREEN(s, "
				"GET_%s_OPTIONS_DISPLAY(s->display));\n"
				"\telse\n\t{\n\t\tos = calloc(1,sizeof(%sOptionsScreen));\n"
				"\t\t%sOptionsScreenInitOptions(os);\n\t}\n\n"
				"\tif (!pOptNum)\n\t{\n\t\t*count = %sScreenOptionNum;\n"
				"\t\treturn os->opt;\n\t}\n\n"
				"\tint sOptSize = sizeof(CompOption) * %sScreenOptionNum;\n"
				"\tint pOptSize = sizeof(CompOption) * pOptNum;\n\n"
				"\tif (!os->mOpt)\n"
				"\t\tos->mOpt = malloc(sOptSize + pOptSize);\n\n"
				"\tmemcpy(os->mOpt,os->opt,sOptSize);\n"
				"\tmemcpy(os->mOpt + %sScreenOptionNum,pOpt,pOptSize);\n\n"
				"\t*count = %sScreenOptionNum + pOptNum;\n"
				"\treturn os->mOpt;\n}\n\n",
				data.name,data.name,data.name,data.name,data.fUName,data.uName,data.uName,
				data.fUName,data.name,data.fUName,data.fUName,data.fUName,data.fUName);
	}

	addString(&screenStruct,"} %sOptionsScreen;\n\n",data.fUName);
}

void finilizeCode()
{

	addString(&file,"#include <stdio.h>\n#include <stdlib.h>\n"
			"#include <string.h>\n\n");

	addString(&file,"#include <compiz.h>\n\n");

	addString(&file,"#define _%s_OPTIONS_INTERNAL\n"
			"#include \"%s\"\n\n",
			data.uName,hFile);

	addString(&file,"static int displayPrivateIndex;\n\n");

	addString(&file,"static CompPluginVTable *%sPluginVTable = NULL;\n"
			"CompPluginVTable %sOptionsVTable;\n\n",data.name,data.name);

	addString(&file,"#define GET_%s_OPTIONS_DISPLAY(d) \\\n"
			"\t((%sOptionsDisplay *) "
			"(d)->privates[displayPrivateIndex].ptr)\n\n",data.uName,data.fUName);

	addString(&file,"#define %s_OPTIONS_DISPLAY(d) \\\n"
			"\t%sOptionsDisplay *od = GET_%s_OPTIONS_DISPLAY (d)\n\n",
			data.uName,data.fUName,data.uName);

	addString(&file,"#define GET_%s_OPTIONS_SCREEN(s, od)	\\\n"
			"\t((%sOptionsScreen *) "
			"(s)->privates[(od)->screenPrivateIndex].ptr)\n\n",data.uName,data.fUName);

	addString(&file,"#define %s_OPTIONS_SCREEN(s) \\\n"
			"\t%sOptionsScreen *os = GET_%s_OPTIONS_SCREEN "
			"(s, GET_%s_OPTIONS_DISPLAY (s->display))\n\n",
			data.uName,data.fUName,data.uName,data.uName);

	addString(&file,displayStruct.data);
	addString(&file,screenStruct.data);

	addString(&file,defines.data);

	addString(&file,functions.data);

	addString(&file,initScreenOpt.data);
	addString(&file,initDisplayOpt.data);

	addString(&file,setScreenOpt.data);
	addString(&file,setDisplayOpt.data);

	addString(&file,"static Bool %sOptionsInitScreen"
			"(CompPlugin * p, CompScreen * s)\n{\n\t%sOptionsScreen *os;\n\n"
			"\t%s_OPTIONS_DISPLAY(s->display);\n\n"
			"\tos = calloc(1,sizeof(%sOptionsScreen));\n\tif (!os)\n"
			"\t\treturn FALSE;\n\n"
			"\ts->privates[od->screenPrivateIndex].ptr = os;\n\n",
			data.name,data.fUName,data.uName,data.fUName);

	addString(&file,initScreen.data);

	addString(&file,"\n\tif (%sPluginVTable && %sPluginVTable->initScreen)\n"
			"\t\treturn %sPluginVTable->initScreen(p,s);\n"
			"\treturn TRUE;\n}\n\n",
			data.name,data.name,data.name);

	addString(&file,"static void %sOptionsFiniScreen"
			"(CompPlugin * p, CompScreen * s)\n{\n"
			"\tif (%sPluginVTable && %sPluginVTable->finiScreen)\n"
			"\t\t%sPluginVTable->finiScreen(p,s);\n\n"
			"\t%s_OPTIONS_SCREEN(s);\n\t%s_OPTIONS_DISPLAY(s->display);\n\n",
			data.name,data.name,data.name,data.name,data.uName,data.uName);

	addString(&file,finiScreen.data);

	addString(&file,"\tfree(os);\n\tod = NULL;\n}\n\n");

	addString(&file,"static Bool %sOptionsInitDisplay"
			"(CompPlugin * p, CompDisplay * d)\n{\n\t%sOptionsDisplay *od;\n\n"
			"\tod = calloc(1,sizeof(%sOptionsDisplay));\n\tif (!od)\n"
			"\t\treturn FALSE;\n\n"
			"\tod->screenPrivateIndex = allocateScreenPrivateIndex(d);\n"
			"\tif (od->screenPrivateIndex < 0)\n\t{\n\t\tfree(od);\n"
			"\t\treturn FALSE;\n\t}\n\n"
			"\td->privates[displayPrivateIndex].ptr = od;\n\n",
			data.name,data.fUName,data.fUName);

	addString(&file,initDisplay.data);

	addString(&file,"\n\tif (%sPluginVTable && %sPluginVTable->initDisplay)\n"
			"\t\treturn %sPluginVTable->initDisplay(p,d);\n"
			"\treturn TRUE;\n}\n\n",
			data.name,data.name,data.name);

	addString(&file,"static void %sOptionsFiniDisplay"
			"(CompPlugin * p, CompDisplay * d)\n{\n"
			"\tif (%sPluginVTable && %sPluginVTable->finiDisplay)\n"
			"\t\t%sPluginVTable->finiDisplay(p,d);\n\n"
			"\t%s_OPTIONS_DISPLAY(d);\n\n"
			"\tfreeScreenPrivateIndex(d, od->screenPrivateIndex);\n\n",
			data.name,data.name,data.name,data.name,data.uName);

	addString(&file,finiDisplay.data);

	addString(&file,"\tfree(od);\n}\n\n");

	addString(&file,"static Bool %sOptionsInit(CompPlugin * p)\n{\n"
			"\tdisplayPrivateIndex = allocateDisplayPrivateIndex();\n"
			"\tif (displayPrivateIndex < 0)\n\t\treturn FALSE;\n"
			"\tif (%sPluginVTable && %sPluginVTable->init)\n"
			"\t\treturn %sPluginVTable->init(p);\n\treturn TRUE;\n}\n\n",
			data.name,data.name,data.name,data.name);

	addString(&file,"static void %sOptionsFini(CompPlugin * p)\n{\n"
			"\tif (%sPluginVTable && %sPluginVTable->fini)\n"
			"\t\t%sPluginVTable->fini(p);\n"
			"\tif (displayPrivateIndex >= 0)\n"
			"\t\tfreeDisplayPrivateIndex(displayPrivateIndex);\n}\n\n",
			data.name,data.name,data.name,data.name);

	addString(&file,"CompPluginVTable *getCompPluginInfo(void)\n{\n"
			"\tif (!%sPluginVTable)\n\t{\n"
			"\t\t%sPluginVTable = %sOptionsGetCompPluginInfo();\n"
			"\t\tmemcpy(&%sOptionsVTable,%sPluginVTable,"
			"sizeof(CompPluginVTable));\n",
			data.name,data.name,data.name,data.name,data.name);

	addString(&file,"\t\t%sOptionsVTable.init = %sOptionsInit;\n"
			"\t\t%sOptionsVTable.fini = %sOptionsFini;\n"
			"\t\t%sOptionsVTable.initDisplay = %sOptionsInitDisplay;\n"
			"\t\t%sOptionsVTable.finiDisplay = %sOptionsFiniDisplay;\n"
			"\t\t%sOptionsVTable.initScreen = %sOptionsInitScreen;\n"
			"\t\t%sOptionsVTable.finiScreen = %sOptionsFiniScreen;\n",
			data.name,data.name,data.name,data.name,data.name,data.name,
			data.name,data.name,data.name,data.name,data.name,data.name);

	if (nSOptions)
	{
		addString(&file,"\t\t%sOptionsVTable.getScreenOptions = "
				"%sOptionsGetScreenOptions;\n"
				"\t\t%sOptionsVTable.setScreenOption = "
				"%sOptionsSetScreenOption;\n",
				data.name,data.name,data.name,data.name);
	}

	if (nDOptions)
	{
		addString(&file,"\t\t%sOptionsVTable.getDisplayOptions = "
				"%sOptionsGetDisplayOptions;\n"
				"\t\t%sOptionsVTable.setDisplayOption = "
				"%sOptionsSetDisplayOption;\n",
				data.name,data.name,data.name,data.name);
	}

	addString(&file,"\t}\n\treturn &%sOptionsVTable;\n}\n",data.name);

	addString(&header,"#ifndef _%s_OPTIONS_H\n"
				"#define _%s_OPTIONS_H\n\n",data.uName,data.uName);

	addString(&header,"#ifndef _%s_OPTIONS_INTERNAL\n"
			"#define getCompPluginInfo %sOptionsGetCompPluginInfo\n"
			"#endif\n\n",data.uName,data.name);

	addString(&header,"CompPluginVTable *%sOptionsGetCompPluginInfo(void);\n\n"
			,data.name);

	addString(&header,hdefines.data);
	addString(&header,hfunctions.data);
	addString(&header,"#endif\n\n");
}


static int writeFiles(char *src, char *hdr)
{

	if (src)
	{
		if (!quiet) printf("Creating \"%s\"...",src);

		FILE *f = fopen(src,"w");
		if (!f)
		{
			if (!quiet) printf("error\n");
			fprintf(stderr,"Unable to write \"%s\"!\n",src);
			return 1;
		}
		fprintf(f,"%s",file.data);
		fclose(f);
		if (!quiet) printf("done\n");
	}

	if (hdr)
	{
		if (!quiet) printf("Creating \"%s\"...",hdr);

		FILE *f = fopen(hdr,"w");
		if (!f)
		{
			if (!quiet) printf("error\n");
			fprintf(stderr,"Unable to write \"%s\"!\n",hdr);
			return 1;
		}
		fprintf(f,"%s",header.data);
		fclose(f);
		if (!quiet) printf("done\n");
	}
	return 0;
}

void addFileHeaders()
{
	char str[1024];
	sprintf(str,"/*\n"
			" *\n"
			" * This file is autogenerated from :\n"
			" * %s with the Beryl/Compiz optionss parser (%s)\n"
			" *\n"
			" * This program is distributed in the hope that it will be useful,\n"
			" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			" * GNU General Public License for more details.\n"
			" *\n"
			" */\n\n",
			fileName,basename(programName));
	addString(&file,str);
	addString(&header,str);
}

int genCode(char *src, char *hdr)
{
	initStrings();
	Option *o = data.options;
	while (o)
	{
		nOptions++;
		if (o->screen)
			nSOptions++;
		else
			nDOptions++;
		o = o->next;
	}

	if (hdr)
		hFile = hdr;
	else
	{
		char str[1024];
		char *prefix = strtok(basename(strdup(fileName)),".");
		if (!prefix || !strlen(prefix))
			prefix = data.name;
		sprintf(str,"%s_options.h",prefix);
		hFile = strdup(str);
	}

	if (!quiet) printf("Generating code for %d settings...",nOptions);

	addFileHeaders();
	addDisplayOptions();
	addScreenOptions();
	finilizeCode();

	if (!quiet) printf("done\n");

	return writeFiles(src, hdr);
}
