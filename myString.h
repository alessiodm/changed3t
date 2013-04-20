#pragma once // Direttiva di ottimizzazione per #ifndef _MYSTRING_H ...
#ifndef _MYSTRING_H
#define _MYSTRING_H

#include <string.h>

// Definizione del Tipo STRING
typedef char* STRING;

// Definizione delle Operazioni possibili su STRING
STRING strCreate (void);
STRING strCreate (char* init);
void strAssign (STRING* dest, STRING newVal);
int strLength (STRING str);
void strAppend (STRING* dest, STRING toAppend);
void strDelete (STRING* str);
int strCompare (STRING str1, STRING str2);

/****************************************************************************
*					IMPLEMENTAZIONE											*
*		(da spostare in un file c o cpp separato)							*
****************************************************************************/
STRING strCreate (void){
	STRING newString;
	newString = NULL;
	return newString;
}

STRING strCreate (STRING init){
	if (init == NULL) return strCreate();
	
	int len = strLength(init);
	char* temp = (char*) malloc (sizeof(char) * (len + 1));
	strcpy(temp, init);
	
	STRING newString;
	newString = temp;
	return newString;
}

void strAssign (STRING* dest, STRING newVal){
	// Realizzazione alternativa...
	if (*dest != NULL) strDelete(dest);
	*dest = strCreate(newVal);
}

int strLength (STRING str){
	if (str == NULL) return -1;
	else return strlen(str);
}

void strAppend (STRING* dest, STRING toAppend){
	int lenD = strLength(*dest);
	int lenS = strLength(toAppend);

	if (lenS < 0) return;
	if (lenD < 0) { strAssign(dest, toAppend); return; }

	char* temp = (char*) malloc((lenD + lenS + 1) * sizeof(char));
	strcpy(temp, *dest); 
	strDelete(dest);
	strcat(temp, toAppend);

	STRING tmpStr; tmpStr = temp;
	strAssign(dest, tmpStr);
}

int strCompare (STRING str1, STRING str2){
	return strcmp(str1, str2);
}

void strDelete (STRING* str){
	if (*str!= NULL) {
		free(*str);
		*str = NULL;
	}
}
//****************************************************************************

#endif /*_MYSTRING_H*/