/*
 *
 * Compiz bcop dump plugin
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@beryl-project.org
 *
 * Based on gconf-dump plugin (David Revemann <davidr@novel.com>)
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

/*
 * To generate a bcop file for third-party plugins, do something
 * like:
 *
 *   COMPIZ_BCOP_PLUGINS="plugin1 plugin2" \
 *   compiz --replace dep1 plugin1 dep2 dep3 plugin2 bcop-dump
 *
 * COMPIZ_BCOP_PLUGINS indicates the plugins to generate bcop
 * info for, which might be a subset of the plugins listed on the
 * compiz command line. (Eg, if your plugin depends on "cube", you
 * need to list that on the command line, but don't put in the
 * environment variable, because you don't want to dump the cube bcop
 * into your plugin's bcop file.)
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <compiz.h>
#undef _
#undef N_

static FILE *schemaFile;

static int displayPrivateIndex;

typedef struct _BcopDumpDisplay {
    HandleEventProc handleEvent;
} BcopDumpDisplay;

#define GET_BCOP_DUMP_DISPLAY(d)				  \
    ((BcopDumpDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define BCOP_DUMP_DISPLAY(d)			      \
    BcopDumpDisplay *bd = GET_BCOP_DUMP_DISPLAY (d)

/* Decimal point character */
#define DECIMAL_POINT _(".")

#define TYPE_ERROR 0
#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_BOOL 3
#define TYPE_STRING 4
#define TYPE_STRINGLIST 5
#define TYPE_ENUM 6
#define TYPE_SELECTION 7
#define TYPE_COLOR 8
#define TYPE_MATCH 9
#define TYPE_ACTION 10

const static char * bcopTypes[] = {
    "",
    "int",
    "float",
    "bool",
    "string",
    "stringlist",
    "enum",
    "selection",
    "color",
    "match",
    "action"
};

static int
bcopGetType (CompOption *o)
{
    switch (o->type)
    {
    case CompOptionTypeInt:
	return TYPE_INT;
    case CompOptionTypeFloat:
	return TYPE_FLOAT;
    case CompOptionTypeBool:
	return TYPE_BOOL;
    case CompOptionTypeColor:
	return TYPE_COLOR;
    case CompOptionTypeMatch:
	return TYPE_MATCH;
    case CompOptionTypeAction:
	return TYPE_ACTION;
    case CompOptionTypeString:
	if (o->rest.s.nString)
	    return TYPE_ENUM;
	else
	    return TYPE_STRING;
    case CompOptionTypeList:
	if (o->value.list.type == CompOptionTypeString)
	{
	    if (o->rest.s.nString)
		return TYPE_SELECTION;
	    else
		return TYPE_STRINGLIST;
	    }
	else
	    return TYPE_ERROR;
    default:
	break;
    }
    return TYPE_ERROR;
}

static void
bcopPrintf (int level, char *format, ...)
{
    va_list args;

    if (level > 0) {
        int i;

        for (i = 0; i < level; i++)
            fprintf (schemaFile, "\t");
    }

    va_start (args, format);
    vfprintf (schemaFile, format, args);
    va_end (args);
}



static void
dumpIntValue (CompDisplay *d, CompOption *o)
{
    bcopPrintf(3,"<default>%d</default>\n",o->value.i);
    bcopPrintf(3,"<min>%d</min>\n",o->rest.i.min);
    bcopPrintf(3,"<max>%d</max>\n",o->rest.i.max);
}

static void
dumpFloatValue (CompDisplay *d, CompOption *o)
{
    bcopPrintf(3,"<default>%g</default>\n",o->value.f);
    bcopPrintf(3,"<min>%g</min>\n",o->rest.f.min);
    bcopPrintf(3,"<max>%g</max>\n",o->rest.f.max);
    bcopPrintf(3,"<precision>%g</precision>\n",o->rest.f.precision);
}

static void
dumpBoolValue (CompDisplay *d, CompOption *o)
{
    bcopPrintf(3,"<default>%s</default>\n",(o->value.b)? "true" : "false");
}

