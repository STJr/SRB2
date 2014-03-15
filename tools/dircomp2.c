//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by Boris Pereira.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------

#include<stdlib.h>
#include<stdio.h>
#include<conio.h>
#include<dir.h>
#include<string.h>
#include<stdarg.h>
#include<unistd.h>
#include<dpmi.h>
#include<sys/stat.h>
#include<utime.h>
#include<time.h>
#include<process.h>
#include"keys.h"


#define true    1
#define false   0

#define MAXLINE      32768
#define MAXLENLINE    4096
#define SCREENWIDTH     80

typedef struct {
    int lines;
    char *line[MAXLINE];
} textfile_t;

char       tempfilename[255],editor[128]="c:\\program files\\ultraedit\\uedit32.exe";
int        sameline=3,tab1=8,tab2=8;
int        screenline=25;
textfile_t f1,f2;
int        nonewfile = false;
int        compspace = false;

unsigned short colornormal = (   BLACK<<4) + LIGHTGRAY;
unsigned short colortitle  = (   BLACK<<4) +     WHITE;
unsigned short colortext   = (   BLACK<<4) +  DARKGRAY;
unsigned short colordiff   = (   BLACK<<4) +  LIGHTRED;
unsigned short colorkeys   = (   BLACK<<4) + LIGHTCYAN;


// compare directorys
void CompareDir(char *dir1,char *dir2);

// compare directorys this one check only for missing file
void CompareDir2(char *dir1,char *dir2);

// compare files
void CompareFile(char *filename1,char *filename2);

// copy a file
void fcopy(char *filename1,char *filename2);

// recursive delete a dir
void dirunlink(char *dirname);

// recursive copy a dir
void copydir(char *dirname1,char *dirname2);

void Quit(char *f,...)
{
    va_list     argptr;

    // put message to stderr
    va_start (argptr,f);
    fprintf  (stderr, "Error: ");
    vfprintf (stderr,f,argptr);
    va_end   (argptr);

    fflush( stderr );
    exit(-1);
}

int CheckParm(int argc, char *argv[], char *param )
{
    int i;
    for(i=1;i<argc;i++)
    {
        if(!stricmp(param,argv[i]))
        {
            return i;
        }
    }

    return 0;
}

int main(int argc,char *argv[])
{
    struct ffblk dir;
    char   *tmp;
    int    i;

    printf("Directory Comparator 1.3 (c) Copyright Boris Pereira\n\n");

    if(argc<2)
       Quit("Missing parameter\n"
            "Syntax: filecomp <dirname1> <dirname2> [<options>]\n\n"
            " Options\n"
            " -------\n"
            "   -t1 n : change tab size for directory 1  (default 8)\n"
            "   -t2 n : change tab size for directory 2  (default 8)\n"
            "   -l  n : use n same line to resynchronize (default 3)\n"
            "   -e <editor> : use spesified editor\n"
            "   -nonew: don't prompt to copy/delete file\n"
            "   -w compress space and tab");

    if(findfirst(argv[1],&dir,FA_DIREC))
       Quit("Error can't find %s\n",argv[1]);

    if(findfirst(argv[2],&dir,FA_DIREC))
       Quit("Error can't find %s\n",argv[2]);

    if(argc>2)
    {
       i=CheckParm(argc,argv,"-t1");
       if(i) tab1=atoi(argv[i+1]);
       i=CheckParm(argc,argv,"-t2");
       if(i) tab2=atoi(argv[i+1]);
       i=CheckParm(argc,argv,"-l");
       if(i) sameline=atoi(argv[i+1]);
       i=CheckParm(argc,argv,"-e");
       if(i) strcpy(editor,argv[i+1]);
       nonewfile=CheckParm(argc,argv,"-nonew");
       compspace=CheckParm(argc,argv,"-w");
    }

    _set_screen_lines(50);
    screenline=50;
    // desable blink color so we can use darkcolor for background
    //outportb(0x3b8,5);
    intensevideo();

    tmp=getenv("temp");
    if(!tmp)
        tmp=getenv("TEMP");
    if(!tmp)
        tmp=getenv("tmp");
    if(!tmp)
        tmp=getenv("TMP");
    if(tmp)
    {
        strcpy(tempfilename,tmp);
        if(tempfilename[strlen(tempfilename)]!='\\')
            strcat(tempfilename,"\\");
    }
    else
        tempfilename[0]='\0';
    strcat(tempfilename,"dircmp$$.tmp");

    CompareDir(argv[1],argv[2]);

    if(!nonewfile)
        // just for file in the directory 2 and not in the 1
        CompareDir2(argv[1],argv[2]);

    return 0;
}

