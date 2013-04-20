#pragma once // Direttiva di ottimizzazione per #ifndef _CHANGEDETECTIONLIB_H ...
#ifndef _CHANGEDETECTIONLIB_H
#define _CHANGEDETECTIONLIB_H

#include "cv.h"
#include "myString.h"

//----------------------------------------------------------------------------
//					Definizione Costanti
//----------------------------------------------------------------------------
#define PI										3.14159265358979323846

// Binarizzazione
#define BACKGROUND								0
#define FOREGROUND								255

// Object Recognition
#define MAX_NUM_OBJECTS_IN_SCENE				15
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//					Definizione Macro
//----------------------------------------------------------------------------
#define AREA(img, lbl)							momento((img), 0, 0, 0, 0, (lbl))
#define I_BAR_COORD(img, lbl)					momento((img), 1, 0, 0, 0, (lbl)) / AREA((img), (lbl))
#define J_BAR_COORD(img, lbl)					momento((img), 0, 1, 0, 0, (lbl)) / AREA((img), (lbl))
#define I_MOMENT_OF_INERTIA(img, lbl)			momento((img), 2, 0, 0, 0, (lbl))
#define J_MOMENT_OF_INERTIA(img, lbl)			momento((img), 0, 2, 0, 0, (lbl))
#define DEVIATION_MOMENT(img, lbl)				momento((img), 1, 1, 0, 0, (lbl))

/* Da questo potrei poi definire gli inviarianti di Hu, ma per ora non sono necessari. 
   Sarebbe meglio dovrebbero ottimizzare i calcoli in virgola mobile...*/
#define V(img, lbl, m, n)						momento((img), (m), (n), I_BAR_COORD(img, lbl), J_BAR_COORD(img, lbl), (lbl)) / pow((double) AREA((img), (lbl)), (double)(((m)+(n))/2) + 1.0)

/* Invarianti di Hu */
#define H1(img, lbl)							V((img), (lbl), 2,0) + V((img), (lbl), 0, 2)
/* Ecc. ecc.*/
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//				Definizione Tipi di Dato
//----------------------------------------------------------------------------
typedef struct {
	unsigned char id;
	STRING classe;
	int area;
	int perimetro;
	double compattezza;
	double haralick;
	double h1;
} OBJECT_DESC;

typedef struct {
	OBJECT_DESC* objects [MAX_NUM_OBJECTS_IN_SCENE];
	int length;
} OBJECTS_LIST;

// Puntatore alla funzione di valutazione di un oggetto in base
// ai suoi descrittori calcolati. 
typedef STRING* (*evalFunc) (OBJECT_DESC*);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//				Prototipi Funzioni
//----------------------------------------------------------------------------
void invertBinary (IplImage* src, IplImage* dest);
// long longPow (long base, long exp); // Serve solo internamente alla libreria

// Segmentazione e Riconoscimento
int segmentation (IplImage* img);
OBJECTS_LIST* objectRecognition (IplImage* src, int numLabel, evalFunc f);
void areaOpening (IplImage* src, IplImage* dest, int numLabel, int soglia);
void findContours(IplImage* src, IplImage* dest);
void findFalseObjects(IplImage* img, IplImage* edges, OBJECTS_LIST* objects);
void deleteObjectsList (OBJECTS_LIST* list);
void binarizeSegmentedImage (IplImage* img);

// Calcolo dei descrittori degli oggetti
long momento (IplImage* src, int ordI, int ordJ, int bCoordI, int bCoordJ, int numLabel);
int findAreaDesc (IplImage* src, int numLabel);
int findPerimeterDesc (IplImage* src, int numLabel);
double findCompactnessDesc (int area, int perimetro);
double findHaralickDesc (IplImage* src, int numLabel);
//----------------------------------------------------------------------------


//****************************************************************************
//				IMPLEMENTAZIONE
//	(da spostare in un file c o cpp separato)
//****************************************************************************
long longPow (long base, long exp) {
	if (exp == 0) return 1;
	return base * longPow(base, exp - 1);
}

void deleteObjectsList (OBJECTS_LIST* list){
	for (int i = 0; i < list->length; i++){
		strDelete(&(list->objects[i]->classe));
		free(list->objects[i]);
	}

	free(list);
}

void invertBinary (IplImage* src, IplImage* dest) {
	unsigned char* srcData = (unsigned char*) src->imageData;
	unsigned char* imgData = (unsigned char*) dest->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;

	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			imgData[i * ws + j] = FOREGROUND - srcData[i * ws + j];
}