static void
dumpStringValue (CompDisplay *d, CompOption *o)
{
    if (o->value.s)
        bcopPrintf(3,"<default>%s</default>\n",o->value.s);
}

static void
dumpStringListValue (CompDisplay *d, CompOption *o)
{
    int i;
    for (i = 0; i < o->value.list.nValue; i++)
	bcopPrintf(3,"<default>%s</default>\n",o->value.list.value[i].s);
}

static void
dumpEnumValue (CompDisplay *d, CompOption *o)
{
    if (o->value.s)
        bcopPrintf(3,"<default>%s</default>\n",o->value.s);
    int i;
    for (i = 0; i < o->rest.s.nString; i++)
	bcopPrintf(3,"<value name=\"value%d\">%s</default>\n",
		   i,o->rest.s.string[i]);
}

static void
dumpSelectionValue (CompDisplay *d, CompOption *o)
{
    int i,j;
    char *def = "";
    for (i = 0; i < o->rest.s.nString; i++)
    {
	def = "";
	for (j = 0; j < o->value.list.nValue; j++)
	    if (!strcmp(o->rest.s.string[i], o->value.list.value[j].s))
	       def = " default=\"true\"";
	bcopPrintf(3,"<value name=\"value%d\"%s>%s</default>\n",
		   i,def,o->rest.s.string[i]);
    }
}

static void
dumpColorValue (CompDisplay *d, CompOption *o)
{
    bcopPrintf(3,"<red>0x%.4x</red>\n",o->value.c[0]);
    bcopPrintf(3,"<green>0x%.4x</green>\n",o->value.c[1]);
    bcopPrintf(3,"<blue>0x%.4x</blue>\n",o->value.c[2]);
    bcopPrintf(3,"<alpha>0x%.4x</alpha>\n",o->value.c[3]);
}

static void
dumpMatchValue (CompDisplay *d, CompOption *o)
{
    char *match = matchToString(&o->value.match);
    if (match)
        bcopPrintf(3,"<default>%s</default>\n",match);
}

struct _BcopMod {
    char *name;
    int  modifier;
} bcopMod[] = {
    { "Shift",      ShiftMask		 },
    { "Control",    ControlMask	 },
    { "Mod1",	      Mod1Mask		 },
    { "Mod2",	      Mod2Mask		 },
    { "Mod3",	      Mod3Mask		 },
    { "Mod4",	      Mod4Mask		 },
    { "Mod5",	      Mod5Mask		 },
    { "Alt",	      CompAltMask        },
    { "Meta",	      CompMetaMask       },
    { "Super",      CompSuperMask      },
    { "Hyper",      CompHyperMask	 },
    { "ModeSwitch", CompModeSwitchMask },
};

#define N_MOD (sizeof (bcopMod) / sizeof (struct _BcopMod))

struct _BcopEdge {
    char *name;
    unsigned int  mask;
} bcopEdge[] = {
    { "left",       (1 << 0)},
    { "right",      (1 << 1)},
    { "top",	    (1 << 2)},
    { "bottom",	    (1 << 3)},
    { "topleft",    (1 << 4)},
    { "topright",   (1 << 5)},
    { "bottomleft", (1 << 6)},
    { "bottomright",(1 << 7)},
};

#define N_EDGE (sizeof (bcopEdge) / sizeof (struct _BcopEdge))

static char*
getMods(unsigned int mods)
{
    char mod1[1024];
    char mod2[1024];
    char *rv = NULL;
    int i;
    Bool first = TRUE;

    for (i = 0; i < N_MOD; i++)
        if (mods & bcopMod[i].modifier)
        {
	    if (first)
	    {
		sprintf(mod1,"%s",bcopMod[i].name);
	    }
	    else
	    {
		sprintf(mod2,"%s+%s",mod1,bcopMod[i].name);
		sprintf(mod1,"%s",mod2);
	    }
	    first = FALSE;
	}
    asprintf(&rv,"%s",mod1);
    return rv;
}