char *fileextension(char *filename)
{
    static char nullextension=0;
    int i;
    i = strlen(filename)-1;
    while(i>0 && filename[i]!='.') i--;
    if( filename[i]=='.' )
        return filename+i+1;
    // else
    return &nullextension;
}

void CompareDir(char *dirname1,char *dirname2)
{
    struct ffblk dir;
    char   filter[20]="\\*.*";
    char   tempstr[255];
    int    finish;

    strcpy(tempstr,dirname1);
    strcat(tempstr,filter);

    printf("Comparing %s and %s\n",dirname2,dirname1);

    // enter in directorys first
    finish=findfirst(tempstr,&dir,FA_DIREC);
    while(!finish)
    {
        char tempstr2[255],tempstr3[255];

        strcpy(tempstr2,dirname1);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        strcpy(tempstr3,dirname2);
        strcat(tempstr3,"\\");
        strcat(tempstr3,dir.ff_name);


        if(dir.ff_attrib & FA_DIREC)
        {
            if(dir.ff_name[0]!='.' && strcmp(dir.ff_name,"CVS")!=0)
            {
                int check,choice;
                check=access(tempstr3,F_OK);
                if( check )
                {
                    printf("\nDirectory missing : %s\n"
                           "Delete %s ? (Y/N)\n",tempstr3,tempstr2);
                    do {
                        choice=getch();
                    } while(choice!='y' && choice!='n'
                         && choice!='Y' && choice!='N');

                    if( choice=='y' || choice=='Y' )
                        dirunlink(tempstr2);
                }
                else
                    CompareDir(tempstr2,tempstr3);
            }
        }
        finish=findnext(&dir);
    }

    finish=findfirst(tempstr,&dir,0);
    while(!finish)
    {
        char tempstr2[255],tempstr3[255];

        strcpy(tempstr2,dirname1);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        strcpy(tempstr3,dirname2);
        strcat(tempstr3,"\\");
        strcat(tempstr3,dir.ff_name);

        if((dir.ff_attrib & FA_DIREC)==0)
        {
            int check;
            char *ext;

            ext = fileextension(dir.ff_name);
            if( !(stricmp(ext,"dsp")==0 ||
                  stricmp(ext,"dep")==0 ||
                  stricmp(ext,"dsw")==0 ||
                  stricmp(ext,"ncb")==0 ||
                  stricmp(ext,"bmp")==0 ||
                  stricmp(ext,"opt")==0 ||
                  stricmp(ext,"plg")==0 ||
                  stricmp(ext,"ico")==0 ||
                  stricmp(ext,"rc" )==0 ||
                  stricmp(ext,"o"  )==0 ||
                  stricmp(ext,"bat")==0 ||
                  stricmp(ext,"obj")==0    ))
            {
                printf("Comparing %s and %s\n",tempstr2,tempstr3);
                check=access(tempstr3,F_OK);

                if(check)
                {
                    // seconde file not found
                    int choice;
                    if(!nonewfile)
                    {
                        printf("\nFile missing : %s\n"
                               "Delete %s ? (Y/N)\n",tempstr3,tempstr2);
                        do {
                           choice=getch();
                        } while(choice!='y' && choice!='n'
                             && choice!='Y' && choice!='N');

                        if(choice=='y' || choice=='Y')
                            unlink(tempstr2);
                    }
                }
                else
                {
                    struct stat fs1,fs2;
                    stat(tempstr2,&fs1);
                    stat(tempstr3,&fs2);

                    if(fs1.st_atime!=fs2.st_atime || fs1.st_size!=fs2.st_size)
                        CompareFile(tempstr2,tempstr3);
                }
                printf("\n");
            }
        }
        finish=findnext(&dir);
    }
}

