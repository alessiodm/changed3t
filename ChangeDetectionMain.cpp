/****************************************************************************
*						ELABORAZIONE DELL'IMMAGINE LS						*
*						   Tesina Change Detection							*
*							 Alessio Della Motta							*
****************************************************************************/

#include <stdio.h>
#include "cv.h"
#include "highgui.h"
#include "changeDetectionLib.h"
#include "myString.h"

//----------------------------------------------------------------------------
// Definizione di alcuni parametri dell'applicazione.
//----------------------------------------------------------------------------
// Modificare BACKGROUND_TRAINING da 1 a 0 per far calcolare
// il background una volta sola alla fine della raccolta delle statistiche;
// lasciare 1 per vedere come si forma il background.
//----------------------------------------------------------------------------
#define BACKGROUND_TRAINING						0
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Definizione Costanti
//----------------------------------------------------------------------------
// Informazioni sulle Immagini Catturate dal Video
#define N_GRAY_LEVELS							256
#define FRAME_WIDTH								320
#define FRAME_HEIGHT							240

#define VIDEO_FILE_NAME							"./video.avi"

// N. Frame di raccolta delle statistiche per il
// background initialization.
#define N_FRAME_BACKGROUND_INITIALIZIATION		55

// Output su File.
#define FILE_NAME								"./change_data.txt"
#define FLUSH_TIME								100

#define MIN_AREA_LIBRO							500
#define MAX_AREA_LIBRO							1600
#define MIN_AREA_PERSONA						5000

// Parametri Area-Opening
#define AREA_OPENING_CONST						500
#define BACKGR_AREA_OPENING_CONST				2000

// Parametri Erosioni e Dilatazioni
#define DILATE_CONST							7
#define ERODE_CONST								4

// Soglia di Background Difference
#define THRESHOLD								25   // 27	 //25  //* 25

// Background Updating
#define BK_THRESHOLD							15 // 10   // 15   //15  //*20 //10 //6
#define ALFA									0.1 // 0.02 // 0.03 //0.1 //*0.1   //  0.02 //0.95

#define BACKGROUND_NO_MOD						0
#define BACKGROUND_POS							1
#define BACKGROUND_NEG							2

#define BACKGROUND_INCR							220 // 30 // 220 // 180

#define FALSE_OBJECT_THRESHOLD					70

// Titoli delle Finestre Video
#define VIDEO_WINDOW							"Video"
#define BKG_WINDOW								"Background"
#define BACKGROUND_SUBTRACTION_WINDOW			"Background Subtraction"
#define OUTPUT_WINDOW							"Output"
#define EDGES_WINDOW							"Canny Edges"

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//				Prototipi Funzioni
//----------------------------------------------------------------------------
// Per stimare il background...
int media(int j, int i);
int mediana(int j, int i);
int moda(int j, int i);

// Output su File
void appendObjectInfo (OBJECT_DESC object, STRING* dest);
void writeInfo(int frame_number, OBJECTS_LIST list);

//Creazione Immagine di Output
void createOutput(IplImage* src, IplImage* contours, IplImage* output, OBJECTS_LIST* list);

// Funzione di riconoscimento oggetti in base ai descrittori
STRING* myEvaluationFunction (OBJECT_DESC* object);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//				Variabili Globali
//----------------------------------------------------------------------------
// Immagini che si devono gestire.
IplImage* curFrame = 0; // Immagine contenente il frame corrente all'istante t
IplImage* lastFrame = 0;
IplImage* frameDiff = 0; // Immagine di Background Subtraction
IplImage* bkgDiff = 0;
IplImage* background = 0; // Immagine contenente il background
IplImage* output = 0;
IplImage* edges = 0;

IplConvKernel* kernel = 0;

unsigned char* curFrameData; 
unsigned char* backgroundData; 
unsigned char* frameDiffData;
unsigned char* bkgDiffData;
unsigned char* lastFrameData;
unsigned char* outputData;
unsigned char* edgesData;

