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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <libgen.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "bcop.h"

Context data;
char *programName = NULL;
char *fileName = NULL;
int quiet = 0;

int error;

static void parseElements(xmlDoc *doc, ParserState* pState, xmlNode *node);

static int nameCheck(char * str)
{
	if (!str)
		return 0;
	if (!strlen(str))
		return 0;
	int i = 0;
	for (i = 0; i < strlen(str);i++)
		if (isspace(str[i]) || !isprint(str[i]))
			return 0;
	return 1;
}

static void
initData()
{
	data.name = NULL;
	data.options = NULL;
	data.mode = CodeNone;
}

static OptionType get_Option_type(xmlChar * type)
{
	if (!xmlStrcmp(type, (const xmlChar *) "int"))
		return OptionTypeInt;
	if (!xmlStrcmp(type, (const xmlChar *) "bool"))
		return OptionTypeBool;
	if (!xmlStrcmp(type, (const xmlChar *) "float"))
		return OptionTypeFloat;
	if (!xmlStrcmp(type, (const xmlChar *) "string"))
		return OptionTypeString;
	if (!xmlStrcmp(type, (const xmlChar *) "stringlist"))
		return OptionTypeStringList;
	if (!xmlStrcmp(type, (const xmlChar *) "enum"))
		return OptionTypeEnum;
	if (!xmlStrcmp(type, (const xmlChar *) "selection"))
		return OptionTypeSelection;
	if (!xmlStrcmp(type, (const xmlChar *) "color"))
		return OptionTypeColor;
	if (!xmlStrcmp(type, (const xmlChar *) "action"))
		return OptionTypeAction;
	if (!xmlStrcmp(type, (const xmlChar *) "match"))
		return OptionTypeMatch;
	return OptionTypeError;
}

static void
parseIntOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "min")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asInt.min = strtol((char *)key,NULL,0);
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "max")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asInt.max = strtol((char *)key,NULL,0);
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asInt.def = strtol((char *)key,NULL,0);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseBoolOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asBool.def = (strcasecmp((char *)key,"true") == 0);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseFloatOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "min")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asFloat.min = strtod((char *)key,NULL);
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "max")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asFloat.max = strtod((char *)key,NULL);
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "precision")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asFloat.precision = strtod((char *)key,NULL);
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asFloat.def = strtod((char *)key,NULL);
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseStringOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asString.def = strdup((key)?(char *)key:"");
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseMatchOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asMatch.def = strdup((key)?(char *)key:"");
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseColorOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "red")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asColor.red =
					MAX(0,MIN(0xffff,strtol((char *)key,NULL,0)));
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "green")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asColor.green =
					MAX(0,MIN(0xffff,strtol((char *)key,NULL,0)));
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "blue")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asColor.blue =
					MAX(0,MIN(0xffff,strtol((char *)key,NULL,0)));
			xmlFree(key);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "alpha")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			set->data.asColor.alpha =
					MAX(0,MIN(0xffff,strtol((char *)key,NULL,0)));
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseStringListOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "default")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (key && strlen((char *)key))
			{
				OptionValuesList *lv = calloc(1,sizeof(OptionValuesList));
				lv->value = strdup((char *)key);
				xmlChar *raw = xmlGetProp(cur, (xmlChar *)"raw");
				if (raw && strcasecmp((char *)raw,"true") == 0)
					lv->raw = 1;
				xmlFree(raw);
				if (!set->data.asSList.begin && !set->data.asSList.end)
				{
					set->data.asSList.begin = set->data.asSList.end = lv;
				}
				else
				{
					set->data.asSList.end->next = lv;
					set->data.asSList.end = lv;
				}
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
}

