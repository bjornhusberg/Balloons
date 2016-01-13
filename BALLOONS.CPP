/*              BALLOONS  v1.1
		Ett specialarbete i C/C++ programmering
				(c) Bj�rn Husberg 1997
*/

#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

//programkroppar:
  int intro      (void);    //Titelsidan m. plasmaeffekten
  int balloons   (void);    //Sj�lva spelet
  int gameover   (void);    //Slutsidan

//funktioner:
  int w_digits   (int,int,int,int,int);
  int ppixel     (int,int,int);
  int rita_e     (int,int,int);
  int rita_b     (int,int,int);
  int sidbyte    (unsigned int,unsigned int);
  int setpal     (unsigned char,unsigned char,unsigned char,unsigned char);
  int getpal     (unsigned char);
  int clear_scr  (unsigned int);
  int graf_init  (int);
  int bld        (FILE *fnamn,int,int,int);
  int memory_init(void);
  int timer      (int);

//Bild-variabler:
unsigned char ballong  [234]; //minnesplats f�r ballongsprite
unsigned char explosion[234]; //minnesplats f�r exploderande ballongsprite
unsigned char siffror  [420]; //minnesplats f�r siffror

//variabler:
unsigned char pal_r,pal_g,pal_b;  //palettf�rger
unsigned int i;                   //allm�n r�knare
int error = 0;                    //felv�rde
int poang;                        //po�ngr�knare

//Pekare till minnesadresser:
unsigned int virt_scr;  //pekare till virt_scr-segmentet
unsigned int bld_scr;   //pekare till bld_scr-segmentet
unsigned int real_scr=0xa000; //pekare till verkliga video-segmentet

//Konstanter
const TEXT = 3;     //Konstant f�r textl�ge
const MCGA = 19;   //Konstant f�r grafikl�ge

//Filhanterare
FILE *logo, *bsprite, *spelbld, *digits, *slut;
//---------------------------------------------------------------------------




/*                               main()
main() �r en huvudfunktion. Den s�tter samman alla olika delar till en
funktionell enhet. F�rst f�rbereder den filkontrollerarna och �ppnar
de filer som kommer att anv�ndas. Sedan laddar den in sprites f�r spelet
och siffrorna till po�ngdisplayen.
Minne reserveras och grafikkortet st�lls in. Om n�got misslyckas hoppar
programmet till error_exit och avslutar.

Huvudloopen inleds och intro() startas. Sedan k�rs balloons() f�rutsatt
att inte ESCAPE-tangenten har blivit tryckt.
Om balloons() ger returv�rdet -1 har det blivit "game over" och funktionen
game_over() k�rs, varefter huvudloopen g�r ett varv till om fortfarande
inte ESCAPE-tangenten har blivit tryckt.

Vid avslutandet st�lls grafikkortet �ter i textl�ge, slutmeddelande och
eventuella felmeddelanden skrivs ut. Minne och filhanterare frig�rs
och programmet avslutas.*/
int main (void)
 {
 char ch=0;
 randomize();  //nollst�ller slumpgeneratorn

 //fopen() �ppnar de filer som programmet kommer att anv�nda
 if((bsprite = fopen("balloons.000","rb"))== NULL
 || (digits  = fopen("balloons.001","rb"))== NULL
 || (logo    = fopen("balloons.002","rb"))== NULL
 || (spelbld = fopen("balloons.003","rb"))== NULL
 || (slut    = fopen("balloons.004","rb"))== NULL)
 {error=2;goto error_exit;} //Om fel uppstod

 //Laddar bilder f�r ballongen, exploderande ballongen & siffrorna.
 for (i=0;i<234;i++)ballong[i]   = fgetc(bsprite);
 for (i=0;i<234;i++)explosion[i] = fgetc(bsprite);
 for (i=0;i<420;i++)siffror[i]   = fgetc(digits);

 //Reserverar plats i minnet f�r virt_scr och bld_scr
 if (memory_init()!=0)
 {error=1;goto error_exit;} //Om fel uppstod

 //St�ller grafikkortet i MCGA- l�ge
 if (graf_init(MCGA)!=0)
 {error=3;goto error_exit;} //Om fel uppstod

 //Huvudloopen g�r runt s� l�nge man inte trycker ESC vid
 //intro-sidan.   (ESC har 27 som ascii-kod)
 //Om intro() ger fel slutv�rde �r n�got galet; g�r till error_exit
 //om balloons() ger -1 som slutv�rde skall gameover() startas
 while(ch!=27)
 {
 if (intro()!=0)goto error_exit;
 ch=getch();
 if (ch!=27)if (balloons()==-1)gameover();
 }

 error_exit:
 graf_init(TEXT);//St�ller grafikkortet i text-l�ge
 textattr(78);  //Gul text mot r�d bakgrund
 cprintf("                     BALLOONS v1.1   (c) Bj�rn Husberg 1997                     ");
 textattr(7);   //Gr� text mot svart bakgrund

  //Felmeddelanden ges om fel har uppst�tt (error �r inte l�ngre 0)
  if (error!=0){
  if (error==1)cprintf("\nFEL: F�r lite ledigt minne");
  if (error==2)cprintf("FEL: Kunde inte �ppna data-fil");
  if (error==3)cprintf("FEL: Kunde inte initiera grafikkortet");
  if (error==4)cprintf("FEL: En av data-filerna �r felaktig");
  if (error!=0)cprintf(" ... avbryter!");}

 //Allt minne som programmet anv�nt frias:
 freemem(virt_scr);
 freemem(bld_scr);
 //och alla filhanterare st�ngs:
 fclose(logo);
 fclose(bsprite);
 fclose(spelbld);
 fclose(digits);
 fclose(slut);
 //Slutligen tas alla inkommande tangenttryckningar om hand:
  while(kbhit()!=0)getch();
 //och programmet l�mnar �ver kontrollen till operativsystemet
return 0;
}
//---------------------------------------------------------------------------