static char*
getEdges(unsigned int eMask)
{
    char mod1[1024];
    char mod2[1024];
    char *rv = NULL;
    int i;
    Bool first = TRUE;

    for (i = 0; i < N_EDGE; i++)
        if (eMask & bcopEdge[i].mask)
        {
	    if (first)
	    {
		sprintf(mod1,"%s",bcopEdge[i].name);
	    }
	    else
	    {
		sprintf(mod2,"%s,%s",mod1,bcopEdge[i].name);
		sprintf(mod1,"%s",mod2);
	    }
	    first = FALSE;
	}
    asprintf(&rv,"%s",mod1);
    return rv;
}

static char*
getEdgeState(unsigned int state)
{
    char mod1[1024];
    char mod2[1024];
    char *rv = NULL;
    Bool first = TRUE;

    if (state & CompActionStateInitEdge)
    {
        sprintf(mod1,"%s","init");
	first = FALSE;
    }
    if (state & CompActionStateTermEdge)
    {
	if (first)
	{
	    sprintf(mod1,"%s","term");
	}
	else
	{
	    sprintf(mod2,"%s,term",mod1);
	    sprintf(mod1,"%s",mod2);
	}
	first = FALSE;
    }
    if (state & CompActionStateInitEdgeDnd)
    {
	if (first)
	{
	    sprintf(mod1,"%s","initdnd");
	}
	else
	{
	    sprintf(mod2,"%s,initdnd",mod1);
	    sprintf(mod1,"%s",mod2);
	}
	first = FALSE;
    }
    if (state & CompActionStateTermEdgeDnd)
    {
	if (first)
	{
	    sprintf(mod1,"%s","termdnd");
	}
	else
	{
	    sprintf(mod2,"%s,termdnd",mod1);
	    sprintf(mod1,"%s",mod2);
	}
	first = FALSE;
    }
    asprintf(&rv,"%s",mod1);
    return rv;
}


static char*
convertToKeyString(CompDisplay *d, CompKeyBinding *k)
{
    char *mods;
    char *kstring;
    char *rv;
    if (k->modifiers)
    {
	int keysym = XKeycodeToKeysym(d->display, k->keycode, 0);
	mods = getMods(k->modifiers);
        if (keysym == NoSymbol)
	    return mods;
	kstring = XKeysymToString(keysym);
	asprintf(&rv,"%s+%s",mods,kstring);
	return rv;
    }
    else
    {
	int keysym = XKeycodeToKeysym(d->display, k->keycode, 0);
        if (keysym == NoSymbol)
	    return strdup("");
	kstring = XKeysymToString(keysym);
	asprintf(&rv,"%s",kstring);
	return rv;
    }
}

static char*
convertToButtonString(CompDisplay *d, CompButtonBinding *b)
{
    char *mods;
    char *rv;
    if (b->modifiers)
    {
	mods = getMods(b->modifiers);
        if (!b->button)
	    return mods;
	asprintf(&rv,"%s+Button%d",mods,b->button);
	return rv;
    }
    else
    {
        if (!b->button)
	    return strdup("");
	asprintf(&rv,"Button%d",b->button);
	return rv;
    }
}


static void
dumpActionValue (CompDisplay *d, CompOption *o)
{
    char *state;

    if (o->value.action.state & CompActionStateInitKey ||
	o->value.action.state & CompActionStateTermKey)
    {
	if (o->value.action.state & CompActionStateTermKey)
	{
	    if (o->value.action.state & CompActionStateInitKey)
	    {
 	        state = " state=\"init,term\"";
	    }
	    else
		state = " state=\"term\"";
	}
	else
	    state = "";
	bcopPrintf(3,"<key%s>%s</key>\n",state,
		   (o->value.action.type & CompBindingTypeKey)?
		   convertToKeyString(d, &o->value.action.key) : "");
    }
    if (o->value.action.state & CompActionStateInitButton ||
	o->value.action.state & CompActionStateTermButton)
    {
	if (o->value.action.state & CompActionStateTermButton)
	{
	    if (o->value.action.state & CompActionStateInitButton)
	    {
 	        state = " state=\"init,term\"";
	    }
	    else
		state = " state=\"term\"";
	}
	else
	    state = "";
	bcopPrintf(3,"<mouse%s>%s</mouse>\n",state,
		   (o->value.action.type & CompBindingTypeButton)?
		   convertToButtonString(d, &o->value.action.button) : "");
    }

    if (o->value.action.state & CompActionStateInitEdge ||
	o->value.action.state & CompActionStateTermEdge ||
	o->value.action.state & CompActionStateInitEdgeDnd ||
	o->value.action.state & CompActionStateTermEdgeDnd)
    {
	if (o->value.action.state & CompActionStateTermEdge ||
	    o->value.action.state & CompActionStateInitEdgeDnd ||
	    o->value.action.state & CompActionStateTermEdgeDnd)
	{
            asprintf(&state," state=\"%s\"",getEdgeState(o->value.action.state));
	}
	else
	    state = "";
	bcopPrintf(3,"<edge%s>%s</edge>\n",state,
		   (o->value.action.edgeMask) ?
		   getEdges(o->value.action.edgeMask) : "");
    }

    if (o->value.action.state & CompActionStateInitBell)
    {
	bcopPrintf(3,"<bell>%s</bell>\n",
		   (o->value.action.bell)? "true" : "false");
    }
}