// Output su File
int flushCounter = 0;
STRING toFile; // Informazioni che andranno scritte su file.
FILE* out;

// Informazioni sul Background e sul Frame Corrente
int histos [FRAME_HEIGHT][FRAME_WIDTH][N_GRAY_LEVELS];
int backgroundIncrCounter[FRAME_HEIGHT * FRAME_WIDTH];
int bkGenerated = 0; // Indica se il background è stato generato.
int frame_number = -1; // Contatore dei frame.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Tipi, prototipi e variabili per acquisire da file avi
//----------------------------------------------------------------------------
typedef struct {
		char * file_name;
		IplImage* frame;
} AVI_READER;	

int open_avi(AVI_READER *);
char* get_next_frame();
int display_images(int delay);
int close_avi();
void release();
void elab(IplImage* inputImage);

CvCapture* cap = 0;
AVI_READER* avi_struct = 0;
IplImage* ipl = 0;
IplImage* copyIpl = 0;
//----------------------------------------------------------------------------



/*****************************************************************************
	Funzione Corpo delle Elaborazioni.
******************************************************************************/
void elab(IplImage* inputImage) {
	cvCvtColor (inputImage, curFrame, CV_BGR2GRAY);
	int h = curFrame->height; int w = curFrame->width;
	int ws = curFrame->widthStep;
	
	printf("FRAME NUMBER: %d\r", frame_number);

	//-----------------------------------------------------------------//
	//				BACKGROUND INITIALIZATION						   //
	//-----------------------------------------------------------------//
	if (frame_number < N_FRAME_BACKGROUND_INITIALIZIATION) {	
		for(int j = 0; j < h; j++)
			for(int i = 0; i < w; i++){
				unsigned char index = curFrameData[j * ws + i];
				histos[j][i][index]++;
				
		#if BACKGROUND_TRAINING//................................
				backgroundData[j * ws + i] = (unsigned char) mediana(j, i);
			}
		cvShowImage(BKG_WINDOW, background);
		#else
			}
		#endif//.................................................

		return;
	}
	
	//...........Per velocizzare la fase di training.............
	#if !BACKGROUND_TRAINING//...................................
	if (!bkGenerated){
		for(int j = 0; j < h; j++)
			for(int i = 0; i < w; i++)
				backgroundData[j * ws + i] = (unsigned char) mediana(j, i);
		bkGenerated = 1;
		
		//cvShowImage(BKG_WINDOW, background);
		//cvWaitKey(0);
	}
	#endif//.....................................................
	//-----------------------------------------------------------------//

	//-----------------------------------------------------------------//
	//				BACKGROUND UPDATING (SELECTIVE MOD)				   //
	//-----------------------------------------------------------------//
	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			// Background Selective Classico
			if (bkgDiffData[j * ws + i] == BACKGROUND_NO_MOD)
				backgroundData[j * ws + i] = ALFA * lastFrameData[j * ws + i] + 
											(1.0 - ALFA) * backgroundData[j * ws + i];
			else if (bkgDiffData[j * ws + i] == BACKGROUND_POS)
				if (backgroundIncrCounter[j * w + i] < BACKGROUND_INCR) 
					backgroundIncrCounter[j * w + i]++;
				else{
					if (backgroundData[j * ws + i] < 255) backgroundData[j * ws + i] += 1;
					backgroundIncrCounter[bkgDiffData[j * w + i]] = 0;
				}
			else if (bkgDiffData[j * ws + i] == BACKGROUND_NEG)
				if (backgroundIncrCounter[j * w + i] < BACKGROUND_INCR) 
					backgroundIncrCounter[j * w + i]++;
				else{
					if (backgroundData[j * ws + i] > 0) backgroundData[j * ws + i] -= 1;
					backgroundIncrCounter[j * w + i] = 0;
				}
	//-----------------------------------------------------------------//


	//-----------------------------------------------------------------//
	//			FRAME & BACKGROUND DIFFERENCE (BINARIZZAZIONE)		   //
	//-----------------------------------------------------------------//
	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			if (abs(curFrameData[j * ws + i] - backgroundData[j * ws + i]) > THRESHOLD)
				frameDiffData[j * ws + i] = FOREGROUND;
			else frameDiffData[j * ws + i] = BACKGROUND;

				
	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			if (abs(curFrameData[j * ws + i] - backgroundData[j * ws + i]) > BK_THRESHOLD){
				if (curFrameData[j * ws + i] - backgroundData[j * ws + i] > 0)
					bkgDiffData[j * ws + i] = BACKGROUND_POS;
				else bkgDiffData[j * ws + i] = BACKGROUND_NEG;
			}
			else bkgDiffData[j * ws + i] = BACKGROUND;
	//-----------------------------------------------------------------//


	//-----------------------------------------------------------------//
	//			ELABORAZIONI MORFOLOGICHE & AREA OPENING			   //
	//-----------------------------------------------------------------//	

	// Erosione preliminare...
	cvErode(frameDiff, frameDiff, NULL, 1);
	// Dilatazione...
	cvDilate(frameDiff, frameDiff, kernel, DILATE_CONST);
	
	/*-----------------------------------------------------------------------------
		Eliminazione lacune dagli oggetti -> area-opening inverso (falsi negativi).
		Lo faccio per l'assunzione che sulla scena non siano presenti oggetti con
		cavità o per lo meno non interessi discriminare negli oggetti presenti
		cavità di area maggiore a 7000.
	------------------------------------------------------------------------------*/
	int numLabel;
	
	invertBinary(frameDiff, frameDiff); // Inverto l'immagine -> sfondo diventa FOREGROUND
	numLabel = segmentation (frameDiff);
	areaOpening (frameDiff, frameDiff, numLabel, BACKGR_AREA_OPENING_CONST);
	binarizeSegmentedImage(frameDiff); // Binarizzo nuovamente...
	invertBinary(frameDiff, frameDiff);

	// ...ed erosione.
	cvErode(frameDiff, frameDiff, kernel, ERODE_CONST);

	// Calcolo Componenti Connesse e Area Opening Falsi Positivi
	numLabel = segmentation (frameDiff);
	areaOpening (frameDiff, frameDiff, numLabel, AREA_OPENING_CONST);
	/* Binarizzo e segmento nuovamente...
	 (non necessario, si farà solo qualche ciclo in più in objectRecognition). 
	 Lo faccio per la visualizzazione della finestra. */
	binarizeSegmentedImage(frameDiff);
	cvShowImage(BACKGROUND_SUBTRACTION_WINDOW, frameDiff);
	numLabel = segmentation (frameDiff);

	// Object Recognition
	OBJECTS_LIST* objects = objectRecognition (frameDiff, numLabel, &myEvaluationFunction);
	//-----------------------------------------------------------------//
	
	cvCopyImage(curFrame, lastFrame);
	
	//-----------------------------------------------------------------//
	//			CREAZIONE DELL'IMMAGINE DI OUTPUT					   //
	//-----------------------------------------------------------------//
	findContours(frameDiff, frameDiff);
	// Individuazione falsi oggetti...
	cvCanny(curFrame, edges, 130.0,  150.0, 3);
	findFalseObjects(frameDiff, edges, objects, 60);

	createOutput(curFrame, frameDiff, output, objects);
	//-----------------------------------------------------------------//
	
	// Scrittura su File (o bufferizzazione)...
	writeInfo(frame_number, *objects);
	// Libero la memoria occupata dai descrittori degli oggetti
	deleteObjectsList(objects);

	return;
}
/******************************************************************************/