/*                 intro
Den f�rsta delen av intro() t�mmer videominnet, fyller bld_scr -segmentet
med titelbilden och st�ller paletten f�r plasma effekten (vitt-bl�tt-vitt)
Sedan skapas sinuskurvan, p� vilken plasmaeffekten grundas.

I huvudloopen skapas f�rst plasmabilden i virt_scr. Sedan l�ggs den �vre
delen av bilden i BLD_SCR utanp�, med bakgrunden genomskinlig.
Slutligen l�ggs den undre delen p�, och d�r bilden inte �r bakgrund
inverteras plasmabilden.*/
int intro (void)
 {
 //T�mmer videominnet
 clear_scr(real_scr);
 //ritar startbilden i bld_scr -segmentet.
 if (bld(logo,0,0,10)!=0)return 1; //om fel: avslutar med felv�rde

 for (i=1;i<130;i++)   //Efterssom bilden var f�r m�rk, var jag tvungen
 {                   //att �ka den r�da f�rgtonen i hela bildpaletten:
 getpal(i);  //L�ser en f�rg
 setpal(i,pal_r+10,pal_g,pal_b); //�kar r�dtonen hos den f�rgen
 }

 //St�ller paletten f�r plasma-effekten:
 for (i=130;i<193;i++)
  setpal(i,(193-i),(193-i),63); //fr�n vitt till ljusbl�tt
 for (i=193;i<256;i++)
  setpal(i,(i-193),(i-193),63); //fr�n ljusbl�tt till vitt

 unsigned char t1=100,t2=100,t3=255,t4=100;
 char sintable[512];

 //g�r en dubbel sinuskurva
 //och lagrar den i sintable[]
 for (i=0;i<512;i++)
  sintable[i]=sin(0.02464*i)*31;//sin(i/255*Pi*2)

 while(kbhit()==0) //Huvudloop
  {
  t3=t3-2;
  t4=0;
  for (int y=0;y<200;y++)
	{
	t4++;                        //Utf�r den algoritm
	t1=sintable[y+t3];         //som ger plasmans utseende
	t2=sintable[t4];
	for (int x=0;x<320;x++)
	 {
	 //Ritar punkterna:
	 ppixel(x,y,sintable[t1+t2]+sintable[y+t2]+193);
	 t1++;
	 t2++;
	 }
	}

  unsigned char c;
  //G�r igenom �vre delen av bilden:
  //(Den med ballongen)
  for (i=0;i<48000;i++)
	{
	//l�ser en byte ur bilden:
	c=peekb(bld_scr,i);
	//Om byten inte �r bakgrund s� ska den ritas i virt_scr:
	if (c>10)pokeb(virt_scr,i,c);
	}
  //G�r igenom nedre delen av bilden:
  //(Den med texten)
  for (i=48640;i<64000;i++)
	{
	//L�ser en byte ur bilden:
	c=peekb(bld_scr,i);
	//Om byten inte �r bakgrund s� ska motsvarande plasmapunkt inverteras.
	if (c!=10)
	 {
	 c=peekb(virt_scr,i);
	 if (c>193)c=c-61;
	 else c=c+61;
	 pokeb(virt_scr,i,c);
	 }
	}
  //Flyttar �ver virt_scr till videominnet
  sidbyte(virt_scr,real_scr);
  //Om ingen tangent blivit tryckt, g�r loopen ett varv till
  }
 return 0;
 }
//---------------------------------------------------------------------------




