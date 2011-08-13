/*
 * PoC / prolomeni captcha ochrany seznam.cz
 * OVX - Patrik Šíma
 * Únor 2009
 * 
 * Princip: registracni formulare Seznamu jsou chraneny captchou,
 * jejiz prolomeni je mozne pouze za pouziti komplikovanych ocr
 * nastroju. Pokud je captcha dobre nastavena je pouziti techto
 * nastroju temer vylouceno. Bohuzel pro uzivatele, cim bezpecnejsi
 * captcha je, tim hure je citelna.
 * I presto, ze je captcha Seznamu dobre citelna a uzivatel by nemel
 * mit problem s jejim prectenim, muze si pripadne lusteni ulehcit tim,
 * ze si obsah obrazku necha prehrat pomoci zvukove nahravky.
 * Souhlasim s tim, ze je to dobre pro uzivatele, ale v tomto provedeni
 * mene dobre pro bezpecnost.
 * Nyni jiz nemusime zkouset prolomit ochranu pomoci slozitych ocr nastroju,
 * ale staci nam, abychom z nekolika takovych nahravek ziskali vzorky
 * jednotlivych pismen, ktere pak snadno porovname s aktualne vygenerovanym
 * bezpecnostnim audio vzorkem, a petipismenny obsah je na svete.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sndfile.h> // knihovna pro zpracovani audio souboru

// struktura vzorku
typedef struct sample {
	char   letter;       // pismeno abecedy
	char  *filename;     // audio soubor
	short *dataptr;      // pointer na audio data
	long   size;         // velikost dat
} SAMPLE;

typedef struct match {
    char letter;         // pismeno abecedy
    long position;       // pozice, kde bylo nalezeno
} MATCH;

// audio vzorky pismen abecedy pro porovnavani
SAMPLE patterns[] = {
	{'A', "A.wav", NULL, 0},
	{'B', "B.wav", NULL, 0},
	{'C', "C.wav", NULL, 0},
	{'D', "D.wav", NULL, 0}, 
	{'E', "E.wav", NULL, 0},
	{'F', "F.wav", NULL, 0},
	{'G', "G.wav", NULL, 0},
	{'H', "H.wav", NULL, 0},
	{'I', "I.wav", NULL, 0},
	{'J', "J.wav", NULL, 0},
	{'K', "K.wav", NULL, 0},
	{'L', "L.wav", NULL, 0},
	{'M', "M.wav", NULL, 0},
	{'N', "N.wav", NULL, 0},
	{'O', "O.wav", NULL, 0},
	{'P', "P.wav", NULL, 0},
	{'Q', "Q.wav", NULL, 0},
	{'R', "R.wav", NULL, 0},
	{'S', "S.wav", NULL, 0},
	{'T', "T.wav", NULL, 0},
	{'U', "U.wav", NULL, 0},
	{'V', "V.wav", NULL, 0},
	{'X', "X.wav", NULL, 0},
	{'Y', "Y.wav", NULL, 0},
	{'Z', "Z.wav", NULL, 0}
};

// debug fce
void print_patterns_table() {
    unsigned int i;
    
    for (i = 0; i < sizeof(patterns)/sizeof(SAMPLE); i++) {
        printf("%c, %s, %p, %d\n", patterns[i].letter, patterns[i].filename, patterns[i].dataptr, patterns[i].size);
    }
}

// nahraje audio vzorek do pameti
int load_pattern(SAMPLE *s) {
    SF_INFO  info;
    SNDFILE *fd;
    
    memset(&info, 0, sizeof(info));
    
    fd = sf_open(s->filename, SFM_READ, &info);
    if (!fd) {
        printf("Cannot open sound file %s.\n", s->filename);
        return -1;
    }
    
    s->dataptr = (short*)malloc(info.frames * 2);
    if (!s->dataptr) {
        printf("Cannot allocate memory.\n");
        return -1;
    }
    s->size = info.frames * 2;
    
    sf_readf_short(fd, s->dataptr, info.frames);
    sf_close(fd);
    
    return 1;
}

// nahraje audio vzorky abecedy do pameti
int load_patterns(const char *dir) {
	unsigned int i = 0;
	
	chdir(dir);
	
	for (i = 0; i < sizeof(patterns)/sizeof(SAMPLE); i++) {
        if (!load_pattern(&patterns[i])) {
            return -1;
        }
	}
	
	return 1;
}

// uvolni vzorky abecedy
int free_patterns() {
	unsigned int i = 0;
	
	for (i = 0; i < sizeof(patterns)/sizeof(SAMPLE); i++) {
		free(patterns[i].dataptr);
		patterns[i].size = 0;
	}
	
	return 1;
}

/*
 * hleda vzorek B ve vzorku A
 * pri shode zapise pozici do pole matches
 * (B muze byt v A vicekrat)
 * pokud se neco naslo vraci funkce 1, jinak 0
 */