// find file in dir2 but not in dir1
void CompareDir2(char *dirname1,char *dirname2)
{
    struct ffblk dir;
    char   filter[20]="\\*.*";
    char   tempstr[255];
    int    finish;

    strcpy(tempstr,dirname2);
    strcat(tempstr,filter);

    printf("Comparing %s and %s\n",dirname1,dirname2);

    finish=findfirst(tempstr,&dir,FA_DIREC);
    while(!finish)
    {
        char tempstr2[255],tempstr3[255];

        strcpy(tempstr2,dirname1);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        strcpy(tempstr3,dirname2);
        strcat(tempstr3,"\\");
        strcat(tempstr3,dir.ff_name);


        if(dir.ff_attrib & FA_DIREC)
        {
            if(dir.ff_name[0]!='.' && strcmp(dir.ff_name,"CVS")!=0)
            {
                int check,choice;
                check=access(tempstr3,F_OK);
                if( check )
                {
                    printf("\nDirectory missing : %s\n"
                           "Copy %s ? (Y/N)\n",tempstr3,tempstr2);
                    do {
                        choice=getch();
                    } while(choice!='y' && choice!='n'
                         && choice!='Y' && choice!='N');

                    if( choice=='y' || choice=='Y' )
                        copydir(tempstr2,tempstr3);
                }
                else
                    CompareDir2(tempstr2,tempstr3);
            }
        }
        finish=findnext(&dir);
    }

    finish=findfirst(tempstr,&dir,FA_DIREC);
    while(!finish)
    {
        char tempstr2[255],tempstr3[255];

        strcpy(tempstr2,dirname1);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        strcpy(tempstr3,dirname2);
        strcat(tempstr3,"\\");
        strcat(tempstr3,dir.ff_name);

        if((dir.ff_attrib & FA_DIREC)==0)
        {
            char *ext = fileextension(dir.ff_name);
            if( !(stricmp(ext,"dsp")==0 ||
                  stricmp(ext,"dep")==0 ||
                  stricmp(ext,"dsw")==0 ||
                  stricmp(ext,"ncb")==0 ||
                  stricmp(ext,"bmp")==0 ||
                  stricmp(ext,"opt")==0 ||
                  stricmp(ext,"plg")==0 ||
                  stricmp(ext,"ico")==0 ||
                  stricmp(ext,"rc" )==0 ||
                  stricmp(ext,"o"  )==0 ||
                  stricmp(ext,"bat")==0 ||
                  stricmp(ext,"obj")==0    ) &&
            // check if file exist in dir1
                  access(tempstr2,F_OK))
            {
                int choice;

                printf("File %s found in %s but not in %s\n"
                       "Copy it ? (y/n)\n",tempstr3,dirname2,dirname1);
                do {
                   choice=getch();
                } while(choice!='y' && choice!='n');

                if(choice=='y')
                    fcopy(tempstr2,tempstr3);

                return;


            }
        }
        finish=findnext(&dir);
    }
}

// convert tab to space
char *tabconv(char *c,int tabsize)
{
    static char buf[MAXLENLINE];
    int i=0,j=0;

    while(c[i])
    {
        if(c[i]=='\t')
        {
           do {
               buf[j++]=' ';
           } while(j % tabsize);
        }
        else
           buf[j++]=c[i];
        i++;
    }

/*
    if(j>maxline)
    {
        buf[maxline-1]='\n';  // trunc
        j=maxline;
    }
*/
    buf[j]=0;

    return buf;
}