/*                        balloons()

balloons() �r den programkropp som �r det egentliga spelet.
plan[x-ruta][y-ruta] h�ller reda p� vilka spelplansplatser som inneh�ller
ballonger, och vilka f�rger dessa har.
�vre v�nstra h�rnet har koordinaten plan[0][0].
xballong[], yballong[] och cballong[] h�ller reda p� de fallande
ballongernas placering p� spelplanen.
Funktionen �r n�rmre beskriven i det programfl�desschema som f�ljer med
som bilaga till dokumentationen av specialarbetet.*/
int balloons(void)
 {
 double level=700,k;
 double lvl=0;
 poang=0;
 unsigned char plan[9][12], xballong[3], yballong[3], cballong[3];
 char ch;

 for (int a=0;a<9;a++)for (int aa=0;aa<12;aa++)
	plan[a][aa]=0;   //nollst�ller spelplanen

 clear_scr(real_scr); //t�mmer videominnet
 clear_scr(virt_scr); //t�mmer det virtuella videominnet

 //S�tter paletterna f�r ballongspritesen:
 setpal(1,30,0,0);   //m�rkr�d
 setpal(2,60,0,0);   //r�d
 setpal(3,63,30,30); //ljusr�d
 setpal(4,0,30,0);   //m�rkgr�n
 setpal(5,0,60,0);   //gr�n
 setpal(6,30,63,30); //ljusgr�n
 setpal(7,0,0,30);   //m�rkbl�
 setpal(8,0,0,60);   //bl�
 setpal(9,30,30,63); //ljusbl�
 setpal(10,63,63,63);//vit

 bld(spelbld,0,0,11);  //l�ser in spelplanen i bld_scr
 w_digits(poang,1,1,10,0);  //ritar ut po�ngsiffrorna (0000) i bld_scr
 clock_t start, stop;  //Skapar strukturvariabler f�r tidtagning

 AGAIN:
  if (lvl!=0)    //Om ballongerna "sl�ppts" med SPACE eller ENTER
  {
  level=lvl-(lvl/14-level); //s� �terst�lls hastigheten till normal
  lvl=0;
  }

  //T�mmer tangentbufferten och avslutar om ESCAPE har tryckts
  while(kbhit()!=0)if(getch()==27)return 1;

  for (a=0;a<3;a++)      //Huvudloop:
	{
	xballong[a]=0;           //Nollst�ller xballong[]
	yballong[a]=0;           //Nollst�ller yballong[]
	cballong[a]=random(3)+1;    //Ny slumpm�ssig f�rg
	}
  xballong[0]=4;            //S�tter xballong[0] till mittenplacering
  yballong[0]=10;        //S�tter yballong[0] till mittenplacering

  //Slumpar ut placeringar f�r de andra tv� ballongerna:
  ny1:
	xballong[1]=random(3)+3;
	yballong[1]=random(3)+9;
	//f�rs�k igen om ballong 1 har samma koordinater som ballong 0:
	if (xballong[1]==4&&yballong[1]==10)goto ny1;
  ny2:
	xballong[2]=random(3)+3;
	yballong[2]=random(3)+9;
	//f�rs�k igen om ballong 2 har samma koordinater som ballong 0:
	if (xballong[2]==4&&yballong[2]==10)goto ny2;
	//f�rs�k igen om ballong 2 har samma koordinater som ballong 1:
	if (xballong[2]==xballong[1]&&yballong[2]==yballong[1])goto ny2;

  //Om n�gon av de tre ballongernas plankoordinater �r upptagen s� �r
  //spelet slut:
  for (a=0;a<3;a++)if (plan[xballong[a]][yballong[a]]!=0)return -1;

  start=clock(); //Startar tidtagaren
	LOOP:
	 sidbyte(bld_scr,virt_scr);   //kopierar spelplanen till virt_scr

	 //G�r igenom spelplanen och ritar ut eventuella ballonger
	 for (a=0;a<9;a++)for (int aa=0;aa<12;aa++)
	  if (plan[a][aa]!=0)rita_b(a*14+98,aa*19+6,plan[a][aa]);

	 //Ritar ut de tre stigande ballongerna:
	 for (a=0;a<3;a++)rita_b(xballong[a]*14+98,yballong[a]*19+6,cballong[a]);
	 //Kopierar den f�rdiga sidan till videominnet:
	 sidbyte(virt_scr,real_scr);

	 while (kbhit()!=0)    //Om n�gon tangent blivit tryckt:
	  {
	  ch=getch();           //L�gg ascii-koden f�r tryckningen i ch
  //-----------------------------------------
	  if (ch==27)return 1;  //ESCAPE tangenten avbryter
  //-----------------------------------------
	  if (ch==' '||ch=='\r')if (lvl==0) //Om SPACE eller ENTER har tryckts
		{
		lvl=level;            //lvl minns vad levels v�rde var
		level=level/14; //Hastigheten �kar (�terst�lls n�r programmet kommer
		}            //tillbaka till labeln AGAIN
  //-----------------------------------------
	  if (ch==75)          //v�nster piltangent
		{
		k=0;                //Kontrollsiffran s�tts till noll
		for (a=0;a<3;a++)   //G�r igenom de tre r�rliga ballongerna
		 {
		 if (xballong[a]==0)k=1;  //Om ballongen �r l�ngst till v�nster
		  //eller om nya koordinaten �r upptagen s� s�tts k=1
		  else if (plan[xballong[a]-1][yballong[a]]!=0)k=1;
		 }
		//Om k �r 0 efter kontroll av de tree ballongerna flyttas
		//alla tre ett steg �t v�nster:
		if (k==0)for (a=0;a<3;a++)xballong[a]--;
	  }
  //-----------------------------------------
	 if (ch==77)    //h�ger piltangent
	  {
	  k=0;                //kontrollsiffran s�tts till noll
	  for (a=0;a<3;a++)  //G�r igenom de tre r�rliga ballongerna
		{
		if (xballong[a]==8)k=1;  //Om ballongen �r l�ngst till h�ger
		 //eller om nya koordinaten �r upptagen s� s�tts k=1
		 else if (plan[xballong[a]+1][yballong[a]]!=0)k=1;
		}
	  //Om k �r 0 efter kontroll av de tree ballongerna flyttas
		//alla tre ett steg �t v�nster:
	  if (k==0)for (a=0;a<3;a++)xballong[a]++;
	 }
  //-----------------------------------------
	if (ch==72)    //Upp - piltangent
	 {             //Inneb�r medurs flyttning kring 0-ballongen
	 k=0;          //kontrollsiffran s�tts till noll
	 for (a=1;a<3;a++)//g�r igenom de tv� cirkulationsr�rliga ballongerna:
	  {
	  if (xballong[a]<=xballong[0]&&yballong[a]<yballong[0])//flyttas h�ger
		if (xballong[a]<8)if (plan[xballong[a]+1][yballong[a]]==0)k++;
		  //Om ballongen kunde flyttas �t h�ger s� �kades k med 1
	  if (xballong[a]>xballong[0]&&yballong[a]<=yballong[0])//flyttas ner
		if (yballong[a]<11)if (plan[xballong[a]][yballong[a]+1]==0)k++;
		  //Om ballongen kunde flyttas ner s� �kades k med 1
	  if (xballong[a]>=xballong[0]&&yballong[a]>yballong[0])//flyttas v�nster
		if (xballong[a]>0)if (plan[xballong[a]-1][yballong[a]]==0)k++;
		  //Om ballongen kunde flyttas v�nster s� �kades k med 1
	  if (xballong[a]<xballong[0]&&yballong[a]>=yballong[0])//flyttas upp
		if (yballong[a]>0)if (plan[xballong[a]][yballong[a]-1]==0)k++;
		  //Om ballongen kunde flyttas upp s� �kades k med 1
	  }

	 if (k==2)for (a=1;a<3;a++)//Om k �r 2 s� kunde b�da ballongerna flyttas
	  { //if-satser kollar och utf�r f�rflyttning h�ger,ner,v�nster och upp
	  if (xballong[a]<=xballong[0]&&yballong[a]<yballong[0])xballong[a]++;
		else if (xballong[a]>xballong[0]&&yballong[a]<=yballong[0])yballong[a]++;
		 else if (xballong[a]>=xballong[0]&&yballong[a]>yballong[0])xballong[a]--;
		  else if (xballong[a]<xballong[0]&&yballong[a]>=yballong[0])yballong[a]--;
	  }
	 }
  //-----------------------------------------
	if (ch==80)   //Ner - piltangent
	 {          //Inneb�r moturs f�rflyttning kring 0-ballongen
	 k=0;       //kontrollsiffran nollst�lls
	 for (a=1;a<3;a++)//G�r igenom de tv� cirkulationsr�rliga ballongerna
	  {
	  if (xballong[a]>xballong[0]&&yballong[a]>=yballong[0])//flyttas upp
		if (yballong[a]>0)if (plan[xballong[a]][yballong[a]-1]==0)k++;
		  //Om ballongen kunde flyttas upp s� �kades k med 1
	  if (xballong[a]>=xballong[0]&&yballong[a]<yballong[0])//flyttas v�nster
		if (xballong[a]>0)if (plan[xballong[a]-1][yballong[a]]==0)k++;
			//Om ballongen kunde flyttas �t v�nster s� �kades k med 1
	  if (xballong[a]<xballong[0]&&yballong[a]<=yballong[0])//flyttas ner
		if (yballong[a]<11)if (plan[xballong[a]][yballong[a]+1]==0)k++;
			 //Om ballongen kunde flyttas ner s� �kades k med 1
	  if (xballong[a]<=xballong[0]&&yballong[a]>yballong[0])//flyttas h�ger
		if (xballong[a]<8)if (plan[xballong[a]+1][yballong[a]]==0)k++;
			 //Om ballongen kunde flyttas �t h�ger s� �kades k med 1
	  }

	 if (k==2)for (a=1;a<3;a++)//Om k �r 2 s� kunde b�da ballongerna flyttas
	  {   //if-satser kollar och utf�r f�rflyttning upp,v�nster,ner och h�ger
	  if (xballong[a]>xballong[0]&&yballong[a]>=yballong[0])yballong[a]--;
		else if (xballong[a]>=xballong[0]&&yballong[a]<yballong[0])xballong[a]--;
		 else if (xballong[a]<xballong[0]&&yballong[a]<=yballong[0])yballong[a]++;
		  else if (xballong[a]<=xballong[0]&&yballong[a]>yballong[0])xballong[a]++;
	  }
	 }
//Om det finns fler tangenttryckningar att h�mta s� g�r loopen ett varv till.
	}
  stop=clock();   //Kollar tiden (Klockan stoppas inte egentligen)
  if (stop-start<level)goto LOOP; //Om tidtagningen inte uppn�tt det
	 //v�rde som anges av level (sv�righetsgrad) s� g�r programmet
	 //tillbaka till labeln LOOP
  start=clock();  //Tidtagningen startas p� nytt
  k=0;
  for (a=0;a<3;a++)//G�r igenom de tre flyttbara ballongerna
	if (yballong[a]==0||plan[xballong[a]][yballong[a]-1]!=0)
	 k=1;  //Om n�gon av dem inte kan flyttas upp�t s�tts k = 1

	if (k==0) //Om alla tre ballongerna kan flyttas upp�t
	 {        //s� flyttas de och programmet hoppar sedan till labeln LOOP
	 for (int aa=0;aa<3;aa++)yballong[aa]--;
	 goto LOOP;
	 }
	//De tre flyttbara ballongerna l�ggs �ver till spelplanen
	for (a=0;a<3;a++)plan[xballong[a]][yballong[a]]=cballong[a];

	STANNA_IGEN:
	k=1;
	 while (k==1)  //S� l�nge det finns fler ballonger att flytta
	 {
	 k=0;     //Kontrollsiffran s�tts till 0
	 for (a=0;a<9;a++)for (int aa=1;aa<12;aa++) //G�r igenom spelplanen
	  if (plan[a][aa-1]==0&&plan[a][aa]!=0)  //Om platsen ovanf�r en ballong
		{                  //�r tom s� skall ballongen flyttas upp�t
		plan[a][aa-1]=plan[a][aa];//kopierar ballongens f�rg till platsen ovan
		plan[a][aa]=0;   //och t�mmer den ursprungliga
		k=1;           //samt s�tter kontrollsiffran till 1
		}

	  if (k==1)     //om kontrollsiffran �r 1 s� har ballonger flyttats
		{
		sidbyte(bld_scr,virt_scr); //Ritar ut spelplanen i virt_scr
		for (a=0;a<9;a++)for (int aa=0;aa<12;aa++) //och ritar ut ballongerna:
		 if (plan[a][aa]!=0)rita_b(a*14+98,aa*19+6,plan[a][aa]);
		sidbyte(virt_scr,real_scr); //Flyttar sidan till videominnet
		start=clock();             //Startar tidtagning
		while (stop-start<level)stop=clock();//V�ntar tills det �r dags igen
		}
	  }//Om k �r 1 s� kan det finnas fler ballonger att flytta och programmet
		//g�r igenom kontrollen en g�ng till (while(k==1)

	 //Skapar en kopia av spelplanen:
	 int kopia[9][12];
	 for (a=0;a<9;a++)for (int aa=0;aa<12;aa++)
	  kopia[a][aa]=plan[a][aa];

	 k=0;      //S�tter kontrollsiffran till 0
	 for (a=0;a<9;a++)for (int aa=0;aa<12;aa++)//G�r igenom planen
	  {
	  if (kopia[a][aa]!=0) //Om det finns en ballong p� platsen:
		{
		if (aa<10)
		 if (kopia[a][aa]==kopia[a][aa+1]&&kopia[a][aa+1]==kopia[a][aa+2])
		  {  //Om tre lodr�ta ballonger �r av samma f�rg:
		  k=1;      //kontrollsiffran s�tts till 1
		  plan[a][aa]=plan[a][aa+1]=plan[a][aa+2]=0;//Nollor skrivs till planen
		  }

		if (a<7&&aa<10)
		 if (kopia[a][aa]==kopia[a+1][aa+1]&&kopia[a+1][aa+1]==kopia[a+2][aa+2])
		  {  //Om tre ballonger snett ned�t-fram�t �r av samma f�rg:
		  k=1;      //kontrollsiffran s�tts till 1 och nollor skrivs till planen
		  plan[a][aa]=plan[a+1][aa+1]=plan[a+2][aa+2]=0;
		  }

		if (a<7)
		 if (kopia[a][aa]==kopia[a+1][aa]&&kopia[a+1][aa]==kopia[a+2][aa])
		  {   //Om tre horisontella ballonger �r av samma f�rg:
		  k=1;     //kontrollsiffran s�tts till 1 och nollor skrivs till planen
		  plan[a][aa]=plan[a+1][aa]=plan[a+2][aa]=0;
		  }

		if (a>1&&aa<10)
		 if (kopia[a][aa]==kopia[a-1][aa+1]&&kopia[a-1][aa+1]==kopia[a-2][aa+2])
		  {   //Om tre ballonger snett ned�t-bak�t �r av samma f�rg:
		  k=1;    //kontrollsiffran s�tts till 1 och nollor skrivs till planen
		  plan[a][aa]=plan[a-1][aa+1]=plan[a-2][aa+2]=0;
		  }
		 }
		}
	  sidbyte(bld_scr,virt_scr); //Ritar ut spelplanen i virt_scr
	  for (a=0;a<9;a++)for (int aa=0;aa<12;aa++)//G�r igenom spelplanen
		{
		if (plan[a][aa]==0&&kopia[a][aa]!=0)//Om en plats p� planen �r 0, men
		 {//samma plats inte �r noll i kopian, har en ballong exploderat
		 if (level>1)level--; //Sv�righetsgraden �kar (Tiden minskar!)
		 rita_e(a*14+98,aa*19+6,kopia[a][aa]);//Ritar ut exploderande ballong
		 poang++;   //�kar po�ngen
		 }
		//Annars skall en vanlig ballong ritas ut:
		else if(plan[a][aa]!=0)rita_b(a*14+98,aa*19+6,plan[a][aa]);
		}

	  sidbyte(virt_scr,real_scr); //Flyttar sidan till videominnet
	  start=clock();              //Startar ny tidtagning
	  while (stop-start<100)stop=clock();//V�ntar en kortare stund
	  w_digits(poang,1,1,10,0);   //Ritar ut nya po�ngen i bld_scr
	  sidbyte(bld_scr,virt_scr);//skriver spelplanen m. nya po�ng till virt_scr

	  for (a=0;a<9;a++)for (int aa=0;aa<12;aa++) //g�r igenom spelplanen
		if(plan[a][aa]!=0)rita_b(a*14+98,aa*19+6,plan[a][aa]);//ritar ballongerna

	  sidbyte(virt_scr,real_scr);   //Flyttar sidan till videominnet
	  start=clock();                //Startar klockan
	  while (stop-start<level)stop=clock();   //V�ntar en stund
	 //Om inga ballonger exploderat, finns inga fler att flytta och det �r
	 //dags f�r tre stycken nya, r�rliga ballonger:
	 if (k==0)goto AGAIN;
	//Annars hoppar programmet till STANNA_IGEN f�r att kontrollera om det
	//finns fler ballonger som skall stiga:
	goto STANNA_IGEN;
 }