/*----------------------------------------------------*/
/*	Implementato qui l'algoritmo di riconoscimento
	degli oggetti a partire dalle loro features.
	L'algoritmo è molto semplice e si basa solo
	sull'area; nonostante questo ha buone capacità
	di riconoscimento in questo caso particolare.	   */
/*-----------------------------------------------------*/
STRING* myEvaluationFunction (OBJECT_DESC* object) {
	STRING classe;
	if (object->area > MIN_AREA_PERSONA)
		classe = strCreate("Persona");
	else if (object->area > MIN_AREA_LIBRO && 
			 object->area < MAX_AREA_LIBRO)
		classe = strCreate("Libro");
	else classe = strCreate("Altro");

	return &classe;
}
/*---------------------------------------------------*/

int media(int j, int i) {
	int nvalues = 0;
	int mu = 0;
	for (int k = 0; k < N_GRAY_LEVELS; k++){
		nvalues += histos[j][i][k];
		mu += k * histos[j][i][k];
	}
	
	return mu / nvalues;
}

int mediana(int j, int i) {
	int nvalues = 0; int counter = 0; int k;
	
	for (k = 0; k < N_GRAY_LEVELS; k++) nvalues += histos[j][i][k];
	double numPrc = nvalues * 0.5;

	for (k = 0; k < N_GRAY_LEVELS; k++){
		counter += histos[j][i][k];
		if (counter >= numPrc) return k;
	}

	return k;
}