void tabconv2(textfile_t *f,int start,int stop)
{
    static char buf[MAXLENLINE];
    char *c;
    int i,j,k;

    for(k=start;k<=stop;k++)
    {
        c=f->line[k];
        i=j=0;
        while(c[i])
        {
            if(c[i]=='\t' && c[i+1]=='\t')
            {
               buf[j++]='\t';
            }
            else
               if(c[i]=='\t')
                   do { buf[j++]=' '; } while(j & 3);
               else
                   buf[j++]=c[i];
            i++;
        }
        buf[j]=0;
        free(f->line[k]);
        f->line[k]=(char *)malloc(j);
        strcpy(f->line[k],buf);
    }
}

int c;

void initmyfgets( FILE *stream )
{
    c = fgetc(stream);    
}
  
// convert any f*** format to the good one
char *myfgets( char *string, int n, FILE *stream )
{
    int i=0;
    char *s=string;
    if( c==EOF )
        return NULL;
    while( i<n-1 )
    {
        if( c==EOF )
        {
            *s++='\0';
            break;
        }
        else    
        if( c==10 )
        {
            c = fgetc(stream);
            if( c==13 )
                c = fgetc(stream);
            *s++='\n'; // hope no overflow
            *s++='\0';                            
            break;    
        }
        else
        if( c==13 )
        {
            c = fgetc(stream);
            if( c==10 )
                c = fgetc(stream);
            *s++='\n'; // hope no overflow
            *s++='\0';
            break;    
        }       
        else
        {
            *s++=c;
            c = fgetc(stream);
        }
    }

    return string;
}

void LoadTextFile(char *filename,textfile_t *file,int tabsize)
{
    int     i;
    FILE    *f;
    char    buf[MAXLENLINE];
    int     convtab=true;
    static  char *eofline="\n";

    if( strnicmp(filename,"makefile",8)==0 )
        convtab = false;
    f=fopen(filename,"rb");
    if(!f)
       Quit("Error : file %s don't exist",filename);
    initmyfgets(f);
    i=0;

    while(!feof(f))
    {
        if(i>=MAXLINE)
           Quit("File %s too big, more than %d lines\n",filename,MAXLINE);
        if(!myfgets(buf,MAXLENLINE,f))
            break;
        if( convtab )
            strcpy(buf,tabconv(buf,tabsize));
        file->line[i]=(char *)malloc(strlen(buf)+1);
        strcpy(file->line[i],buf);
        i++;
    }
    fclose(f);

    file->lines=i;

    for(;i<MAXLINE;i++)
        file->line[i]=eofline;
}

void FreeTextFile(textfile_t *file)
{
    int i;

    for(i=0;i<file->lines;i++)
        free(file->line[i]);
        
    file->lines = 0;   
}

// afficher une partie de fichier en partant de startline jusqu'a stop line
// affiche les ligne redbeg   jusqua redstop en rouge
// decale le text vers la droite de offset caractere
void PrintfBlock(textfile_t *f,int start,int redbeg,int redstop,int stop, int offset)
{
    int i,l;
    char save80,save81,*p;

    if(start<0)
       start=0;

    for(i=start;i<=stop;i++)
    {
        if(i>=redbeg && i<=redstop)
            textattr(colordiff);
        else
            textattr(colortext);
        if( f->line[i] )
            l=strlen(f->line[i]);
        else
            l=0;
        if(l<=offset)
        {
            cprintf("\n\r");
            continue;
        }
        p=&(f->line[i][offset]);
        l-=offset;
        if( l>SCREENWIDTH-1)
        {
            save80=p[SCREENWIDTH-2];
            save81=p[SCREENWIDTH-1];
            if( p[SCREENWIDTH-2]!='\0')
                p[SCREENWIDTH-2]='\n';
            p[SCREENWIDTH-1]='\0';
            cprintf("%s\r",p);
            p[SCREENWIDTH-2]=save80;
            p[SCREENWIDTH-1]=save81;
        }
        else
            cprintf("%s\r",p);
    }
}