//---------------------------------------------------------------------------




/*                    gameover()

gameover() �r den programkropp som utf�r blinkeffekten och ritar ut
slutsidan.
F�rst �kas alla f�rgerna suggestivt tills de n�tt vitt och sedan minskas
f�rgerna tillsammans ned till svart. D�refter l�ses slut-bilden in
och vissas tills en tangent trycks.*/
int gameover (void)
 {
 delay(1000);    //paus
 //Fr�n nuvarande f�rg mot vitt:
 for (i=0;i<63;i++)
  for (int a=0;a<255;a++)   //G�r igenom alla 255 f�rgerna 63 ggr
	{
	getpal(a);              //l�ser paletten
	if (pal_r<63)pal_r++;   //�kar r�dtonen mot 63
	if (pal_g<63)pal_g++;   //�kar gr�ntonen mot 63
	if (pal_b<63)pal_b++;   //�kar bl�tonen mot 63
	setpal(a,pal_r,pal_g,pal_b);  //s�tter ny palett
	}
 //Fr�n vitt till svart:
 for (i=63;i>0;i--)
  for (int a=0;a<255;a++)  //G�r igenom alla f�rgerna 63 ggr
	setpal(a,i,i,i);        //minskar enligt i:s minskning med ett/varv

 clear_scr(real_scr);    //T�mmer videominnet
 bld (slut,0,0,0);       //och l�ser in slutbilden i bld_scr
 w_digits(poang,152,76,209,-1);  //Skriver ut po�ngen p� koordinat (152,76)
										  //i f�rgen 209, med genomskinlig bakgrund.
 sidbyte (bld_scr,real_scr);  //Flyttar den f�rdiga bilden till videominnet

 while(kbhit()!=0)getch();   //V�ntar p� tangenttryckning
 getch();
return 1;
}
//---------------------------------------------------------------------------




