//
// Wortindices, sind unabhängig von der Buchstabenmaske, beschreiben die Position im Array der Worte
//
#define W_NOWORD			-1
#define W_UHR				0
#define W_FUENF_NACH		1
#define W_ZEHN_NACH			2
#define W_VIERTEL_NACH		3
#define W_ZWANZIG_NACH		4
#define W_FUENF_VOR_HALB	5
#define W_HALB				6
#define W_FUENF_NACH_HALB	7
#define W_ZWANZIG_VOR		8
#define W_VIERTEL_VOR		9
#define W_ZEHN_VOR			10
#define W_FUENF_VOR			11
#define W_ES_IST			12
#define W_ZWOELF			13
#define W_EINS				14
#define W_ZWEI				15
#define W_DREI				16
#define W_VIER				17
#define W_FUENF				18
#define W_SECHS				19
#define W_SIEBEN            20
#define W_ACHT              21
#define W_NEUN              22
#define W_ZEHN 				23
#define W_ELF 				24
#define W_EIN				25
#define W_WLAN				26
#define W_VIERTEL			27
#define W_ZEHN_VOR_HALB		28
#define W_ZEHN_NACH_HALB	29
#define W_DREIVIERTEL		30
#define W_UND				31
#define W_ZWANZIG			32
#define W_DREISSIG			33
#define W_PLUS				34
#define W_MINUS				35
#define W_GRAD				36
#define W_NULL				37
#define W_PUNKT				38
#define W_EIN_PUNKT			39
#define W_ZWEI_PUNKTE		40
#define W_DREI_PUNKTE		41
#define W_VIER_PUNKTE		42
#define W_ZEHNER			43
#define W_SIEB				44
#define W_SECH				45
#define W_HERZ				46
#define W_ARRAYGROESSE		47

#ifdef MyWC12x12_INO
//
// Wortlisten
//
int wordsindex_single_m[] = { W_NOWORD, W_EIN_PUNKT, W_ZWEI_PUNKTE, W_DREI_PUNKTE, W_VIER_PUNKTE};
int wordsindex_minutes[] = { W_UHR, W_FUENF_NACH, W_ZEHN_NACH, W_VIERTEL_NACH, W_ZWANZIG_NACH, W_FUENF_VOR_HALB, W_HALB, W_FUENF_NACH_HALB, W_ZWANZIG_VOR, W_VIERTEL_VOR, W_ZEHN_VOR, W_FUENF_VOR };
int wordsindex_hours[] = {W_ZWOELF, W_EINS, W_ZWEI, W_DREI, W_VIER, W_FUENF, W_SECHS, W_SIEBEN, W_ACHT, W_NEUN, W_ZEHN, W_ELF};
int wordsindex_ziffern[] = {W_NULL, W_EIN, W_ZWEI, W_DREI, W_VIER, W_FUENF, W_SECHS, W_SIEBEN, W_ACHT, W_NEUN};

// Ab wann (Minutenindex) wird von den "nach"-Angaben auf "vor"-Angaben umgeschaltet unf damit die tatsächliche Stunde um eine erhöht.
// Kann in uterschiedliche Lokalisierungen unterschiedlich ausfallen
int words_umschaltung_schwellwert = L_S_DEUTSCHLAND;


//
// Abbildung der Buchstabenmaske
// Welche LED ist für welches Wort zu aktivieren?
//
// Ergibt sich aus dem Layout der Maske. Jedes Wort wird mit -1 terminiert. Da pro Wort 13 ints vorgesehen werden, kann in einer 12x12-Matrix eine ganze Zeile mit einem Wort angeschaltet werden.
//
int words[W_ARRAYGROESSE][13] = {                				 // reicht für eine Zeile plus -1 als terminierender Wert
  { 134,133,132,											-1}, // uhr
  { 7,8,9,10, 44,43,42,41, 									-1}, // fuenf nach
  { 23,22,21,20,  44,43,42,41, 								-1}, // zehn nach
  { 29,30,31,32,33,34,35, 44,43,42,41,			 			-1}, // viertel nach
  { 12, 13, 14, 15, 16, 17, 18,  44, 43, 42, 41, 			-1}, // zwanzig nach
  {  7, 8, 9, 10,  47, 46, 45,  36, 37, 38, 39, 			-1}, // fuenf vor halb
  { 36, 37, 38, 39, 										-1}, // halb
  {  7, 8, 9, 10, 44, 43, 42, 41,  36, 37, 38, 39, 			-1}, // fuenf nach halb
  { 12, 13, 14, 15, 16, 17, 18, 45, 46, 47, 				-1}, // zwanzig vor
  { 29, 30, 31, 32, 33, 34, 35,  45, 46, 47, 				-1}, // viertel vor
  { 23, 22, 21, 20, 45, 46, 47,  							-1}, // zehn vor
  { 7, 8, 9, 10, 45, 46, 47,  								-1}, // fuenf vor
  { 0, 1, 3, 4, 5, 											-1}, // es ist !
  { 127,128,129,130,131,  									-1}, // zwoelf
  { 95,94,93,92,  											-1}, // eins
  { 87,86,85,84,  											-1}, // zwei
  { 91,90,89,88,  											-1}, // drei
  { 71,70,69,68,  											-1}, // vier
  { 63,62,61,60,  											-1}, // fuenf
  { 72,73,74,75,76, 										-1}, // sechs
  { 76,77,78,79,80,81, 										-1}, // sieben
  { 67,66,65,64,  											-1}, // acht
  { 96,97,98,99,  											-1}, // neun
  {100, 101, 102, 103,  									-1}, // zehn
  {104, 105, 106,  											-1}, // elf
  { 93,94,95,  												-1}, // ein
  { 11,35, 59, 83, 											-1}, // WLAN
  { 29, 30, 31, 32, 33, 34, 35, 							-1}, // viertel
  { 23,22,21,20, 45,46,47, 36,37,38,39, 					-1}, // zehn vor halb
  { 23,22,21,20, 44,43,42,41, 36,37,38,39, 					-1}, // zehn nach halb
  { 25,26,27,28,29,30,31,32,33,34,35,						-1}, // dreiviertel
  { 119,118,117,											-1}, // und
  { 120,121,122,123,124,125,126, 							-1}, // zwanzig
  { 115,114,113,112,111,110,109,108, 						-1}, // dreissig
  { 48,49,50,51, 											-1}, // plus
  { 54,55,56,57,58, 										-1}, // minus
  { 138, 137, 136, 135, 									-1}, // grad
  { 143,142,141,140, 										-1}, // null
  { 144, 													-1}, // punkt
  { 144, 													-1}, // ein punkt
  { 144, 145, 												-1}, // zwei punkte
  { 144, 145, 146, 											-1}, // drei punkte
  { 144, 145, 146, 147, 									-1}, // vier punkte
  { 100,101,102,103, 										-1}, // zehn (aber unten)
  { 76,77,78,79, 											-1}, // sieb (zehn)
  { 72,73,74,75, 											-1}, // sech (zehn)
  { 2,														-1}  // Herz
};
#else
extern int wordsindex_single_m[];
extern int wordsindex_minutes[];
extern int wordsindex_hours[];
extern int wordsindex_ziffern[];
extern int words_umschaltung_schwellwert;
extern int words[][13];
#endif