static void
parseEnumOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "value")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			xmlChar *name;
			name = xmlGetProp(cur, (xmlChar *)"name");
			if (!nameCheck((char *)name))
			{
				if (!error) printf("error\n");
				error = 1;
				fprintf(stderr,"[ERROR]: wrong value name \"%s\"\n",name);
				xmlFree(name);
				xmlFree(key);
				continue;
			}
			OptionValuesList *l = set->data.asSList.begin;
			while (l && name)
			{
				if (!strcasecmp(l->name,(char *)name))
				{
					if (!error) printf("error\n");
					error = 1;
					fprintf(stderr,"[ERROR]: value with same name already defined \"%s\"\n",name);
					xmlFree(name);
					xmlFree(key);
					name = NULL;
					key = NULL;
					continue;
				}
				l = l->next;
			}
			if (name && strlen((char *)name))
			{
				OptionValuesList *lv = calloc(1,sizeof(OptionValuesList));
				lv->value = (key && strlen((char *)key)) ?
							strdup((char *)key) : strdup((char *)name);
				lv->name = strToLower((char *)name);
				lv->uName = strToUpper((char *)name);
				lv->fUName = strToFirstUp((char *)name);
				xmlChar *def;
				def = xmlGetProp(cur, (xmlChar *)"default");
				if (def && strcasecmp((char *)def,"true") == 0)
					lv->def = 1;
				xmlFree(def);
				if (!set->data.asSList.begin && !set->data.asSList.end)
				{
					set->data.asSList.begin = set->data.asSList.end = lv;
				}
				else
				{
					set->data.asSList.end->next = lv;
					set->data.asSList.end = lv;
				}
			}
			xmlFree(key);
			xmlFree(name);
		}
		cur = cur->next;
	}
}

static void
parseSelectionOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "value")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			xmlChar *name;
			name = xmlGetProp(cur, (xmlChar *)"name");
			if (!nameCheck((char *)name))
			{
				if (!error) printf("error\n");
				error = 1;
				fprintf(stderr,"[ERROR]: wrong value name \"%s\"\n",name);
				xmlFree(name);
				xmlFree(key);
				continue;
			}
			OptionValuesList *l = set->data.asSList.begin;
			while (l && name)
			{
				if (!strcasecmp(l->name,(char *)name))
				{
					if (!error) printf("error\n");
					error = 1;
					fprintf(stderr,"[ERROR]: value with same name already defined \"%s\"\n",name);
					xmlFree(name);
					xmlFree(key);
					name = NULL;
					key = NULL;
					continue;
				}
				l = l->next;
			}
			if (key && strlen((char *)key) && name && strlen((char *)name))
			{
				OptionValuesList *lv = calloc(1,sizeof(OptionValuesList));
				lv->value = (key && strlen((char *)key)) ?
							strdup((char *)key) : strdup((char *)name);
				lv->name = strToLower((char *)name);
				lv->uName = strToUpper((char *)name);
				lv->fUName = strToFirstUp((char *)name);
				xmlChar *def;
				def = xmlGetProp(cur, (xmlChar *)"default");
				if (def && strcasecmp((char *)def,"true") == 0)
					lv->def = 1;
				xmlFree(def);
				if (!set->data.asSList.begin && !set->data.asSList.end)
				{
					set->data.asSList.begin = set->data.asSList.end = lv;
				}
				else
				{
					set->data.asSList.end->next = lv;
					set->data.asSList.end = lv;
				}
			}
			xmlFree(key);
			xmlFree(name);
		}
		cur = cur->next;
	}
}