/*                 rita_b(x-koordinat, y-koordinat, f�rg)

rita_b() ritar ut en ballong av f�rgen rc, med �vre v�nstra h�rnet i
punkten (rx,ry). Ballongen l�ses in i ballong[] vid programmets b�rjan.
(0=bakgrund, 1=m�rk, 2=mellan, 3=ljus)*/
int rita_b(int rx,int ry, int rc)
 {
 rc=rc*3-3;
 int ii=0;
 int c;
 for (int y=0;y<18;y++)
  for (int x=0;x<13;x++)
	if ((c=ballong[ii++])!=0)  //om punkten inte �r bakgrund:
	 if ((ry+y)<200)pokeb(virt_scr,320*(y+ry)+x+rx,c+rc);
return 0;
}
//---------------------------------------------------------------------------




/*                     rita_e(x-koordinat, y-koordinat,rc)

rita_e fungerar precis som rita_b (se ovan) men med den enkla skillnad att
den, i st�llet f�r den vanliga ballongen, ritar ut en exploderande ballong,
som finns lagrad i explosion[] sedan programmets b�rjan.*/
int rita_e(int rx,int ry, int rc)
 {
 rc=rc*3-3;
 int ii=0;
 int c;
 for (int y=0;y<18;y++)
  for (int x=0;x<13;x++)
	if ((c=explosion[ii++])!=0)    //Om punkten inte �r bakgrund:
	 if ((ry+y)<200)pokeb(virt_scr,320*(y+ry)+x+rx,c+rc);
 return 0;
 }