// copy only the syntax, compress all spacing in 1 space
char *delspace(char *s)
{
static char buf[MAXLENLINE];
    int i=0,j=0;

    if( !compspace )
    {
        // remove just trailing space
        strcpy(buf, s);
        for(i = strlen(buf)-1;(buf[i]==' ' || buf[i]=='\n' || buf[i]=='\r') && i>=0;i--)
            buf[i]='\0';
        buf[i+1]='\n';
        buf[i+2]='\0';
        return buf;
    }

    while(s[i]!='\n' && s[i])
    {
       if(s[i]==' ')
       {
           while(s[i]==' ' && s[i]!='\n' && s[i]) { i++; }
           if(s[i]!='\n' && s[i])
              buf[j++]=' ';
       }
       else
           buf[j++]=s[i++];
    }
    buf[j++]='\n';
    buf[j]='\0';

    return buf;
}

int LineCmp(char **a1,char **a2,int linetocmp)
{
    char buf[MAXLENLINE];
    char buf2[MAXLENLINE];
    int  i;

    for(i=0;i<linetocmp;i++)
    {
        strcpy(buf,delspace(a1[i]));
        strcpy(buf2,delspace(a2[i]));
        if(strcmp(buf,buf2))
            return false;
    }
    return true;
}

void Resynchronize(textfile_t *f1,int line1,
                   textfile_t *f2,int line2,
                   int *ri,int *rj,int maxlines,int sameline)
{
    int  i,j,animcount=0;
    char anim[]="-\\|/";
    // must compare line1+i and line2+j with all i and j
    // insteed do a double for do fist the most probable
    // therefore try first :
    //  i = 0 1 0 2 1 0 3 2 1 0 4 ...
    //  j = 0 0 1 0 1 2 0 1 2 3 0 ...
    cprintf("Resynchronising %c",anim[animcount]);
    for(i=1;i<maxlines;i++)
    {
        for(j=0;j<=i;j++)
            if(LineCmp(&f1->line[line1+i-j]
                      ,&f2->line[line2+j],sameline))
            {
                *ri=i-j;
                *rj=j;

                return;
            }
        animcount++;
        animcount&=0xF;
        // not to mush printf this slowdown a lot
        if( (animcount&3)==0 )
            cprintf("\b%c",anim[animcount>>2]);
    }
    Quit("Too mush difference between the files\n");
}

void cnprintf(char *s,int n)
{
    int i;
    for(i=0;i<n;i++)
        cputs(s);
}

void PrintVDiff(char *filename1,textfile_t *f1,int line1,
                char *filename2,textfile_t *f2,int line2,
                int i,int j,int l,int offset)
{
     int k;

     clrscr();

     textattr(colortitle);
     k=(SCREENWIDTH-strlen(filename1)-2-15)/2-1;
     cnprintf("=",k);
     cprintf(" %s ",filename1);
     cnprintf("=",k);
     textattr(colorkeys);
     cprintf(" [9]  All U[p]");
     cprintf("\n\r");
     k=((screenline-2)/2-2-i)/2;
     PrintfBlock(f1,line1-k+l,line1,line1+i-1,line1+i+k+l,offset);

     textattr(colortitle);
     k=(SCREENWIDTH-strlen(filename2)-2-15)/2-1;
     cnprintf("=",k);
     cprintf(" %s ",filename2);
     cnprintf("=",k);
     textattr(colorkeys);
     cprintf(" [3] All [D]own");
     cprintf("\n\r");
     k=((screenline-2)/2-2-j)/2;
     PrintfBlock(f2,line2-k+l,line2,line2+j-1,line2+j+k+l,offset);

     textattr(colorkeys);
     cprintf("[<Pg Up><Pg Down>%c%c] [-]%d lines[+] [u]ndo [e]dit [c]ompress space",27,26,sameline);
     fflush(stdout);
     textattr(colornormal);
}