int moda(int j, int i) {
	int md = 0;
	for (int k = 1; k < N_GRAY_LEVELS; k++)
		if (histos[j][i][k] > histos[j][i][md]) md = k;

	return md;
}

void appendObjectInfo (OBJECT_DESC object, STRING* dest){
	char buffer[20];
	
	itoa(object.id, buffer, 10);
	
	strAppend(dest, "ID:");
	strAppend(dest, buffer);
	strAppend(dest, "\t");

	strAppend(dest, "CLASSE:");
	strAppend(dest, object.classe);
	strAppend(dest, "\t");

	itoa(object.area, buffer, 10);
	strAppend(dest, "AREA:");
	strAppend(dest, buffer);
	strAppend(dest, "\t");

	itoa(object.perimetro, buffer, 10);
	strAppend(dest, "PERIMETRO:");
	strAppend(dest, buffer);
	strAppend(dest, "\t");

	sprintf(buffer, "%f", object.compattezza);
	strAppend(dest, "COMPATTEZZA:");
	strAppend(dest, buffer);
	strAppend(dest, "\t");

	/*sprintf(buffer, "%f", object.haralick);
	strAppend(dest, "HARALICK:");
	strAppend(dest, buffer);
	strAppend(dest, "\t");

	sprintf(buffer, "%f", object.h1);
	strAppend(dest, "HU1:");
	strAppend(dest, buffer);*/
	
	strAppend(dest, "\n\r");

	// Ulteriori informazioni su descrittori...
	// TODO
}

void writeInfo(int frame_number, OBJECTS_LIST list){
	char buffer[10];
	strAppend(&toFile, "FRAME-NUM:");
	itoa(frame_number, buffer, 10);
	strAppend(&toFile, buffer);

	strAppend(&toFile, "\tNUM_OBJ:");
	itoa(list.length, buffer, 10);
	strAppend(&toFile, buffer);
	strAppend(&toFile, "\n\r");

	for (int i = 0; i < list.length; i++)
		appendObjectInfo(*(list.objects[i]), &toFile);

	strAppend(&toFile, "\n\r");

	if (flushCounter > FLUSH_TIME){
		fputs(toFile, out);
		fflush(out);
		strAssign(&toFile, "");
		flushCounter = 0;
	} else flushCounter++;
}