//---------------------------------------------------------------------------




/*  w_digits(tal,x-koordinat, y-koordinat, f�rgrundsf�rg, bakgrundsf�rg)

w_digits() �r den funktion som ritar ut de fyra po�ngsiffrorna uppe i
v�nstra h�rnet under spelets g�ng och p� mitten av slut-sidan.
Po�ngtalet, som anges i nr, minskas f�rst med 10000 s� l�nge som det �r
st�rre �n 9999, s� att det alltid �r h�gst fyrsiffrigt.
Sedan delas talet upp i ental, tiotal, hundratal och tusental. Slutligen
ritas siffrorna ut i minnesomr�det bld_scr, med en punkts mellanrum och
med f�rsta siffrans �vre v�nstra h�rn i punkten (wx,wy). Cr anger den
�nskade sifferf�rgen och k anger bakgrundsf�rgen (k=-1 ger genomskinlig
bakgrund).
Siffrorna �r 7*6 punkter stora och �r lagrade som 1:or och 0:or i siffror[]
sedan programmets b�rjan.*/
int w_digits(int nr, int wx,int wy, int cr, int k)
 {
 while (nr>9999)nr=nr-10000;
 int rt[4];
 rt[0]=nr/1000;    //tusental
 nr=nr-1000*(nr/1000);
 rt[1]=nr/100;     //hundratal
 nr=nr-100*(nr/100);
 rt[2]=nr/10;      //tiotal
 nr=nr-10*(nr/10);
 rt[3]=nr;         //ental
 for (int ii=0;ii<4;ii++) //loopar igenom de fyra siffrorna
  {
  int c=rt[ii]*42;    //den byte d�r sifferbilden b�rjar
  for (int aa=0;aa<6;aa++)  //y-led
  for (int a=0;a<7;a++)     //x-led
	{
	if (siffror[c]==0)       //om punkten �r f�rgrund:
	 pokeb(bld_scr,ii*8+wx+a+((wy+aa)*320),cr+siffror[c]);
	else if(k!=-1)  //om punkten �r bakgrund och k inte s�ger att punkten
	 pokeb(bld_scr,ii*8+wx+a+((wy+aa)*320),k);   //ska vara bakgrund:
	c++;
	}
  }
 return 0;
 }
