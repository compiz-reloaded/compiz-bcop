//TODO - add license header
//some code from gconf-dump.c

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
#include <errno.h>
#include <libgen.h>
#include <X11/Xlib.h>

#include "bcop.h"

static FILE * schemaFile;

//Nice little function from gconf-dump.c to pretty-print output
//modified to append \n to each line automatically
static void
gconfPrintf (int level, char *format, ...)
{
    va_list args;

    if (level > 0) {
	int i;

	for (i = 0; i < level * 2; i++)
	    fprintf (schemaFile, "  ");
    }

    va_start (args, format);
    vfprintf (schemaFile, format, args);
    va_end (args);
	fprintf(schemaFile,"\n");
}

void gconfWriteHeader()
{
	gconfPrintf(0,"<!-- schema file written by bcop -->");
	gconfPrintf(0,"<gconfschemafile>");
	gconfPrintf(1,"<schemalist>");
	gconfPrintf(2,"<!-- options for %s -->",data.name);
}

void gconfWriteFooter()
{
	gconfPrintf(1,"</schemalist>");
	gconfPrintf(0,"</gconfschemafile>");
}

//swiped from gconf-dump.c
static char *
gconfTypeToString (OptionType type)
{
    switch (type) {
    case OptionTypeBool:
	return "bool";
    case OptionTypeString:
    case OptionTypeColor:
    case OptionTypeMatch:
	case OptionTypeEnum:
	return "string";
    case OptionTypeInt:
	return "int";
    case OptionTypeFloat:
	return "float";
	case OptionTypeSelection:
    case OptionTypeStringList:
	return "list";
    default:
	break;
    }

    return "unknown";
}

int gconfGetDefaultValue (Option * o, char * value)
{
	OptionValuesList * l;
	switch (o->type)
	{
		case OptionTypeInt:
			sprintf(value,"%d",o->data.asInt.def);
			return 1;
		case OptionTypeFloat:
			sprintf(value,"%f",o->data.asFloat.def);
			return 1;
		case OptionTypeBool:
			sprintf(value,"%s",(o->data.asBool.def?"true":"false"));
			return 1;
		case OptionTypeColor:
			sprintf(value,"#%.2x%.2x%.2x%.2x",
					o->data.asColor.red>>8,
					o->data.asColor.green>>8,
					o->data.asColor.blue>>8,
					o->data.asColor.alpha>>8);
			return 1;
		case OptionTypeString:
			sprintf(value,"%s",o->data.asString.def);
			return 1;
		case OptionTypeMatch:
			sprintf(value,"%s",o->data.asMatch.def);
			return 1;
		case OptionTypeEnum:
			//stored in a list
			for(l=o->data.asSList.begin;l;l=l->next)
			{
				if (l->def)
					sprintf(value,"%s",l->value);
					return 1;
			}
			break;
		default:
			break;
	}
	return 0;
}

int gconfGetSubInfo (Option * o, char * subInfo)
{
	OptionValuesList * l;
	char tempBuffer[8192];
	switch (o->type)
	{
		case OptionTypeInt:
			sprintf(subInfo,"%d-%d",o->data.asInt.min,o->data.asInt.max);
			return 1;
		case OptionTypeFloat:
			sprintf(subInfo,"%f-%f",o->data.asFloat.min,o->data.asFloat.max);
			return 1;
		case OptionTypeEnum:
			if (!o->data.asSList.begin)
				return 0;
			subInfo[0]='\0';
			for (l=o->data.asSList.begin;l;l=l->next)
			{
				strcpy(tempBuffer,subInfo);
				if (subInfo[0])
					sprintf(subInfo,"%s, %s",tempBuffer,l->value);
				else
					sprintf(subInfo,"%s",l->value);
			}
			return 1;
		default:
			break;
	}
	return 0;
}

void gconfDumpToSchema(Option * o, char * pathFmt, char * subkey)
{
	char path[1024];
	char subInfo[8192];
	char value[8192];
	sprintf(path,pathFmt,subkey,o->name);
	gconfPrintf(2,"<schema>");
	gconfPrintf(3,"<key>/schemas%s</key>",path);
	gconfPrintf(3,"<applyto>%s</applyto>",path);
	gconfPrintf(3,"<owner>compiz</owner>");
	gconfPrintf(3,"<type>%s</type>",gconfTypeToString(o->type));
	if (o->type == OptionTypeStringList)
		gconfPrintf(3,"<list_type>string</list_type>");
	if (gconfGetDefaultValue(o,value))
		gconfPrintf(3,"<default>%s</default>",value);
	else
		gconfPrintf(3,"<!-- type not implemented yet -->");
	gconfPrintf(3,"<locale name=\"C\">");
	gconfPrintf(4,"<short>%s</short>",o->shortDesc);
	if(gconfGetSubInfo(o,subInfo))
		gconfPrintf(4,"<long>%s (%s)</long>",o->longDesc,subInfo);
	else
		gconfPrintf(4,"<long>%s</long>",o->longDesc);
	gconfPrintf(3,"</locale>");
	gconfPrintf(2,"</schema>");
}

void gconfWriteSchema(int is_general)
{
	char * pluginPathFmtFmt = "/apps/compiz/plugins/%s/%%s/options/%%s";
	char pluginPathFmt[1024];
	char * generalPathFmt = "/apps/compiz/general/%s/options/%s";
	char * pathFmt;
	sprintf(pluginPathFmt,pluginPathFmtFmt,data.name);
	if (is_general)
		pathFmt = generalPathFmt;
	else
		pathFmt = pluginPathFmt;
	gconfWriteHeader();
	Option * o = data.options;
	while (o)
	{
		if (o->screen)
			gconfDumpToSchema(o, pathFmt, "screen0");
		else
			gconfDumpToSchema(o, pathFmt, "allscreens");
		o=o->next;
	}
	gconfWriteFooter();
}

int genSchema(char *sch)
{
	int rv=0;
	schemaFile = fopen(sch,"w");
	if (!schemaFile)
    {
		fprintf (stderr, "Could not open %s: %s\n", sch,
				strerror (errno));
		return 1;
    }
	switch (data.mode)
	{
		case SchemaGConf:
			gconfWriteSchema(0); // General not yet supported/possible
			break;
		default:
			rv=2; // unknown mode
	}
	fclose(schemaFile);
	return rv;
}