static void
parseActionOption(xmlDoc *doc, xmlNode *node, Option *set)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "key")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (key && strlen((char *)key))
			{
				set->data.asBind.type |= CompBindingTypeKey;
				set->data.asBind.skey = 1;
				set->data.asBind.key = strdup((char *)key);
			}
			xmlFree(key);
			xmlChar *state;
			state = xmlGetProp(cur, (xmlChar *)"state");

			if (!state)
			{
				set->data.asBind.state |= CompActionStateInitKey;
			}
			else
			{
				char *tok = strtok((char *)state,",");
				while (tok)
				{
					if (!strcasecmp(tok,"init"))
						set->data.asBind.state |= CompActionStateInitKey;
					else if (!strcasecmp(tok,"term"))
						set->data.asBind.state |= CompActionStateTermKey;
					tok = strtok(NULL,",");
				}
			}
			xmlFree(state);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "mouse")) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (key && strlen((char *)key))
			{
				set->data.asBind.type |= CompBindingTypeButton;
				set->data.asBind.smouse = 1;
				set->data.asBind.mouse = strdup((char *)key);
			}
			xmlFree(key);
			xmlChar *state;
			state = xmlGetProp(cur, (xmlChar *)"state");
			if (!state)
			{
				set->data.asBind.state |= CompActionStateInitButton;
			}
			else
			{
				char *tok = strtok((char *)state,",");
				while (tok)
				{
					if (!strcasecmp(tok,"init"))
						set->data.asBind.state |= CompActionStateInitButton;
					else if (!strcasecmp(tok,"term"))
						set->data.asBind.state |= CompActionStateTermButton;
					tok = strtok(NULL,",");
				}
			}
			xmlFree(state);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "edge")) {
			set->data.asBind.sedge = 1;
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			char *tok = strtok((char *)key,",");
			while (tok && key)
			{
				if (!strcasecmp(tok,"left"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_LEFT);
				else if (!strcasecmp(tok,"right"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_RIGHT);
				else if (!strcasecmp(tok,"top"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_TOP);
				else if (!strcasecmp(tok,"bottom"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_BOTTOM);
				else if (!strcasecmp(tok,"topleft"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_TOPLEFT);
				else if (!strcasecmp(tok,"topright"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_TOPRIGHT);
				else if (!strcasecmp(tok,"bottomleft"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_BOTTOMLEFT);
				else if (!strcasecmp(tok,"bottomright"))
					set->data.asBind.edge |= (1 << SCREEN_EDGE_BOTTOMRIGHT);
				tok = strtok(NULL,",");
			}
			xmlFree(key);
			xmlChar *state;
			state = xmlGetProp(cur, (xmlChar *)"state");
			if (!state)
			{
				set->data.asBind.state |= CompActionStateInitEdge;
			}
			else
			{
				char *tok = strtok((char *)state,",");
				while (tok)
				{
					if (!strcasecmp(tok,"init"))
						set->data.asBind.state |= CompActionStateInitEdge;
					else if (!strcasecmp(tok,"term"))
						set->data.asBind.state |= CompActionStateTermEdge;
					else if (!strcasecmp(tok,"initdnd"))
						set->data.asBind.state |= CompActionStateInitEdgeDnd;
					else if (!strcasecmp(tok,"termdnd"))
						set->data.asBind.state |= CompActionStateTermEdgeDnd;
					tok = strtok(NULL,",");
				}
			}
			xmlFree(state);
		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "bell")) {
			set->data.asBind.sbell = 1;
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (key)
				set->data.asBind.bell = (strcasecmp((char *)key,"true") == 0);
			xmlFree(key);
			set->data.asBind.state |= CompActionStateInitBell;
		}
		cur = cur->next;
	}
}



static void
parseOption(xmlDoc *doc, ParserState* pState, xmlNode *node)
{
	xmlNode *cur = node;
	xmlChar *name;
	name = xmlGetProp(cur, (xmlChar *)"name");
	if (!name)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no Option name defined\n");
		xmlFree(name);
		return;
	}
	if (!nameCheck((char *)name))
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: wrong Option name \"%s\"\n",name);
		xmlFree(name);
		return;
	}
	xmlChar *type;
	type = xmlGetProp(cur, (xmlChar *)"type");
	if (!type)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no Option type defined\n");
		xmlFree(name);
		xmlFree(type);
		return;
	}

	OptionType sType = get_Option_type(type);

	if (sType == OptionTypeError)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: wrong Option type \"%s\"\n",(char *)type);
		xmlFree(name);
		xmlFree(type);
		return;
	}
	if (sType == OptionTypeAction && pState->screen)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: action defination in screen section\n");
		xmlFree(name);
		xmlFree(type);
		return;
	}
	xmlFree(type);

	Option *set = data.options;
	while (set != NULL)
	{
		if (!strcmp(set->name,(char *)name))
		{
			if (!error) printf("error\n");
			error = 1;
			fprintf(stderr,"[ERROR]: Option \"%s\" already defined\n",name);
			xmlFree(name);
			return;
		}
		set = set->next;
	}

	Option *ns = calloc(1, sizeof(Option));
	ns->name = strToLower((char *)name);
	ns->uName = strToUpper((char *)name);
	ns->fUName = strToCamel((char *)name);
	ns->type = sType;
	ns->group = (pState->group)?strdup(pState->group):NULL;
	ns->subGroup = (pState->subGroup)?strdup(pState->subGroup):NULL;
	ns->screen = pState->screen;

	xmlChar *adv;
	adv = xmlGetProp(cur, (xmlChar *)"name");
	if (adv && strcasecmp((char *)adv,"true") == 0)
		ns->advanced = 1;
	else
		ns->advanced = 0;
	xmlFree(adv);

	if (!data.options)
	{
		data.options = ns;
	}
	else
	{
		Option *s = data.options;
		while (s->next != NULL) s = s->next;
		s->next = ns;
	}
	cur = node->xmlChildrenNode;

	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "short") && !ns->shortDesc) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			ns->shortDesc = strdup((char *)key);
			xmlFree(key);
		} else
		if (!xmlStrcmp(cur->name, (const xmlChar *) "long") && !ns->longDesc) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			ns->longDesc = strdup((char *)key);
			xmlFree(key);
		}
		if (!xmlStrcmp(cur->name, (const xmlChar *) "hints") && !ns->hints) {
			xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			ns->hints = strdup((char *)key);
			xmlFree(key);
		}
		cur = cur->next;
	}
	xmlFree(name);

	switch (ns->type)
	{
		case OptionTypeInt:
			parseIntOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeBool:
			parseBoolOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeFloat:
			parseFloatOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeString:
			parseStringOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeStringList:
			parseStringListOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeEnum:
			parseEnumOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeSelection:
			parseSelectionOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeColor:
			parseColorOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeAction:
			parseActionOption(doc, node->xmlChildrenNode, ns);
			break;
		case OptionTypeMatch:
			parseMatchOption(doc, node->xmlChildrenNode, ns);
			break;
		default:
			break;
	}
}