//---------------------------------------------------------------------------




/*                            memory_init()

memory_init reserverar dataminne m.h.a. kommandot 'allocmem'.
Till allocmem skickar man �nskat minnesutrymme i paragrafer
(1 paragraf=16 bytes) och namnet p� den pekare som man vill att allocmem
skall returnera adressen i.

Jag beh�vde i det h�r fallet tv� stycken 'virtuella videominnen'.
(d.v.s. tv� minnesareor som �r lika stora som en videosida)
De �nskade storlekarna p� 64000 bytes (en videosida) �r lika med 4000
paragrafer, som �r de angedda v�rdena i funktionen.

Sidan bld_scr anv�nds senare i programmet till att ladda de bilder som
finns i .bld -format p� disken.
Sidan virt_scr anv�nds f�r hopklippning och redigering av grafik
f�r senare kopiering till det verkliga videominnet.*/
int memory_init(void)
 {
 if (allocmem(4000,&virt_scr)!=-1){return 6;}
 if (allocmem(4000,&bld_scr)!=-1){freemem(virt_scr);return 6;}
 clear_scr(virt_scr);
 clear_scr(bld_scr);
 return 0;
 }
//---------------------------------------------------------------------------




/*                    graf_init(�nskat grafikl�ge)

Eftersom C++'s grafikbibliotek saknar funktioner f�r videol�get MCGA
(Som st�ds av alla VGA - grafikkort) var jag tvungen att anv�nda en
s.k. Interruptrutin. (i det h�r fallet en av DOS egna)

Med hj�lp av kommandot Int86 och strukturvariabeln REGS skickar jag numret
f�r det �nskade grafikl�get till grafikkortet.
(3 f�r Textl�ge och 19 f�r MCGA l�ge);

N�r det �r gjort g�r jag en kontrollfunktion m.h.a. samma interrupt.
Interrupten skickar d� numret p� det nuvarande grafikl�get i 'regs.l.al'.
Om det returnerade v�rdet �r samma siffra som gi_gmode s� lyckades
initieringen.*/
int graf_init(int gi_gmode)
 {
 union REGS regs;
 regs.h.ah = 0;
 regs.h.al = gi_gmode;    // v�rdet f�r inst�llning av grafikl�get
 int86(16, &regs, &regs); // utf�r interrupten

 regs.h.ah = 15;          // v�rdet f�r kontrollfunktion
 int86(16, &regs,&regs);  // utf�r interrupten
 if (regs.h.al != gi_gmode)  //verifierar kontrollv�rdet
  return 1;
 return 0;
 }
//---------------------------------------------------------------------------