//----------------------------------------------------------------------------
//			CALCOLO DEI DESCRITTORI
//----------------------------------------------------------------------------
long momento (IplImage* src, int ordI, int ordJ, int bCoordI, int bCoordJ, int numLabel){
	unsigned char* srcData = (unsigned char*) src->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;
	long momento = 0;

	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			if (srcData[i * ws + j] == numLabel)
				momento += longPow(i - bCoordI, ordI) * 
						   longPow(j - bCoordJ, ordJ);

	return momento;
}

int findAreaDesc (IplImage* src, int numLabel) {
	unsigned char* srcData = (unsigned char*) src->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;
	int area = 0;

	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			if (srcData[i * ws + j] == numLabel) area++;

	return area;
}

int findPerimeterDesc (IplImage* src, int numLabel) {
	IplImage* temp = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	unsigned char* srcData = (unsigned char*) src->imageData;
	unsigned char* tempData = (unsigned char*) temp->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;
	
	double perimetro = 0.0;
	int i, j; int flag = 0;
	
	for (i = 0; i < h; i++)
		for (j = 0; j < w; j++)
			tempData[i * ws + j] = BACKGROUND;
	
	// Estrazione curva di contorno
	for (i = 0; i < h; i++)
		for (j = 0; j < w; j++)
			if (srcData[i * ws + j] == numLabel)
				if (srcData[(i - 1) * ws + j] == numLabel &&
					srcData[(i + 1) * ws + j] == numLabel &&
					srcData[i * ws + (j - 1)] == numLabel &&
					srcData[i * ws + (j + 1)] == numLabel)
					tempData[i * ws + j] = BACKGROUND;
				else tempData[i * ws + j] = FOREGROUND;

	// Trovo il primo punto da analizzare
	for (i = 0; i < h; i++){
		for (j = 0; j < w; j++) if (tempData[i * ws + j] == FOREGROUND) { flag = 1; break; }
		if (flag == 1) break;
	}

	// Inseguimento del contorno...
	while (1) {
		//flag = 0;

		/*  ATTENZIONE alla disposizione degli IF:
		*
		*	le nelle curve 8 connesse controllare prima gli spostamenti
		*	orizzontali e verticali, dopodiché quelli diagonali; altrimenti
		*	c'è il rischio di terminare l'analisi prima del dovuto.
		*/
		if (i > 0 && tempData[(i - 1) * ws + j] == FOREGROUND){
			perimetro += 1.0;
			i = i - 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(i < h - 1 && tempData[(i + 1) * ws + j] == FOREGROUND){
			perimetro += 1.0;
			i = i + 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(j < w - 1 && tempData[i * ws + (j + 1)] == FOREGROUND){
			perimetro += 1.0;
			j = j + 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(j > 0 && tempData[i * ws + (j - 1)] == FOREGROUND){
			perimetro += 1.0;
			j = j - 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(i > 0 && j > 0 && tempData[(i - 1) * ws + (j - 1)] == FOREGROUND){
			perimetro += sqrt(2.0);
			i = i - 1;
			j = j - 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(i > 0 && j < w - 1 && tempData[(i - 1) * ws + (j + 1)] == FOREGROUND){
			perimetro += sqrt(2.0);
			i = i - 1;
			j = j + 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(i < h - 1 && j > 0 && tempData[(i + 1) * ws + (j - 1)] == FOREGROUND){
			perimetro += sqrt(2.0);
			i = i + 1;
			j = j - 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		else if(i < h - 1 && j < w - 1 && tempData[(i + 1) * ws + (j + 1)] == FOREGROUND){
			perimetro += sqrt(2.0);
			i = i + 1;
			j = j + 1;
			tempData[i * ws + j] = BACKGROUND;
			continue;
		}
		
		break;
	}
	
	cvReleaseImage(&temp);

	return ((int) perimetro);
}

double findCompactnessDesc (int area, int perimetro){
	return (double) 4.0 * PI * area / longPow(perimetro, 2);
}


//	-- findHaralickDesc: --
// Il descrittore non è quello presentato sulle slide (ovvero
// media fratto varianza), ma è l'inverso: varianza fratto media.
// Questo perché il cerchio dovrebbe avere varianza nulla ed, in
// quel caso, si avrebbe una divisione per zero.
double findHaralickDesc (IplImage* src, int numLabel){
	unsigned char* srcData = (unsigned char*) src->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;

	int ib = I_BAR_COORD(src, numLabel);
	int jb = J_BAR_COORD(src, numLabel);
	int area = findAreaDesc(src, numLabel);

	double media = 0.0;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			if (srcData[i * ws + j] == numLabel)
				media += sqrt((double) longPow(i - ib, 2) + longPow(j - jb, 2));
	media /= area;

	double var = 0.0;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			if (srcData[i * ws + j] == numLabel)
				var += pow(sqrt((double) longPow(i - ib, 2) + longPow(j - jb, 2)) - media, 2.0);
	var /= area;

	return var / media;
}
//----------------------------------------------------------------------------

void binarizeSegmentedImage (IplImage* img){
	unsigned char* imgData = (unsigned char*) img->imageData;
	int h = img->height; int w = img->width; int ws = img->widthStep;

	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			if (imgData[j * ws + i] != BACKGROUND)
				imgData[j * ws + i] = FOREGROUND;
}

int segmentation (IplImage* img) {
	unsigned char* imgData = (unsigned char*) img->imageData;
	int h = img->height; int w = img->width; int ws = img->widthStep;

	unsigned char equiv [256][256];
	unsigned char LUT [256];

	// Inizializzazione LUT e matrice delle equivalenze
	for (int i = 0; i < 256; i++) {
		LUT[i] = i;
		for (int j = 0; j < 256; j++)
			if (i != j) equiv[i][j] = 0;
			else equiv[i][j] = 1;
	}

	// Inizializzazione a BACKGROUND dei bordi superiore,
	// sinistro, destro e inferiore dell'immagine...
	for (int i = 0; i < h; i++) {
		imgData[i * ws] = BACKGROUND;
		imgData[i * ws + (w - 1)] = BACKGROUND;
	}
	for (int j = 0; j < w; j++){
		imgData[j] = BACKGROUND;
		imgData[(h - 1) * ws + j] = BACKGROUND;
	}

	// Prima scansione
	unsigned char maxLabel = 1;
	for (int i = 1; i < h - 1; i++){
		for (int j = 1; j < w - 1; j++){
			if (imgData[i * ws + j] == FOREGROUND){			
				unsigned char lp = imgData[(i - 1) * ws + j];
				unsigned char lq = imgData[i * ws + (j - 1)];
				unsigned char lx = lp;

				if (lp == BACKGROUND && lq == BACKGROUND){
					lx = maxLabel;  // ...
					maxLabel++;		// lx = maxLabel++;
				}
				// Trovata un'equivalenza
				else if((lp != lq) && (lp != BACKGROUND) && (lq != BACKGROUND)){
					equiv [lp][lq] = 1; equiv [lq][lp] = 1;
					lx = lq;
				}
				else if (lq != BACKGROUND) lx = lq;
				else if (lp != BACKGROUND) lx = lp;

				imgData[i * ws + j] = lx;
			}
		}
	}
	
	// Trovo le equivalenze (propago -> transitività)
	// TODO: Ottimizzare -> 'equiv' è una matrice quadrata simmetrica...
	for (int i = 0; i < maxLabel; i++)
		for (int j = 0; j < maxLabel; j++)
			if (equiv[i][j] == 1 && i != j)
				for (int k = 0; k < maxLabel; k++){
					equiv[i][k] = equiv[i][k] || equiv[j][k];
					equiv[k][i] = equiv[i][k];
				}

	int numLabel = maxLabel - 1;
	
	// Inserisco nella LUT l'informazione delle equivalenze
	// Versione ottimizzata...
	for (int i = 0; i < maxLabel; i++)
		for (int j = i + 1; j < maxLabel; j++)
			if (equiv[i][j] == 1 /*&& i != j*/){
				LUT[j] = i;
				numLabel--; // Aggiunto
				for (int k = 0; k < maxLabel; k++) {
					equiv[j][k] = 0;
					equiv[k][j] = 0;
				}
			}
	
	//-------------------- Aggiunto ---------------------------------------
	// Assegno label continue e ordinate alla LUT
	unsigned char valLabel = 1;
	unsigned char appoggio [256];
	for (int i = 1; i < maxLabel; i++) appoggio[i] = 0;
	// ATTENZIONE: i parte da 1 -> appoggio[0] = 0 -> BACKGROUND
	for (int i = 1; i < maxLabel; i++){
		if (appoggio[LUT[i]] == 0) { 
			appoggio[LUT[i]] = valLabel; 
			valLabel++;
		}
		LUT[i] = appoggio[LUT[i]];
	}
	//------------------------------------------------------------------
	
	// Seconda scansione
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			imgData[i * ws + j] = LUT[imgData[i * ws + j]];
	
	return numLabel;
}

void areaOpening (IplImage* src, IplImage* dest, int numLabel, int soglia) {
	unsigned char* srcData = (unsigned char*) src->imageData;
	unsigned char* destData = (unsigned char*) dest->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;
	
	unsigned char* matches = (unsigned char*) malloc(sizeof(unsigned char) * numLabel);
	for (int i = 0; i < numLabel; i++) matches[i] = 0;

	for (unsigned char l = 0; l < numLabel; l++){
		int area = findAreaDesc (src, l + 1); // Più veloce di AREA (src, l+1);
		if (area < soglia) matches[l] =  BACKGROUND;
		else matches[l] = l + 1;
	}

	int nl;
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) 
			if((nl = srcData[i * ws + j] - 1) >= 0)
				destData[i * ws + j] = matches[nl];
			else destData[i * ws + j] = BACKGROUND;

	free (matches);
}

OBJECTS_LIST* objectRecognition (IplImage* src, int numLabel, evalFunc f) {
	unsigned char* srcData = (unsigned char*) src->imageData;
	int h = src->height; int w = src->width; int ws = src->widthStep;

	OBJECTS_LIST* list = (OBJECTS_LIST*) malloc(sizeof(OBJECTS_LIST));
	list->length = 0;

	// Classificazione degli Oggetti.
	for (unsigned char l = 0; l < numLabel; l++){
		// Calcolo dei descrittori.
		int area = findAreaDesc (src, l+1); // Più veloce di AREA (src, l+1);
		
		/*------------------------------------------------------------------*/
		/* Se uno (pseudo-)oggetto labellato ha area nulla, significa che è
		 * intervenuto in precedenza l'operatore di area-opening e l'oggetto
		 * in questione segmentato è stato ritenuto non significativo e da
		 * eliminare; per questo motivo non si procede con la valutazione.  
		 *------------------------------------------------------------------*/
		if (area == 0) continue;
		else list->length++;
		/*------------------------------------------------------------------*/
		
		int perimetro = findPerimeterDesc(src, l+1);
		double compattezza = findCompactnessDesc (area, perimetro);
		// Non lo trovo per motivi di velocità computazionale.
		double haralick = 0.0; /*findHaralickDesc(src, l+1);*/
		double h1 = 0.0; /*H1(src, l+1);*/

		OBJECT_DESC* object = (OBJECT_DESC*) malloc (sizeof(OBJECT_DESC));
		object->id = l+1;
		object->area = area;
		object->compattezza = compattezza;
		object->perimetro = perimetro;
		object->haralick = haralick;
		object->h1 = h1;

		STRING* classe = f(object);
		object->classe = *classe;

		list->objects[list->length - 1] = object;
		//-------------------------------------------------------------
	}

	return list;
}

/*
	Riduce un'immagine segmentata ai contorni degli oggetti...
*/
void findContours(IplImage* src, IplImage* dest) {
	int h = src->height; int w = src->width; int ws = src->widthStep;
	IplImage* temp = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	IplConvKernel* kernel = cvCreateStructuringElementEx(5, 5, 2, 2, CV_SHAPE_CROSS, NULL);

	unsigned char* srcData = (unsigned char*) src->imageData;
	unsigned char* tempData = (unsigned char*) temp->imageData;
	unsigned char* destData = (unsigned char*) dest->imageData;

	/* ATTENZIONE: Operatore morfologico utilizzato in un'immagine a scala
				   di grigi. Per come è strutturata l'immagine segmentata
				   funziona bene nella rilevazione del contorno e quindi è
				   stato utilizzato senza troppe preoccupazioni.
	*/
	cvErode(src, temp, kernel, 2); cvSub(src, temp, dest, NULL);
	
	cvReleaseImage(&temp);
	cvReleaseStructuringElement(&kernel);
}

/*
	A partire dai contorni degli oggetti di un'immagine segmentata e dagli
	edge della stessa immagine, individua quali sono i falsi oggetti.
*/
void findFalseObjects(IplImage* contours, IplImage* edges, OBJECTS_LIST* objects, int threshold) {
	unsigned char* contoursData = (unsigned char*) contours->imageData;
	unsigned char* edgesData = (unsigned char*) edges->imageData;
	int h = contours->height; int w = contours->width; int ws = contours->widthStep;
	int counter;

	for(int k = 0; k < objects->length; k++){
		counter = 0;

		for(int j = 0; j < h; j++)
			for(int i = 0; i < w; i++)
				if (contoursData[j * ws + i] == objects->objects[k]->id) 
					if (edgesData[j * ws + i] != 0) counter++;

		if (counter > threshold) continue;
		//else strAssign (&(objects->objects[k]->classe), "FalseObj");
		else strAppend (&(objects->objects[k]->classe), "#");
	}
}

//****************************************************************************

#endif /*_CHANGEDETECTIONLIB_H*/