static void
parseSubGroup(xmlDoc *doc, ParserState* pState, xmlNode *node)
{
	xmlNode *cur = node;
	xmlChar *name;
	name = xmlGetProp(cur, (xmlChar *)"name");
	if (!name)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no subgroup name defined\n");
		xmlFree(name);
		return;
	}
	if (pState->subGroup)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: recursive subgroup defined\n");
		xmlFree(name);
		return;
	}


	pState->subGroup = (char *)name;
	parseElements(doc, pState, node->xmlChildrenNode);
	pState->subGroup = NULL;
	xmlFree(name);
}

static void
parseGroup(xmlDoc *doc, ParserState* pState, xmlNode *node)
{
	xmlNode *cur = node;
	xmlChar *name;
	name = xmlGetProp(cur, (xmlChar *)"name");
	if (!name)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no group name defined\n");
		xmlFree(name);
		return;
	}
	if (pState->group)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: recursive group defined\n");
		xmlFree(name);
		return;
	}
	if (pState->subGroup)
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no group defination inside of subgroup definition allowed\n");
		xmlFree(name);
		return;
	}

	pState->group = (char *)name;
	parseElements(doc, pState, node->xmlChildrenNode);
	pState->group = NULL;
	xmlFree(name);
}

static void
parseElements(xmlDoc *doc, ParserState* pState, xmlNode *node)
{
	xmlNode *cur = node;
	while (cur != NULL)
	{
		if (!xmlStrcmp(cur->name, (const xmlChar *) "group")) {
			parseGroup(doc, pState, cur);
		} else
		if (!xmlStrcmp(cur->name, (const xmlChar *) "subgroup")) {
			parseSubGroup(doc, pState, cur);
		} else
		if (!xmlStrcmp(cur->name, (const xmlChar *) "option")) {
			parseOption(doc, pState, cur);
		}
		cur = cur->next;
	}

}

static void
parseOptionFile(xmlDoc *doc)
{
	xmlNode *root_element = NULL;
	ParserState pState;
	pState.screen = 0;
	pState.group = NULL;
	pState.subGroup = NULL;

	root_element = xmlDocGetRootElement(doc);

	if (root_element == NULL) {
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: empty document\n");
		return;
	}
	if (xmlStrcmp(root_element->name, (const xmlChar *) "plugin")) {
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: document of the wrong type, root node != plugin");
		return;
	}

	xmlChar *name;
	name = xmlGetProp(root_element, (xmlChar *)"name");
	if (!name || !strlen((char *)name))
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: no plugin name defined\n");
		xmlFree(name);
		return;
	}
	if (!nameCheck((char *)name))
	{
		if (!error) printf("error\n");
		error = 1;
		fprintf(stderr,"[ERROR]: plugin name has to be a single word\n");
		xmlFree(name);
		return;
	}
	data.name = strToLower((char *)name);
	data.uName = strToUpper((char *)name);
	data.fUName = strToFirstUp((char *)name);

	xmlNode *pc = root_element->xmlChildrenNode;

	while (pc != NULL)
	{
		if (!xmlStrcmp(pc->name, (const xmlChar *)"screen"))
		{
			pState.screen = 1;
			parseElements(doc, &pState, pc->xmlChildrenNode);
		} else if (!xmlStrcmp(pc->name, (const xmlChar *)"display"))
		{
			pState.screen = 0;
			parseElements(doc, &pState, pc->xmlChildrenNode);
		}
		pc = pc->next;
	}
	xmlFree(name);
}

char * strToLower(char *str)
{
	char *ret = strdup(str);
	int i;
	for (i = 0; i < strlen(str);i++)
		ret[i] = tolower(ret[i]);
	return ret;
}