/*  bld(filkontrollerare, x-koordinat, ykoordinat, f�rgnummer)

bld() �r en funktion som ritar ut en bild av mitt eget bildformat
.bld (=bild). Till att b�rja med nollst�lls filkontrolleraren med
rewind(). Sedan l�ses f�rsta v�rdet i bild-filen, som m�ste vara 66
f�r att indikera att det �r en .bld -fil.
Sedan l�ses tre v�rden som talar om bildens bredd (xx) enligt
1=hundratal 2=tiotal och 3=ental. Samma procedur sker f�r bildens
h�jd (yy). Antalet f�rger l�ses in i variabeln frg. (<256)
Sedan s�tts >frg< stycken f�rger i paletten till de rgb - v�rden som
�r lagrade i filen. (se setpal())
Slutligen l�ses >xx< * >yy< punkter fr�n filen och ritas i
minnesadressen bld_scr. bld_scr kan sedan kopieras till minnesadressen
virt_scr eller videominnet med hj�lp av sidbyte()*/
int bld(FILE *fnamn,int bx,int by,int bc)
 {
 rewind(fnamn);  //nollst�ller filkontrolleraren
 if (fgetc(fnamn)!=66){error=4;return 1;}    //kollar filformtet
 int xx=fgetc(fnamn)*100+fgetc(fnamn)*10+fgetc(fnamn); //l�ser bildens bredd
 int yy=fgetc(fnamn)*100+fgetc(fnamn)*10+fgetc(fnamn); //l�ser bildens h�jd

 int frg=fgetc(fnamn);     //l�ser antalet f�rger och f�rgernas toner
 for (i=0;i<=frg;i++)setpal(i+bc,fgetc(fnamn),fgetc(fnamn),fgetc(fnamn));
 //F�ljande loop ritar bilden i bld_scr:
 for (int y=0;y<yy;y++)
  for (int x=0;x<xx;x++)
	{
	pokeb(bld_scr,320*(y+by)+(x+bx),fgetc(fnamn)+bc);
	}

 return 0;
 }
//---------------------------------------------------------------------------




/*             ppixel(x-koordinat, y-koordinat, f�rg)

ppixel() fungerar som C++'s putpixel(), men �r mycket snabbare.
Funktionen skriver direkt till dataminnet p� adressen virt_scr som,
n�r sidan �r f�rdigritad, kopieras med sidbyte() till det verkliga
videominnet, varvid punkten blir synlig p� sk�rmen.*/
int ppixel(int ppx, int ppy, int ppc)
 {
 pokeb(virt_scr,320*ppy+ppx,ppc);
 return 0;
 }
//---------------------------------------------------------------------------




/*                    clear_scr(minnesadress(segment))

clear_scr() skriver 64000 nollor till den angedda minnesadressen.
Funktionen anv�nds till att sudda ut bilden som programmet lagrat
p� den aktuella minnesadressen*/
int clear_scr(unsigned int c_scr)
 {
 for (i=0;i<64000;i=i+2)
  poke(c_scr,i,0);
 return 0;
 }
//---------------------------------------------------------------------------




/*                    setpal(f�rgnummer, r�dton, gr�nton, bl�ton)

setpal() �r en snabbare funktion �n C++'s egen setrgbpalette.
Den fungerar dock likadant. Funktionen anropas med ett
f�rgnummer. Ett r(�d), ett g(r�n) och ett b(l�) -v�rde.
F�rgnummret skrivs till grafikkortet �ver port 3c8 (hexadecimalt)
och f�ljs av r,g och b - v�rdena �ver port 3c9 (hexadecimalt)
Resultatet �r att f�rgnummret f�r den f�rgton som bildas genom
blandning av rgb - v�rdena.*/
int setpal(unsigned char spnr, unsigned char spr, unsigned char spg, unsigned char spb)
 {
 outportb(0x3c8,spnr);
 outportb(0x3c9,spr);
 outportb(0x3c9,spg);
 outportb(0x3c9,spb);
 return 0;
 }
//---------------------------------------------------------------------------




/*                         getpal(f�rgnummer)

getpal() fungerar motsatt setpal(). Genom att f�rst skriva numret
p� den �nskade f�rgen till port 3c7 (hexadecimalt) kan man l�sa r(�d)
g(r�n) och b(l�) - v�rdena fr�n port 3c9 (hexadecimalt).
Dessa tre v�rden l�ggs i variablerna pal_r, pal_g och pal_b som
sedan kan l�sas fr�n n�gon annan funktion, vilken som helst.*/
int getpal(unsigned char spnr)
 {
 outportb(0x3c7,spnr);
 pal_r=inportb(0x3c9);
 pal_g=inportb(0x3c9);
 pal_b=inportb(0x3c9);
 return 0;
 }
//---------------------------------------------------------------------------




/*                 sidbyte(k�lladress, m�ladress)

Sidbyte() kopierar 64000 bytes ifr�n minnesadressen som
anges i src_scr till minnesadressen i dest_scr.
Anv�nds till exempel f�r att kopiera den f�rdigredigerade
bilden fr�n minnesarean virt_scr till videominnet s� att
bilden blir synlig p� sk�rmen.*/
int sidbyte(unsigned int src_scr, unsigned int dest_scr)
 {
 movedata(src_scr,0,dest_scr,0,64000);
 return 0;
 }