// copy file date of source to dest
void CopyFileDate(char *dest,char *source)
{
    struct stat    fs1;
    struct utimbuf fs2;

    stat(source,&fs1);
    fs2.actime  = fs1.st_mtime;
    fs2.modtime = fs1.st_mtime;

    utime(dest,&fs2);
}

void CompareFile(char *filename1,char *filename2)
{
    int  i,j,choice,k,l;
    int  line1,line2;
    FILE *outfile;
    int  maxlines,always,enterline=0,column=0;
    int  oneselected=false,twoselected=false;

    LoadTextFile(filename1,&f1,tab1);
    LoadTextFile(filename2,&f2,tab2);
    outfile=fopen(tempfilename,"wt");
    if( !outfile )
         Quit("Error: No way to open %s\n",tempfilename);
    line1=0;
    line2=0;
    always=0;
    maxlines=f1.lines+f2.lines+1;

//    if(maxlines<f2.lines+1)
//        maxlines=f2.lines;

    while(line1<maxlines && line2<maxlines)
    {
        if(!LineCmp(&f1.line[line1],&f2.line[line2],1))
        {   // different
            // resynchronisation
            int oldcompspace = compspace;
            l=0;column=0;
            Resynchronize(&f1,line1,&f2,line2,&i,&j,maxlines,sameline);

            if( strncmp(f1.line[line1],"// $",4)==0 &&
                strncmp(f2.line[line2],"// $",4)==0)
                choice = '9'; // skip stupid cvs changes
            else
            {
                choice = 0;
              // ask the possibility and write it to the file
                do {
                   PrintVDiff(filename1,&f1,line1,
                              filename2,&f2,line2,i,j,l,column);

                   if(always==0)
                   {
                       choice=getch();
                       if(choice==0) choice=getch()+256;
                   }

                   switch(choice) {
                      case KEY_PGUP   : l-=20;break;
                      case KEY_PGDN   : l+=20;break;
                      case KEY_CUP    : l--;break;
                      case KEY_CDOWN  : l++;break;
                      case KEY_CRIGHT : column++;break;
                      case KEY_CLEFT  : if( column )
                                            column--;
                                        break;
                      case KEY_HOME   : column=0;break;
                      case KEY_END    : column+=20;break;

                      case '+' :
                          sameline++;
                          Resynchronize(&f1,line1,&f2,line2,&i,&j,maxlines,sameline);
                          break;
                      case '-' :
                          if( sameline>1 ) {
                              sameline--;
                              Resynchronize(&f1,line1,&f2,line2,&i,&j,maxlines,sameline);
                          }
                          break;
                      case 'c' :
                          compspace = !compspace;
                          Resynchronize(&f1,line1,&f2,line2,&i,&j,maxlines,sameline);
                          break;
/*
                      case 't' :
                          tabconv2(&f1,line1,line1+i-1);
                          Resynchronize(&f1,line1,&f2,line2,&i,&j,maxlines,sameline);
                          break;
*/
                      case 'p' : always=1;break;
                      case 'd' : always=2;break;
                      case 'u' :
                          fclose(outfile);
                          FreeTextFile(&f1);
                          FreeTextFile(&f2);

                          // remove the temp file
                          unlink(tempfilename);

                          CompareFile(filename1,filename2);

                          return;
                      case 'e' :
                      {
                          char params[256];
                          fclose(outfile);
                          FreeTextFile(&f1);
                          FreeTextFile(&f2);

                          // remove the temp file
                          unlink(tempfilename);

                          sprintf(params,"\"%s/%d\" \"%s/%d\"",filename1,line1,filename2,line2);

                          spawnlp(P_WAIT,editor,params,params,NULL);
                          printf("\n\nHit Enter to continue\n");
                          getchar();

                          CompareFile(filename1,filename2);
                          return;
                      }
                   }

                } while(choice!='3' && choice!='9' && always==0);
            }
            printf("\n");

            if(choice=='9'|| always==1)
            {
                int blankline;
                for(blankline=0;blankline<enterline;blankline++)
                    fputs("\n",outfile);
                enterline=0;

                for(k=0;k<i;k++)
                    fputs(f1.line[line1+k],outfile);

                oneselected=true;
            }
            else
            if(choice=='3'|| always==2)
            {
                int blankline;
                for(blankline=0;blankline<enterline;blankline++)
                    fputs("\n",outfile);
                enterline=0;

                for(k=0;k<j;k++)
                    fputs(f2.line[line2+k],outfile);

                twoselected=true;
            }
          // resynchronize
            line1+=i;
            line2+=j;
            compspace = oldcompspace;
        }
        // skip "\n" line at the end of the file
        // it is added for more easy compare at load time
        if(strcmp(f1.line[line1],"\n")==0)
            enterline++;
        else
        {
            int i;
            for(i=0;i<enterline;i++)
                fputs("\n",outfile);
            enterline=0;
            fputs(f1.line[line1],outfile);
        }
        line1++;
        line2++;
    }
    fclose(outfile);
    FreeTextFile(&f1);
    FreeTextFile(&f2);

    // now overwrite the source file
    if( !twoselected )
        // this is not necessarie but in futur version we can use trie directory
        fcopy(filename1,filename1);
    else
    if( !oneselected )
        fcopy(filename1,filename2);
    else
        // twoselected==true && oneselected==true
        fcopy(filename1,tempfilename);

    // remove the temp file
    unlink(tempfilename);
}