void createOutput(IplImage* src, IplImage* contours, IplImage* output, OBJECTS_LIST* list) {
	unsigned char* srcData = (unsigned char*) src->imageData;
	unsigned char* outputData = (unsigned char*) output->imageData;
	unsigned char* contoursData = (unsigned char*) contours->imageData;
	int h = contours->height; int w = contours->width;
	int ws = contours->widthStep;
	int outputws = output->widthStep;

	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			if (contoursData[j * ws + i] != 0){
				for (int k = 0; k < list->length; k++) {
					if (contoursData[j * ws + i] == list->objects[k]->id){
						int len = strLength(list->objects[k]->classe);
						if (strCompare(list->objects[k]->classe, "Persona") == 0){
							outputData[j * outputws + i * 3] = 70;
							outputData[j * outputws + i * 3 + 1] = 70;
							outputData[j * outputws + i * 3 + 2] = 255;
							break;
						}
						else if (strCompare(list->objects[k]->classe, "Libro") == 0){
							outputData[j * outputws + i * 3] = 255;
							outputData[j * outputws + i * 3 + 1] = 150;
							outputData[j * outputws + i * 3 + 2] = 70;
						}
						else if (strCompare(list->objects[k]->classe, "Altro") == 0){
							outputData[j * outputws + i * 3] = 187;
							outputData[j * outputws + i * 3 + 1] = 240;
							outputData[j * outputws + i * 3 + 2] = 253;
						}
						//else if (strCompare(list->objects[k]->classe, "FalseObj") == 0){
						else if (list->objects[k]->classe[len-1] == '#'){					
							outputData[j * outputws + i * 3] = 92;
							outputData[j * outputws + i * 3 + 1] = 234;
							outputData[j * outputws + i * 3 + 2] = 152;
						}
					}
				}
			}
			else {
				outputData[j * outputws + i * 3] = srcData[j * ws + i];
				outputData[j * outputws + i * 3 + 1] = srcData[j * ws + i];
				outputData[j * outputws + i * 3 + 2] = srcData[j * ws + i];
			}
}

int main(int argc, char** argv){
	AVI_READER avi;
	avi.file_name = VIDEO_FILE_NAME;
	int res = open_avi(&avi);
	
	if(res != 1) { printf("File video non esistente...\nTermino...\n");  return -1; }
	frame_number = 0; char retcode = -1; 
	out = fopen(FILE_NAME, "w");	
	if (out == NULL) { printf("Impossibile aprire il file...\nTermino...\n"); return -1; }
	toFile = strCreate("");

	//------------- BACKGROUND DETECTION INIT ------------------------
	// Inizializzazione statistiche background
	//----------------------------------------------------------------
	for (int i = 0; i < FRAME_HEIGHT * FRAME_WIDTH; i++)
		backgroundIncrCounter[i] = 0;

	for(int j = 0; j < FRAME_HEIGHT; j++)
		for(int i = 0; i < FRAME_WIDTH; i++)
			for(int k = 0; k < N_GRAY_LEVELS; k++)
				histos[j][i][k] = 0;
	
	CvSize in_size; 
	in_size.height = FRAME_HEIGHT; 
	in_size.width = FRAME_WIDTH;

	curFrame = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	lastFrame = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	frameDiff = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	bkgDiff = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	background = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	edges = cvCreateImage(in_size, IPL_DEPTH_8U, 1);
	output = cvCreateImage(in_size, IPL_DEPTH_8U, 3);
	
	kernel = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_RECT, NULL);
	
	curFrameData = (unsigned char*) curFrame->imageData;
	backgroundData = (unsigned char*) background->imageData;
	frameDiffData = (unsigned char*) frameDiff->imageData;
	bkgDiffData = (unsigned char*) bkgDiff->imageData;
	lastFrameData = (unsigned char*) lastFrame->imageData;
	edgesData = (unsigned char*) edges->imageData;
	outputData = (unsigned char*) output->imageData;

	for(int j = 0; j < FRAME_HEIGHT; j++)
		for(int i = 0; i < FRAME_WIDTH; i++){
			backgroundData[j * background->widthStep + i] = BACKGROUND;
			edgesData[j * edges->widthStep + i] = BACKGROUND;
			lastFrameData[j * lastFrame->widthStep + i]  = BACKGROUND;
			bkgDiffData[j * bkgDiff->widthStep + i] = 10; // Fondamentale!!!

			outputData[j * output->widthStep + i * 3] = BACKGROUND;
			outputData[j * output->widthStep + i * 3 + 1] = BACKGROUND;
			outputData[j * output->widthStep + i * 3 + 2] = BACKGROUND;
		}

	cvNamedWindow(BACKGROUND_SUBTRACTION_WINDOW, 0);
	cvNamedWindow(BKG_WINDOW, 0);
	cvNamedWindow(OUTPUT_WINDOW, 0);
	cvNamedWindow(EDGES_WINDOW, 0);

	cvResizeWindow(VIDEO_WINDOW, copyIpl->width, copyIpl->height);
	cvResizeWindow(BKG_WINDOW, FRAME_WIDTH, FRAME_HEIGHT);
	cvResizeWindow(BACKGROUND_SUBTRACTION_WINDOW, FRAME_WIDTH, FRAME_HEIGHT);
	cvResizeWindow(EDGES_WINDOW, FRAME_WIDTH, FRAME_HEIGHT);
	cvResizeWindow(OUTPUT_WINDOW, FRAME_WIDTH, FRAME_HEIGHT);
	//----------------------------------------------------------------
	
	while(get_next_frame() != 0 && retcode != 'q'){
		elab(avi.frame);
		//if (frame_number == 378) cvWaitKey(0); //169 //70
		retcode = (char)(display_images(40));
		frame_number++;
	}
	
	fputs(toFile, out);
	fclose(out);

	printf("\nPremi un tasto per uscire...\n");
	cvWaitKey(0);
	close_avi();
	release();
	return 1;
}
void release(){
	cvReleaseImage(&curFrame);
	cvReleaseImage(&lastFrame);
	cvReleaseImage(&frameDiff);
	cvReleaseImage(&bkgDiff);
	cvReleaseImage(&background);
	cvReleaseImage(&edges);
	cvReleaseImage(&output);

	cvReleaseStructuringElement(&kernel);

	cvDestroyWindow(VIDEO_WINDOW);
	cvDestroyWindow(BACKGROUND_SUBTRACTION_WINDOW);
	cvDestroyWindow(BKG_WINDOW);
	cvDestroyWindow(OUTPUT_WINDOW);
	cvDestroyWindow(EDGES_WINDOW);
}