char * strToUpper(char *str)
{
	char *ret = strdup(str);
	int i;
	for (i = 0; i < strlen(str);i++)
		ret[i] = toupper(ret[i]);
	return ret;
}

char * strToFirstUp(char *str)
{
	char *ret = strToLower(str);
	ret[0] = toupper(ret[0]);
	return ret;
}

char * strToCamel(char *str)
{
	char *lowString = strToLower(str);
	char *token;
	char *buffer;

	buffer = malloc (sizeof(char) * (strlen(lowString) + 1));
	strcpy (buffer, "");

	token = strtok (lowString, "_");
	while (token)
	{
		token[0] = toupper(token[0]);
		strcat (buffer, token);
		token = strtok (NULL, "_");
	}

	free (lowString);
	
	return buffer;
}

static void usage(void)
{
	printf("Usage: %9s [options] <options file>\n",programName);
	printf("Options:\n");
	printf("  -h, --help           display this help message\n");
	printf("  -v, --version        print version information\n");
	printf("  -b, --beryl          generate beryl compatible code (default)\n");
	printf("  -c, --compiz         generate compiz compatible code\n");
	printf("  -q, --quiet          don't print informational messages\n");
	printf("      --source=<file>  source file name\n");
	printf("      --header=<file>  header file name\n");
}

#define OPT_HELP 'h'
#define OPT_VERSION 'v'
#define OPT_BERYL 'b'
#define OPT_COMPIZ 'c'
#define OPT_QUIET 'q'
#define OPT_SOURCE 1
#define OPT_HEADER 2
	
int main(int argc, char **argv)
{
	xmlDoc *doc = NULL;
	int optch;
	programName = argv[0];
	char *src = NULL;
	char *hdr = NULL;
	
	char sopts[] = "hvbcq";
	struct option lopts[] = {
		{"help", 0, 0, OPT_HELP},
		{"version", 0, 0, OPT_VERSION},
		{"beryl", 0, 0, OPT_BERYL},
		{"compiz", 0, 0, OPT_COMPIZ},
		{"quiet", 0, &quiet, OPT_QUIET},
		{"source", 1, 0, OPT_SOURCE},
		{"header", 1, 0, OPT_HEADER},
		{0, 0, 0, 0}
	};
	
	if (argc < 2)
	{
		usage();
        return 1;
	}

	initData();

	/* Process arguments */
	while ((optch = getopt_long(argc, argv, sopts, lopts, NULL)) != EOF)
	{
		switch (optch)
		{
		case OPT_HELP:
			usage();
			return 0;
		case OPT_VERSION:
			printf(PACKAGE_STRING "\n");
			return 0;
		case OPT_BERYL:
			if (data.mode != CodeNone)
			{
				fprintf(stderr,"%s: can only generate output for one composite manager\n",programName);
				return 1;
			}
			data.mode = CodeBeryl;
			break;
		case OPT_COMPIZ:
			if (data.mode != CodeNone)
			{
				fprintf(stderr,"%s: can only generate output for one composite manager\n",programName);
				return 1;
			}
			data.mode = CodeCompiz;
			break;
		case OPT_SOURCE:
			if (optarg)
				src = optarg;
			break;
		case OPT_HEADER:
			if (optarg)
				hdr = optarg;
			break;
		case 0:				/* Returned when auto-set stuff is in effect */
			break;
		default:
			/* Not recognised option or with missing argument.
			 * getopt_long() prints an error message for us.
			 */
			return 1;

		}
	}

	if (data.mode == CodeNone)
		data.mode = CodeBeryl;

	fileName = argv[argc-1];

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
	if (!quiet) printf("Parsing XML Option file \"%s\"...",fileName);
    doc = xmlReadFile(fileName, NULL, 0);

    if (doc == NULL) {
		if (!quiet) printf("error\n");
		exit(1);
    }

	if (!quiet) printf("done\n");

	error = 0;
	if (!quiet) printf("Parsing XML tree...");
	parseOptionFile(doc);
	if (!quiet) printf("done\n");

	if (!src && !hdr)
	{
		char str[1024];
		char *prefix = strtok(basename(strdup(fileName)),".");
		if (!prefix || !strlen(prefix))
			prefix = data.name;
		sprintf(str,"%s_options.c",prefix);
		src = strdup(str);
		sprintf(str,"%s_options.h",prefix);
		hdr = strdup(str);
	}
	
	int rv = error;
	if (!error)
		rv = genCode(src,hdr);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    return rv;
}