static void
bcopDumpToSchema (CompDisplay *d, CompOption  *o)
{
    int type = bcopGetType(o);
    if (type == TYPE_ERROR)
    {
	printf("[BCOP]: unsuported option type for option \"%s\"\n",o->name);
	return;
    }
    bcopPrintf(2,"<option name=\"%s\" type=\"%s\">\n",
	       o->name, bcopTypes[type]);
    bcopPrintf(3,"<short>%s</short>\n",o->shortDesc);
    bcopPrintf(3,"<long>%s</long>\n",o->longDesc);
    switch (type)
    {
	case TYPE_INT:
	    dumpIntValue (d, o);
	    break;
	case TYPE_FLOAT:
	    dumpFloatValue (d, o);
	    break;
	case TYPE_BOOL:
	    dumpBoolValue (d, o);
	    break;
	case TYPE_STRING:
	    dumpStringValue (d, o);
	    break;
	case TYPE_STRINGLIST:
	    dumpStringListValue (d, o);
	    break;
	case TYPE_ENUM:
	    dumpEnumValue (d, o);
	    break;
	case TYPE_SELECTION:
	    dumpSelectionValue (d, o);
	    break;
	case TYPE_COLOR:
	    dumpColorValue (d, o);
	    break;
	case TYPE_MATCH:
	    dumpMatchValue (d, o);
	    break;
	case TYPE_ACTION:
	    dumpActionValue (d, o);
	    break;
	default:
	    break;
    }
    bcopPrintf(2,"</option>\n");
}

static void
dumpGeneralOptions (CompDisplay *d)
{
    CompOption   *option;
    int	         nOption;

    bcopPrintf (0, "<!-- general compiz options -->\n\n");
    bcopPrintf (0, "<plugin name=\"general\">\n");
    bcopPrintf (1, "<short>General Options</short>\n");
    bcopPrintf (1, "<long>General compiz options</long>\n");

    option = compGetDisplayOptions (d, &nOption);
    bcopPrintf (1, "<display>\n");
    while (nOption--)
    {
	if (!strcmp (option->name, "active_plugins"))
	{
            continue;
	}

	bcopDumpToSchema (d, option);

	option++;
    }
    bcopPrintf (1, "</display>\n");
    option = compGetScreenOptions (&d->screens[0], &nOption);
    bcopPrintf (1, "<screen>\n");
    while (nOption--)
    {

	bcopDumpToSchema (d, option);

	option++;
    }
    bcopPrintf (1, "</screen>\n");
}