int open_avi (AVI_READER* avi_h){
	avi_struct = avi_h;
	cap = cvCaptureFromAVI (avi_h->file_name);
	if (cap == 0){
		printf("Il file %s non è stato trovato\n", avi_h->file_name);
		cvWaitKey(0);
		return -1;
	}
	cvNamedWindow(VIDEO_WINDOW, 0);
	cvGrabFrame(cap);
	ipl = cvRetrieveFrame(cap);
	avi_struct->frame = ipl;
	if (ipl->nChannels != 3) {
		printf("Il file %s contiene immagini con nChannels != 3\n", avi_h->file_name);
		cvReleaseCapture(&cap);
		cvWaitKey(0);
		return -2;
	}

	if (ipl->height != FRAME_HEIGHT || ipl->width != FRAME_WIDTH){
		printf("Dimensioni frame non coerenti!");
		cvReleaseCapture(&cap);
		cvWaitKey(0);
		return -3;
	}

	copyIpl = cvCloneImage(ipl);
	cvFlip(copyIpl);
	copyIpl->origin = 0;
	avi_struct->frame = copyIpl;
	return 1;
}

char* get_next_frame(){
	int res = 0;
	char* imag = 0;
	if(ipl == 0) {
		res = cvGrabFrame(cap);
		if(res > 0){
			ipl = cvRetrieveFrame(cap);
			cvFlip(ipl, copyIpl);
			imag = copyIpl->imageData;
		}
	}
	else imag = copyIpl->imageData;

	ipl = 0;
	return imag;
}

int close_avi(){
	if (cap != 0) cvReleaseCapture(&cap);
	if (copyIpl != 0)cvReleaseImage(&copyIpl);
	return 1;
}

int display_images(int delay){
	cvShowImage(VIDEO_WINDOW, copyIpl);
	cvShowImage(BKG_WINDOW, background);
	cvShowImage(OUTPUT_WINDOW, output);
	cvShowImage(EDGES_WINDOW, edges);

	return cvWaitKey(delay);
}