int compare_sample(const SAMPLE *a, const SAMPLE *b, MATCH *matches) {
	unsigned long i, j;
	unsigned short int x, y;
	float errrate = 0.0f;
	long  umatch  = 0U;
	
    long halfa = (long)(a->size/2);
    long halfb = (long)(b->size/2);
    
    unsigned int matched = 0;
    
    for (i = 0; i < halfa-halfb; i += 1) {
    	short t = 10; // rychlost: testujeme hlavicku vzorku 10b, tolerance 2 chyby
        for (j = 0; j < halfb; j += 6) {
            x = (unsigned short int)a->dataptr[i+j];
            y = (unsigned short int)b->dataptr[j];
            if (x != y) {
            	// chyba v hlavicce, tolerance 2 chyby, jinak ven
            	if (t > 0 && umatch > 2) break;
                umatch++;                
            }
            if (t > 0) t--;
        }
        // chyba v hlavicce?
        if (t > 0) {
        	umatch = 0;
        	continue;
		}
        
        // chybovost srovnani, bereme pod 10%
        errrate = (float)umatch / (float)halfb * 100.0f;

        if (errrate < 10.0f) {
            //printf("Letter: %c, Err rate: %.2f, Position: %d\n", b->letter, errrate, i);
            matches[matched].letter   = b->letter;
            matches[matched].position = i;
            matched++;
        }
        
        umatch = 0;
    }
	
    if (matched) {
        return 1; // neco jsme nasli
    }
	
	return 0;
}

// porovnovaci fce pro quick sort
int compare_match(const MATCH *a, const MATCH *b) {
    if (a->position == b->position)
        return 0;
    if (a->position > b->position)
        return 1;
    else
        return -1;
}


int main (int argc, char **argv) {
	SAMPLE m; // tady bude audio vzorek obsahujici 5 pismen
    
	if (argc < 2) {
		printf("Use: %s sndfile\n", argv[0]);
		return 0;
	}
	
	if (strlen(argv[1]) > 1024) {
		printf("Error: Filename too long.\n");
		return -1;
	}

#if DEBUG    
    clock_t start, end;
    double elapsed;
    start = clock();
#endif
    
    m.letter   = '\0';
    m.filename = argv[1];
    m.dataptr  = NULL;
    m.size     = 0; 
    
    load_pattern(&m); //nahraj vzorek
    load_patterns("abeceda/"); // nahraj abecedu
    
    MATCH temp[5];
    MATCH matches[5];
    memset(temp, 0, sizeof(MATCH)*5);
    memset(matches, 0, sizeof(MATCH)*5);
    
    int count = 4; // 5 pismen
    while(count >= 0) {
        int i, j, e=0, tested=0;
        // prochazime abecedu a srovnavame jednotlive vzorky
        // se vstupnim
        for (i = 0; i < sizeof(patterns)/sizeof(SAMPLE); i++) {
        	// jednou testovany vzorek preskocime
            tested = 0;
            for (j = 0; j < 5; j++) {
                if (matches[j].letter == patterns[i].letter)
                    tested = 1;
            }
            // porovnej vzorek pismena se vstupnim
            // najdes li, zapis pismena a pozice do pole
            if (!tested && compare_sample(&m, &patterns[i], temp)) {
				for (j = 0; j < 5; j++) {
					if (temp[j].letter > 0) {
						matches[count].letter   = temp[j].letter;
						matches[count].position = temp[j].position;
						count--;
						e = 1;
					}
				}
				memset(temp, 0, sizeof(MATCH)*5);
            }
        }
        // nic jsme nenasli (chybovost asi 2 vzorky s 50)
        if (e == 0) {
        	printf("No match.\n");
        	return -1;
		}
    }
    
    // setridime podle pozic, at je vystup ve spravnem poradi
    qsort(matches, 5, sizeof(MATCH), compare_match);
    
    // tisk
    int i;
    for (i = 0; i < 5; i++) {
        printf("%c", matches[i].letter);
    }
    printf("\n");

#if DEBUG    
    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %.2f\n", elapsed);
#endif

	// uvolnime zdroje    
    free(m.dataptr);
    free_patterns();
	
	return 0;
}
