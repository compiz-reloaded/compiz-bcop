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

static char *edgeName[] = {
	"Left",
	"Right",
	"Top",
	"Bottom",
	"TopLeft",
	"TopRight",
	"BottomLeft",
	"BottomRight"
};

int nEdges=sizeof(edgeName)/sizeof(edgeName[0]);

enum {
	actionPartKey,
	actionPartButton,
	actionPartBell,
	actionPartEdge,
};

char * actPartInfo[][3]={
	{"key","Key","Key Binding"},
	{"button","Button","Mouse Button Binding"},
	{"bell","Bell","System Bell Binding"},
	{"edge","Edge","Screen Edge Binding"},
};
int nActParts=sizeof(actPartInfo)/sizeof(actPartInfo[0]);

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

void gconfEscapeXML(char * value)
{
	char srcBuffer[8192];
	char destBuffer[8192];
	char tempBuffer[8192];
	char * indx;
	strcpy(srcBuffer,value);
	destBuffer[0]='\0';
	for (indx=srcBuffer;*indx;indx++)
	{
		strcpy(tempBuffer,destBuffer);
		char * rep=NULL;
		switch(*indx)
		{
			case '&':
				rep="&amp;";
				break;
			case '<':
				rep="&lt;";
				break;
			case '>':
				rep="&gt;";
				break;
			default:
				break;
		}
		if (rep)
			sprintf(destBuffer,"%s%s",tempBuffer,rep);
		else
			sprintf(destBuffer,"%s%c",tempBuffer,*indx);
	}
	strcpy(value,destBuffer);
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
gconfTypeToString (OptionType type, int actionPart)
{
    switch (type) {
		case OptionTypeAction:
			switch(actionPart)
			{
				case actionPartBell:
					return "bool";
				case actionPartKey:
				case actionPartButton:
					return "string";
				case actionPartEdge:
					return "list";
				default:
					break;
			}
			break;
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

static char * modifiers[] = {
	"Shift",
	"Control",
	"Mod1",
	"Mod2",
	"Mod3",
	"Mod4",
	"Mod5",
	"Alt",
	"Meta",
	"Super",
	"Hyper",
	"ModeSwitch",
};

int nModifiers=sizeof(modifiers)/sizeof(modifiers[0]);

char * gconfConvertModifiers(char * value, char * ret)
{
	char tempBuffer[8192];
	char tokBuffer[8192];
	char valBuffer[8192];
	strcpy(tokBuffer,value);
	char * tok;
	ret[0]='\0';
	for (tok=strtok(tokBuffer,"+");tok;tok=strtok(NULL,"+"))
	{
		strcpy(tempBuffer,ret);
		int i;
		for (i=0;i<nModifiers;i++)
		{
			if (!strcmp(tok,modifiers[i]))
			{
				sprintf(ret,"%s<%s>",tempBuffer,tok);
				break;
			}
		}
		if (i==nModifiers)
			strcpy(valBuffer,tok);
	}
	strcpy(tempBuffer,ret);
	sprintf(ret,"%s%s",tempBuffer,valBuffer);
	return ret;	
}

int gconfGetDefaultValue (Option * o, char * value, int actionPart)
{
	OptionValuesList * l;
	int i;
	char tempBuffer[8192];
	switch (o->type)
	{
		case OptionTypeAction:
			switch(actionPart)
			{
				case actionPartKey:
					if (o->data.asBind.skey)
						sprintf(value,"%s",
								gconfConvertModifiers(
									o->data.asBind.key,
									tempBuffer));
					else
						sprintf(value,"None");
					return 1;
				case actionPartButton:
					if (o->data.asBind.smouse)
						sprintf(value,"%s",
								gconfConvertModifiers(
									o->data.asBind.mouse,
									tempBuffer));
					else
						sprintf(value,"None");
					return 1;
				case actionPartBell:
					sprintf(value,
							(o->data.asBind.sbell?
							 (o->data.asBind.bell?"true":"false"):"false"));
					return 1;
				case actionPartEdge:
					if (o->data.asBind.sedge)
					{
						tempBuffer[0]='\0';
						for (i=0;i<nEdges;i++)
						{
							strcpy(value,tempBuffer);
							if (o->data.asBind.edge & 1<<i)
							{
								if (tempBuffer[0])
									sprintf(tempBuffer,"%s,%s",value,
											edgeName[i]);
								else
									sprintf(tempBuffer,"%s",edgeName[i]);
							}
						}
						sprintf(value,"[%s]",tempBuffer);
					}
					else
						sprintf(value,"[]");
					return 1;
				default:
					break;
			}
			break;
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
			for(l=o->data.asSList.begin;l;l=l->next)
			{
				if (l->def)
					sprintf(value,"%s",l->value);
					return 1;
			}
			break;
		case OptionTypeStringList:
		case OptionTypeSelection:
			if (!o->data.asSList.begin)
			{
				sprintf(value,"[]");
				return 1;
			}
			tempBuffer[0]='\0';
			for (l=o->data.asSList.begin;l;l=l->next)
			{
				if (o->type==OptionTypeStringList || l->def)
				{
					strcpy(value,tempBuffer);
					if (value[0])
						sprintf(tempBuffer,"%s,%s",value,l->value);
					else
						sprintf(tempBuffer,"%s",l->value);
				}
			}
			sprintf(value,"[%s]",tempBuffer);
			return 1;
		default:
			break;
	}
	return 0;
}

int gconfGetSubInfo (Option * o, char * subInfo, int actionPart)
{
	OptionValuesList * l;
	char tempBuffer[8192];
	switch (o->type)
	{
		case OptionTypeAction:
			sprintf(subInfo,"%s",actPartInfo[actionPart][2]);
			return 1;
		case OptionTypeInt:
			sprintf(subInfo,"%d-%d",o->data.asInt.min,o->data.asInt.max);
			return 1;
		case OptionTypeFloat:
			sprintf(subInfo,"%f-%f",o->data.asFloat.min,o->data.asFloat.max);
			return 1;
		case OptionTypeEnum:
		case OptionTypeSelection:
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

void gconfDumpToSchema(Option * o, char * pathFmt, char * subkey,
		int actionPart)
{
	char path[1024];
	char subInfo[8192];
	char value[8192];
	sprintf(path,pathFmt,subkey,o->name);
	if (o->type == OptionTypeAction)
	{
		strcpy(subInfo,path);
		sprintf(path,"%s_%s",subInfo,actPartInfo[actionPart][0]);
	}
	gconfPrintf(2,"<schema>");
	gconfPrintf(3,"<key>/schemas%s</key>",path);
	gconfPrintf(3,"<applyto>%s</applyto>",path);
	gconfPrintf(3,"<owner>compiz</owner>");
	gconfPrintf(3,"<type>%s</type>",gconfTypeToString(o->type,actionPart));
	if (!strcmp(gconfTypeToString(o->type,actionPart),"list"))
		gconfPrintf(3,"<list_type>string</list_type>");
	if (gconfGetDefaultValue(o,value,actionPart))
	{
		gconfEscapeXML(value);
		gconfPrintf(3,"<default>%s</default>",value);
	}
	else
		gconfPrintf(3,"<!-- type not implemented yet -->");
	gconfPrintf(3,"<locale name=\"C\">");
	if (o->type == OptionTypeAction)
		gconfPrintf(4,"<short>%s %s</short>",o->shortDesc,
				actPartInfo[actionPart][1]);
	else
		gconfPrintf(4,"<short>%s</short>",o->shortDesc);
	if(gconfGetSubInfo(o,subInfo,actionPart))
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
		if (o->type==OptionTypeAction)
		{
			int i;
			for (i=0;i<nActParts;i++)
				gconfDumpToSchema(o, pathFmt, 
						(o->screen?"screen0":"allscreens"),i);
		}
		else
		{
			gconfDumpToSchema(o, pathFmt, 
					(o->screen?"screen0":"allscreens"),0);
		}
		o=o->next;
	}
	gconfWriteFooter();
}

int genSchema(char *sch)
{
	int rv=0;

	printf("Creating \"%s\"...", sch);
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

	printf("done.\n");
	return rv;
}