static void
dumpPluginOptions (CompDisplay *d, CompPlugin *p)
{
    CompOption   *option;
    int	         nOption;

    if (!strcmp (p->vTable->name, "bcop-dump"))
	return;

    bcopPrintf (0, "<!-- %s options -->\n", p->vTable->name);
    bcopPrintf (0, "<plugin name=\"%s\">\n", p->vTable->name);
    bcopPrintf (1, "<short>%s</short>\n", p->vTable->shortDesc);
    bcopPrintf (1, "<long>%s</long>\n", p->vTable->longDesc);

    if (p->vTable->getDisplayOptions)
    {
	option = p->vTable->getDisplayOptions (p, d, &nOption);
        bcopPrintf (1, "<display>\n");
	while (nOption--)
	{

	    bcopDumpToSchema (d, option);
	    option++;
	}
	bcopPrintf (1, "</display>\n");
    }

    if (p->vTable->getScreenOptions)
    {
	option = p->vTable->getScreenOptions (p, &d->screens[0], &nOption);
	bcopPrintf (1, "<screen>\n");
	while (nOption--)
	{

	    bcopDumpToSchema (d, option);
	    option++;
	}
	bcopPrintf (1, "</screen>\n");
    }
    bcopPrintf (0, "</plugin>\n");
}

static int
dumpSchema (CompDisplay *d)
{
    char *schemaFilename, *pluginList;
    char *plugins;

    if (findActivePlugin ("bset"))
    {
	fprintf (stderr, "Can't use bcop-dump plugin with bset plugin\n");
	return 1;
    }

    if (getenv ("COMPIZ_BCOP_GENERAL"))
    {
	schemaFilename = "general.options";

	schemaFile = fopen (schemaFilename, "w+");
	if (!schemaFile)
	{
	    fprintf (stderr, "Could not open %s: %s\n", schemaFilename,
		    strerror (errno));
	    return 1;
	}
	dumpGeneralOptions (d);
	fclose (schemaFile);
    }

    pluginList = getenv ("COMPIZ_BCOP_PLUGINS");
    if (pluginList)
    {
	plugins = strtok(pluginList," ");

	while (plugins)
	{
            CompPlugin *plugin;
	    char name[1024];

	    plugin = findActivePlugin (plugins);
	    if (!plugin)
	    {
		fprintf (stderr, "No such plugin %s\n", plugins);
		return 1;
	    }

	    sprintf (name, "%s.options", plugins);

	    schemaFile = fopen (name, "w+");
	    if (!schemaFile)
	    {
		fprintf (stderr, "Could not open %s: %s\n", schemaFilename,
			strerror (errno));
		return 1;
	    }
	    dumpPluginOptions (d, plugin);
	    fclose (schemaFile);
	    plugins = strtok(NULL," ");
	}
    }
    return 0;
}

static char *restartArgv[] = { "--replace", "bset", NULL };

static void
bcopDumpHandleEvent (CompDisplay *d,
		      XEvent	  *event)
{
    int status;

    BCOP_DUMP_DISPLAY (d);

    UNWRAP (bd, d, handleEvent);

    status = dumpSchema (d);

    if (fork () != 0)
	exit (status);

    programArgv = restartArgv;
    kill (getpid (), SIGHUP);
}

static Bool
bcopDumpInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    BcopDumpDisplay *bd;

    bd = malloc (sizeof (BcopDumpDisplay));
    if (!bd)
	return FALSE;

    WRAP (bd, d, handleEvent, bcopDumpHandleEvent);

    d->privates[displayPrivateIndex].ptr = bd;

    return TRUE;
}

static void
bcopDumpFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    BCOP_DUMP_DISPLAY (d);

    UNWRAP (bd, d, handleEvent);
    free (bd);
}

static Bool
bcopDumpInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
bcopDumpFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static int
bcopDumpGetVersion (CompPlugin *plugin,
		     int	version)
{
    return ABIVERSION;
}

CompPluginVTable bcopDumpVTable = {
    "bcop-dump",
    "Bcop dump",
    "Bcop dump - dumps bcop files",
    bcopDumpGetVersion,
    bcopDumpInit,
    bcopDumpFini,
    bcopDumpInitDisplay,
    bcopDumpFiniDisplay,
    0, /* InitScreen */
    0, /* FiniScreen */
    0, /* InitWindow */
    0, /* FiniWindow */
    0, /* GetDisplayOptions */
    0, /* SetDisplayOption */
    0, /* GetScreenOptions */
    0, /* SetScreenOption */
    0, /* Deps */
    0, /* nDeps */
    0, /* Features */
    0  /* nFeatures */
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &bcopDumpVTable;
}