void fcopy(char *dest,char *source)
{
    FILE *f1,*f2;
    char buf[4096];
    int  byteread;

    if( stricmp(dest,source)==0 )
        return;

    f1=fopen(dest,"wb");
    f2=fopen(source,"rb");

    while(!feof(f2))
    {
        byteread=fread(&buf,1,4096,f2);
        if(byteread!=fwrite(buf,1,byteread,f1))
            Quit("Copying %s to %s : Write Error (disk full ?)\n",source,dest);
    }

    fclose(f1);
    fclose(f2);

    // copy too the file date
    CopyFileDate(dest,source);
}

// recurisve remove dir
void dirunlink(char *dirname)
{
    struct ffblk dir;
    char   tempstr[255];
    int    finish;

    strcpy(tempstr,dirname);
    strcat(tempstr,"\\*.*");

    // remove all dirs and file inside
    finish=findfirst(tempstr,&dir,0);
    while(!finish)
    {
        char tempstr2[255];

        strcpy(tempstr2,dirname);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        if(dir.ff_attrib & FA_DIREC)
        {
            if( dir.ff_name[0]!='.' )
                dirunlink( tempstr2 );
        }
        else
            unlink( tempstr2 );

        finish=findnext(&dir);
    }

    // remove dir (it is now empty)
    unlink( dirname );
}

void copydir(char *destdirname,char *srcdirname)
{
    struct ffblk dir;
    char   tempstr[255];
    int    finish;

    mkdir(destdirname, S_IWUSR);

    strcpy(tempstr,srcdirname);
    strcat(tempstr,"\\*.*");

    // remove all dirs and file inside
    finish=findfirst(tempstr,&dir,0);
    while(!finish)
    {
        char tempstr2[255],tempstr3[255];

        strcpy(tempstr2,srcdirname);
        strcat(tempstr2,"\\");
        strcat(tempstr2,dir.ff_name);

        strcpy(tempstr3,destdirname);
        strcat(tempstr3,"\\");
        strcat(tempstr3,dir.ff_name);

        if(dir.ff_attrib & FA_DIREC)
        {
            if( dir.ff_name[0]!='.' )
                copydir( tempstr3, tempstr2 );
        }
        else
            fcopy( tempstr3, tempstr2 );

        finish=findnext(&dir);
    }

}
