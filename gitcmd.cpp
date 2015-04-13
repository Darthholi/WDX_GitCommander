// gitcmd.cpp : Defines the entry point for the DLL application.
//
//posunu li sloupec doelva a nic do nej nenapisu, zustane info z predchoziho sloupce. TO se muze hodit.
//size a pripona u adresaru nejsou, takze tam zobrazovat co?
//branch, origin, datum posl. commitu,
//nahradit datum datumem posledniho predchoziho commitu
//posledni commit message..?
//pluginst.inf
//plugman protoze neni schvalenej plugin.
//jmeno a pripona nejdou odstranit ale kdyz budu zobrazovat nazev vetve tak ho muzu zobrazit prez priponu
//jak je to s datetime ?
//zobraz neco pro radku s  .. ...?
//if (_tcslen(FileName)<=3) .. tohle?
//folder .git - size is also a nice column
//https://github.com/libgit2/GitForDelphi/blob/binary/tests/git2.dll
//https://github.com/libgit2/GitForDelphi
//remote :git_remote_list
//posledni commit ovlivnujici file:GIT_EXTERN(int) git_commit_entry_last_commit_id(git_oid *out,	const git_repository *repo,	const git_commit *commit,	const char *path);
//komentar posledniho cimmitu?
// cas posledniho cimmitu?
//autor posl commitu
//origin, vetev, tag
//git stat pro ten soubor
//git log https://libgit2.github.com/libgit2/ex/HEAD/log.html

//pridat cache poslednich nekolika .git repozitaru a reference na nej.
//cleaning kdyz se otevre jiny, nebo kdyz se zavre dll.

//if C++ builder5:
#define _BCB
#define _BUILD_DLL_
//GIT:
//#include "include/git2.h"
#include "uGitForBCB.h"
#include "StdAfx.h"
#include "time.h"
#include "contentplug.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream.h>
#include <algorithm>

#define _detectstring ""

#define fieldcount 31

char* fieldnames[fieldcount]={
	"name","Size","creationdate","creationtime",
	"writedate","writetime","accessdate","accesstime",
	"attributes","attrstr",
	"archive","read only","hidden","system",
	"compressed","encrypted","sparse",
	"versionstring","versionnr","file type","random nr",
	"CutNameStart","DayOfYear","PathLenAnsi","PathLenUnicode",
	"JPG+RAW present",
  "LastCommit", "CurrentBranch", "CommitAuthor", "CommitDate", "CommitAge"};

int fieldtypes[fieldcount]={
		ft_string,ft_numeric_floating,ft_datetime,ft_time,
		ft_datetime,ft_time,ft_datetime,ft_time,
		ft_numeric_32,ft_string,
		ft_boolean,ft_boolean,ft_boolean,ft_boolean,
		ft_boolean,ft_boolean,ft_boolean,
		ft_string,ft_numeric_floating,
		ft_multiplechoice,ft_numeric_32,ft_string,
		ft_numeric_32,ft_numeric_32,ft_numeric_32,ft_boolean,
    ft_string, ft_string, ft_string,ft_datetime,ft_string};

char* fieldunits_and_multiplechoicestrings[fieldcount]={
		"","bytes|kbytes|Mbytes|Gbytes","","",
		"","","","",
		"","",
		"","","","",
		"","","",
		"","","file|folder|reparse point","","0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20",
		"","","","",
    "","","name|email","",""};

int fieldflags[fieldcount]={
    0,contflags_passthrough_size_float | contflags_edit,contflags_edit,contflags_edit,
	contflags_substdate | contflags_edit,contflags_substtime | contflags_edit,contflags_edit,contflags_edit,
    contflags_substattributes,contflags_substattributestr,
    0,0,0,0,
    0,0,0,
	0,0,0,0,0,0,0,0,0,
  0,0,0,0,0};

int sortorders[fieldcount]={
  1,-1,-1,-1,
  -1,-1,-1,-1,
  1,1,
  1,1,1,1,
  1,1,1,
  1,1,1,1,1,1,1,1,
  1,1,1,1,-1};


char* multiplechoicevalues[3]={
	"file","folder","reparse point"
};

BOOL GetValueAborted=false;


BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if (ul_reason_for_call==DLL_PROCESS_ATTACH)
  {
		srand( (unsigned)time( NULL ) );
    git_libgit2_init();
  }
  if (ul_reason_for_call==DLL_PROCESS_DETACH)
  {
    git_libgit2_shutdown();
  }
  return TRUE;
}

char* strlcpy(char* p,const char* p2,int maxlen)
{
    if ((int)strlen(p2)>=maxlen) {
        strncpy(p,p2,maxlen);
        p[maxlen]=0;
    } else
        strcpy(p,p2);
    return p;
}

TCHAR* _tcslcpy(TCHAR* p,const TCHAR* p2,int maxlen)
{
    if ((int)_tcslen(p2)>=maxlen) {
        _tcsncpy(p,p2,maxlen);
        p[maxlen]=0;
    } else
        _tcscpy(p,p2);
    return p;
}

PLUGFUNC int __stdcall ContentGetDetectString(char* DetectString,int maxlen)
{
	strlcpy(DetectString,_detectstring,maxlen);
	return 0;
}

PLUGFUNC int __stdcall ContentGetSupportedField(int FieldIndex,char* FieldName,char* Units,int maxlen)
{
	if (FieldIndex==10000) {
		strlcpy(FieldName,"Compare as text",maxlen-1);
		Units[0]=0;
		return ft_comparecontent;
	}
	if (FieldIndex<0 || FieldIndex>=fieldcount)
		return ft_nomorefields;
	strlcpy(FieldName,fieldnames[FieldIndex],maxlen-1);
	strlcpy(Units,fieldunits_and_multiplechoicestrings[FieldIndex],maxlen-1);
	return fieldtypes[FieldIndex];
}

int monthdays[12]={31,28,31,30,31,30,31,31,30,31,30,31};

BOOL LeapYear(int year)
{
	return ((year % 4)==0) && ((year % 100)!=0 || (year % 400==0));
}

bool dirExists(const char *path)
{
    struct stat info;

    if(stat( path, &info ) != 0)
        return false;
    else if(info.st_mode & S_IFDIR)
        return true;
    else
        return false;
}
string number_fmt(__int64 n)
{
  // cout << "(" << n << ")" << endl;
  char s[128];
  sprintf(s, "%Li", n);
  string r(s);
  reverse(r.begin(), r.end());
  int space_inserted = 0;
  size_t how_many_spaces = r.length() / 3;

  if(r.length() % 3 != 0)
      how_many_spaces += 1;

  for(int i = 1; i < how_many_spaces; ++i)
  {
      r.insert(3 * i + space_inserted, " ");
      space_inserted += 1;
  }
  reverse(r.begin(), r.end());

  return r;
}
void TimetToFileTime( time_t t, LPFILETIME pft )
{
    LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD) ll;
    pft->dwHighDateTime = ll >>32;
}

//git and caching repos (to come)
enum EPositionType{EPTNothing, //not git;
EPTGitOK,                       //The viewed directory has .git subfolder (filename is a directory)
EPTGitDirect,                   //this is the .git folder                 (filename is a directory)
EPTInside};                     //this folder is some folder inside current repository
git_repository *CheckRepo(WIN32_FIND_DATA fd, TCHAR* FileName, EPositionType &pRet)
{
  if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))  //for files:
  {
    pRet=EPTNothing;
    return NULL;
  }
  //...(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)//String to display... now this is the magic
  git_repository *repo = NULL;
  if (strstr(FileName,"\\.git")==FileName+strlen(FileName)-5) //now we are INSIDE a repo
  {
    pRet=EPTGitDirect;
    if (0==git_repository_open(&repo, FileName))
      return repo;
  }
  else
  {
    char xTemp[2048];
    strcpy(xTemp,FileName);
    strcat(xTemp,"\\.git");
    if (dirExists(xTemp)) // we are looking at the repo
    {
      pRet=EPTGitOK;
      if (0==git_repository_open(&repo, FileName))
        return repo;
     }
  }
  return NULL;
}
void ByeRepo(git_repository *pBye)
{
  git_repository_free(pBye);
}
//end git


PLUGFUNC int __stdcall ContentGetValue(TCHAR* FileName,int FieldIndex,int UnitIndex,void* FieldValue,int maxlen,int flags)
{
	WIN32_FIND_DATA fd;
	FILETIME lt;
	SYSTEMTIME st;
	HANDLE fh;
	DWORD handle;
	DWORD dwSize,dayofyear;
	int i;
	__int64 filesize;
	GetValueAborted=false;

	if (_tcslen(FileName)<=3)
    return ft_fileerror;

	if (flags & CONTENT_DELAYIFSLOW) {
		if (FieldIndex==17)
			return ft_delayed;
		if (FieldIndex==18)
			return ft_ondemand;
	}

	if (flags & CONTENT_PASSTHROUGH) {
		switch (FieldIndex) {
		case 1:  // "size"
			filesize=(__int64)*(double*)FieldValue;
			switch (UnitIndex) {
			case 1:
				filesize/=1024;
				break;
			case 2:
				filesize/=(1024*1024);
				break;
			case 3:
				filesize/=(1024*1024*1024);
				break;
			}
			*(double*)FieldValue=(double)filesize;
      strlcpy(((char*)FieldValue)+8,number_fmt(filesize).c_str(),maxlen-8-1);
			return ft_numeric_floating;
		default:
			return ft_nosuchfield;
		}
	}

	if (FieldIndex!=20 && FieldIndex!=23 && FieldIndex!=24)  //not needed for these fields!
		fh=FindFirstFile(FileName,&fd);
	else
		fh=0;
	if (fh!=INVALID_HANDLE_VALUE) {
		if (FieldIndex!=20 && FieldIndex!=23 && FieldIndex!=24)
			FindClose(fh);
		switch (FieldIndex) {
		case 0:  //	"name"
			_tcslcpy((TCHAR*)FieldValue,fd.cFileName,maxlen-1);
			break;
    case 30: //"CommitAge"
    case 29: //"CommitDatetime"
    case 28: //"CommitAuthor"
    case 27:  //"CurrentBranch",
    case 26: //git last commit message
    {
      EPositionType xRet;
      git_repository *repo = CheckRepo(fd,FileName,xRet);
      bool xGitPrinted=false;
      if (repo)                   //possibility to display different things for different xRet;
      {
        //get commit and commit message
        git_commit * commit = NULL; /* the result */
        git_oid oid_parent_commit;  /* the SHA1 for last commit */
        /* resolve HEAD into a SHA1 */
        int rc = git_reference_name_to_id( &oid_parent_commit, repo, "HEAD" );
        if ( rc == 0 )
        {
          /* get the actual commit structure */
          rc = git_commit_lookup( &commit, repo, &oid_parent_commit );
          if ( rc == 0 )
          {
            //return commit;
            switch (FieldIndex)
            {
              case 26: //git last commitmessage
              {
                const git_signature *xSig=git_commit_author(commit);
                if (xSig)
                {
                  switch (UnitIndex)
                  {
                  case 0:        //name
                    _tcslcpy((TCHAR*)FieldValue,xSig->name,maxlen-1);
                    break;
                  case 1:       //email
                    _tcslcpy((TCHAR*)FieldValue,xSig->email,maxlen-1);
                    break;
                  }
                }
              }
              break;
            }


            git_time_t comtim=git_commit_time(commit);
            int origoffset=git_commit_time_offset(commit);
            TimetToFileTime(comtim, ((FILETIME*)FieldValue));
            /* print as string:
            int offset=origoffset;

            char sign, out[32], msg[1024];
            struct tm *intm;
            int hours, minutes;
            time_t t;

            if (offset < 0) {
              sign = '-';
              offset = -offset;
            } else {
              sign = '+';
            }

            hours   = offset / 60;
            minutes = offset % 60;

            t = (time_t)comtim + (origoffset * 60);

            intm = gmtime(&t);
            strftime(out, sizeof(out), "%a %b %e %T %Y", intm);

            sprintf(msg,"%s%s %c%02d%02d\n", "Date: ", out, sign, hours, minutes);
            _tcslcpy((TCHAR*)FieldValue,msg,maxlen-1);      */
          }
        }

        ByeRepo(repo);
      }
    }
    case 28: //"CommitAuthor"
    {
      EPositionType xRet;
      git_repository *repo = CheckRepo(fd,FileName,xRet);
      bool xGitPrinted=false;
      if (repo)                   //possibility to display different things for different xRet;
      {
        //get commit and commit message
        git_commit * commit = NULL; /* the result */
        git_oid oid_parent_commit;  /* the SHA1 for last commit */
        /* resolve HEAD into a SHA1 */
        int rc = git_reference_name_to_id( &oid_parent_commit, repo, "HEAD" );
        if ( rc == 0 )
        {
          /* get the actual commit structure */
          rc = git_commit_lookup( &commit, repo, &oid_parent_commit );
          if ( rc == 0 )
          {
            //return commit;
            const git_signature *xSig=git_commit_author(commit);
            if (xSig)
            {
              switch (UnitIndex)
              {
              case 0:        //name
                _tcslcpy((TCHAR*)FieldValue,xSig->name,maxlen-1);
                break;
              case 1:       //email
                _tcslcpy((TCHAR*)FieldValue,xSig->email,maxlen-1);
                break;
              }
            }
          }
        }

        ByeRepo(repo);
      }
    }
    break;
    case 27:  //"CurrentBranch",
    {
      EPositionType xRet;
      git_repository *repo = CheckRepo(fd,FileName,xRet);
      bool xGitPrinted=false;
      if (repo)                   //possibility to display different things for different xRet;
      {
        int error = 0;
        const char *branch = NULL;
        git_reference *head = NULL;

        error = git_repository_head(&head, repo);

        if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
          branch = NULL;
        else if (!error) {
          branch = git_reference_shorthand(head);
        }

        if (branch)
          strlcpy(((char*)FieldValue),branch,maxlen-1);
        else
          strlcpy(((char*)FieldValue),"[HEAD]",maxlen-1);

        git_reference_free(head);
        ByeRepo(repo);
      }
    }
    break;
    case 26: //git last commit message
    {
      EPositionType xRet;
      git_repository *repo = CheckRepo(fd,FileName,xRet);
      if (repo)
      {
        //get commit and commit message
        git_commit * commit = NULL; /* the result */
        git_oid oid_parent_commit;  /* the SHA1 for last commit */


        /* resolve HEAD into a SHA1 */
        int rc = git_reference_name_to_id( &oid_parent_commit, repo, "HEAD" );
        if ( rc == 0 )
        {
          /* get the actual commit structure */
          rc = git_commit_lookup( &commit, repo, &oid_parent_commit );
          if ( rc == 0 )
          {
            //return commit;

            char *xCMsg=(char*)git_commit_summary(commit);//whole message: git_commit_message
            _tcslcpy((TCHAR*)FieldValue,xCMsg,maxlen-1);

            git_commit_free(commit);
          }
          //else return NULL
        }
        if (rc!=0) _tcslcpy((TCHAR*)FieldValue,"Cant get commit message.",maxlen-1);
        //else return NULL;
        //end get commit and commit message

        ByeRepo(repo);
      }
    }
    break;
		case 1:  // "size"  + SPECIAL FOR GIT
			filesize=fd.nFileSizeHigh;
			filesize=(filesize<<32) + fd.nFileSizeLow;
			switch (UnitIndex) {
			case 1:
				filesize/=1024;
				break;
			case 2:
				filesize/=(1024*1024);
				break;
			case 3:
				filesize/=(1024*1024*1024);
				break;
			}
			//*(__int64*)FieldValue=filesize;
      *(double*)FieldValue=filesize;
			// alternate string
      if (maxlen>12)
      {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))  //for files:
        {
          strlcpy(((char*)FieldValue)+8,number_fmt(filesize).c_str(),maxlen-8-1);
        }
        else //  if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)//String to display... now this is the magic
        {
          EPositionType xRet;
          git_repository *repo = CheckRepo(fd,FileName,xRet);
          bool xGitPrinted=false;
          if (repo)                   //possibility to display different things for different xRet;
          {
            int error = 0;
            const char *branch = NULL;
            git_reference *head = NULL;

            error = git_repository_head(&head, repo);

            if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
              branch = NULL;
            else if (!error) {
              branch = git_reference_shorthand(head);
            }

            if (branch)
              strlcpy(((char*)FieldValue)+8,branch,maxlen-8-1);
            else
              strlcpy(((char*)FieldValue)+8,"[HEAD]",maxlen-8-1);

            git_reference_free(head);
            ByeRepo(repo);
            xGitPrinted=true;
          }
          if (!xGitPrinted)
            strlcpy(((char*)FieldValue)+8,"<DIR>",maxlen-8-1);
        }
      }
			break;
		case 2:  // "creationdate"
			FileTimeToLocalFileTime(&fd.ftCreationTime,&lt);
			/*FileTimeToSystemTime(&lt,&st);
			((pdateformat)FieldValue)->wYear=st.wYear;
			((pdateformat)FieldValue)->wMonth=st.wMonth;
			((pdateformat)FieldValue)->wDay=st.wDay;*/
      *((FILETIME*)FieldValue)=lt;//lepsi, ma vic moznosti v novejch commanderech nastaveni formatovani.
			break;
		case 3:  // "creationtime",
			FileTimeToLocalFileTime(&fd.ftCreationTime,&lt);
			FileTimeToSystemTime(&lt,&st);
			((ptimeformat)FieldValue)->wHour=st.wHour;
			((ptimeformat)FieldValue)->wMinute=st.wMinute;
			((ptimeformat)FieldValue)->wSecond=st.wSecond;
			break;
		case 4:  // "writedate"
			FileTimeToLocalFileTime(&fd.ftLastWriteTime,&lt);
			/*FileTimeToSystemTime(&lt,&st);
			((pdateformat)FieldValue)->wYear=st.wYear;
			((pdateformat)FieldValue)->wMonth=st.wMonth;
			((pdateformat)FieldValue)->wDay=st.wDay;       */
      *((FILETIME*)FieldValue)=lt;//lepsi, ma vic moznosti v novejch commanderech nastaveni formatovani.
			break;
		case 5:  // "writetime"
			FileTimeToLocalFileTime(&fd.ftLastWriteTime,&lt);
			FileTimeToSystemTime(&lt,&st);
			((ptimeformat)FieldValue)->wHour=st.wHour;
			((ptimeformat)FieldValue)->wMinute=st.wMinute;
			((ptimeformat)FieldValue)->wSecond=st.wSecond;
			break;
		case 6:  // "accessdate"
			FileTimeToLocalFileTime(&fd.ftLastAccessTime,&lt);
			/*FileTimeToSystemTime(&lt,&st);
			((pdateformat)FieldValue)->wYear=st.wYear;
			((pdateformat)FieldValue)->wMonth=st.wMonth;
			((pdateformat)FieldValue)->wDay=st.wDay;*/
      *((FILETIME*)FieldValue)=lt;//lepsi, ma vic moznosti v novejch commanderech nastaveni formatovani.
			break;
		case 7:  // "accesstime",
			FileTimeToLocalFileTime(&fd.ftLastAccessTime,&lt);
			FileTimeToSystemTime(&lt,&st);
			((ptimeformat)FieldValue)->wHour=st.wHour;
			((ptimeformat)FieldValue)->wMinute=st.wMinute;
			((ptimeformat)FieldValue)->wSecond=st.wSecond;
			break;
		case 8:  // "attributes",
			*(int*)FieldValue=fd.dwFileAttributes;
			break;
		case 9:  // "attributestr",
			_tcslcpy((TCHAR*)FieldValue,TEXT("----"),maxlen-1);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ((TCHAR*)FieldValue)[0]='r';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)  ((TCHAR*)FieldValue)[1]='a';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)   ((TCHAR*)FieldValue)[2]='h';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)  ((TCHAR*)FieldValue)[3]='s';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) _tcsncat((TCHAR*)FieldValue,TEXT("c"),maxlen-1);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) _tcsncat((TCHAR*)FieldValue,TEXT("e"),maxlen-1);
			break;
		case 10: // "archive"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE;
			break;
		case 11: // "read only"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
			break;
		case 12: // "hidden"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN;
			break;
		case 13: // "system"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM;
			break;
		case 14: // "compressed"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED;
			break;
		case 15: // "encrypted"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;
			break;
		case 16: // "sparse"
			*(int*)FieldValue=fd.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE;
			break;
		case 17: // "versionstring"
		case 18: // "versionnr"
			dwSize = GetFileVersionInfoSize(FileName, &handle);
			if(dwSize) {
				VS_FIXEDFILEINFO *lpBuffer;
				void *pData=malloc(dwSize);
				GetFileVersionInfo(FileName, handle, dwSize, pData);
				if (VerQueryValue(pData, TEXT("\\"), (void **)&lpBuffer, (unsigned int *)&dwSize)) {
					DWORD verhigh=lpBuffer->dwFileVersionMS >> 16;
					DWORD verlow=lpBuffer->dwFileVersionMS & 0xFFFF;
					if (FieldIndex==17) {
						TCHAR buf[128];
						DWORD verhigh2=lpBuffer->dwFileVersionLS >> 16;
						DWORD verlow2=lpBuffer->dwFileVersionLS & 0xFFFF;
						wsprintf(buf,TEXT("%d.%d.%d.%d"),verhigh,verlow,verhigh2,verlow2);
						_tcslcpy((TCHAR*)FieldValue,buf,maxlen-1);
					} else {
						double version=verlow;
						while (version>=1)
							version/=10;
						version+=verhigh;
						*(double*)FieldValue=version;
						// make sure we have the correct DLL
					}
				} else {
					free(pData);
					return ft_fileerror;
				}
				free(pData);
			} else
				return ft_fileerror;
			break;
		case 19: // "file type"
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
				strlcpy((char*)FieldValue,multiplechoicevalues[0],maxlen-1);
			else if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)==0)
				strlcpy((char*)FieldValue,multiplechoicevalues[1],maxlen-1);
			else
				strlcpy((char*)FieldValue,multiplechoicevalues[2],maxlen-1);
			break;
		case 20: // "random nr"
			*(int*)FieldValue=rand();
			break;
		case 21: // "CutNameStart"
			if (UnitIndex>=0 && (int)_tcslen(fd.cFileName)>UnitIndex)
				_tcslcpy((TCHAR*)FieldValue,fd.cFileName+UnitIndex,maxlen-1);
			else
				((TCHAR*)FieldValue)[0]=0;
			break;
		case 22: // "DayOfYear"
			FileTimeToLocalFileTime(&fd.ftLastWriteTime,&lt);
			FileTimeToSystemTime(&lt,&st);
			dayofyear=st.wDay;
			for (i=1;i<st.wMonth;i++) {
				dayofyear+=monthdays[i-1];  //array is 0-based
				if (i==2 && LeapYear(st.wYear))
					dayofyear++;
			}
			*(int*)FieldValue=dayofyear;
			break;
		case 23: // "PathLenAnsi"
#ifdef UNICODE
			{
				char abuf[2048];
				WideCharToMultiByte(CP_ACP,0,FileName,-1,abuf,2047,NULL,NULL);
				*(int*)FieldValue=strlen(abuf);
			}
#else
			*(int*)FieldValue=_tcslen(FileName);
#endif
			break;
		case 24: // "PathLenUnicode"
#ifdef UNICODE
			*(int*)FieldValue=_tcslen(FileName);
#else
			{
				WCHAR wbuf[2048];
				MultiByteToWideChar(CP_ACP,0,FileName,-1,wbuf,2047);
				*(int*)FieldValue=wcslen(wbuf);
			}
#endif
			break;
		case 25:  // RAW+JPG  *.CAM,*.CCD,*.CR2,*.CRW,*.RAW
			{
				TCHAR FileName2[MAX_PATH];
				_tcslcpy(FileName2,FileName,MAX_PATH-1);
				TCHAR* ext=_tcsrchr(FileName2,'.');
				if (!ext)
					return ft_fileerror;
				if (_tcsicmp(ext,_T(".JPG"))==0) {
					_tcscpy(ext,_T(".*"));
					fh=FindFirstFile(FileName2,&fd);
					if (fh!=INVALID_HANDLE_VALUE) {
						do {
							ext=_tcsrchr(fd.cFileName,'.');
							if (ext!=NULL &&
							  _tcsicmp(ext,_T(".CAM"))==0 || _tcsicmp(ext,_T(".CCD"))==0 ||
								_tcsicmp(ext,_T(".CR2"))==0 || _tcsicmp(ext,_T(".CRW"))==0 ||
								_tcsicmp(ext,_T(".RAW"))==0) {
								FindClose(fh);
								*(int*)FieldValue=1;
								return ft_boolean;
							}
						} while (FindNextFile(fh,&fd));
						FindClose(fh);
					}
				} else if (_tcsicmp(ext,_T(".CAM"))==0 || _tcsicmp(ext,_T(".CCD"))==0 ||
					_tcsicmp(ext,_T(".CR2"))==0 || _tcsicmp(ext,_T(".CRW"))==0 ||
					_tcsicmp(ext,_T(".RAW"))==0) {
					_tcscpy(ext,_T(".JPG"));
					fh=FindFirstFile(FileName2,&fd);
					if (fh!=INVALID_HANDLE_VALUE) {
						FindClose(fh);
						*(int*)FieldValue=1;
						return ft_boolean;
					}
				} else
					return ft_fileerror;
				*(int*)FieldValue=0;;
				return ft_boolean;
			}
		default:
			return ft_nosuchfield;
		}
	} else
		return ft_fileerror;
	int retval=fieldtypes[FieldIndex];  // very important!
#ifdef UNICODE
	if (retval==ft_string) {
		switch (FieldIndex) {
		case 0: // name
		case 9: // attrstr
		case 17:// versionstr
		case 21:// "CutNameStart"
			retval=ft_stringw;
		}
	}
#endif;
	return retval;
}

PLUGFUNC int __stdcall ContentSetValue(TCHAR* FileName,int FieldIndex,int UnitIndex,int FieldType,void* FieldValue,int flags)
{
	int retval=ft_nomorefields;
	FILETIME oldfiletime,localfiletime;
	FILETIME *p1,*p2,*p3;
	SYSTEMTIME st;
	HANDLE f;
	pdateformat FieldDate;
	ptimeformat FieldTime;

	if (FileName==NULL)     // indicates end of operation -> may be used to flush data
		return ft_nosuchfield;

	if (FieldIndex<0 || FieldIndex>=fieldcount)
		return ft_nosuchfield;
	else if (fieldflags[FieldIndex] & contflags_edit==0)
		return ft_nosuchfield;
	else {
		switch (FieldIndex) {
		case 1:  // size
			retval=ft_fileerror;
			break;
		case 2:  // "creationdate"
		case 4:  // "writedate"
		case 6:  // "accessdate"
			FieldDate=(pdateformat)FieldValue;
			p1=NULL;p2=NULL;p3=NULL;
			if (FieldIndex==2)
				p1=&oldfiletime;
			else if (FieldIndex==4)
				p3=&oldfiletime;
			else
				p2=&oldfiletime;
			f= CreateFile (FileName,
                      GENERIC_READ|GENERIC_WRITE, // Open for reading+writing
                      0,                      // Do not share
                      NULL,                   // No security
                      OPEN_EXISTING,          // Existing file only
                      FILE_ATTRIBUTE_NORMAL,  // Normal file
                      NULL);
			GetFileTime(f,p1,p2,p3);
			FileTimeToLocalFileTime(&oldfiletime,&localfiletime);
			FileTimeToSystemTime(&localfiletime,&st);
			st.wDay=FieldDate->wDay;
			st.wMonth=FieldDate->wMonth;
			st.wYear=FieldDate->wYear;
			SystemTimeToFileTime(&st,&localfiletime);
			LocalFileTimeToFileTime(&localfiletime,&oldfiletime);
			if (!SetFileTime(f,p1,p2,p3))
				retval=ft_fileerror;
			CloseHandle(f);
			break;
		case 3:  // "creationtime"
		case 5:  // "writetime"
		case 7:  // "accesstime"
			FieldTime=(ptimeformat)FieldValue;
			p1=NULL;p2=NULL;p3=NULL;
			if (FieldIndex==3)
				p1=&oldfiletime;
			else if (FieldIndex==5)
				p3=&oldfiletime;
			else
				p2=&oldfiletime;
			f= CreateFile (FileName,
                      GENERIC_READ|GENERIC_WRITE, // Open for reading+writing
                      0,                      // Do not share
                      NULL,                   // No security
                      OPEN_EXISTING,          // Existing file only
                      FILE_ATTRIBUTE_NORMAL,  // Normal file
                      NULL);
			GetFileTime(f,p1,p2,p3);
			FileTimeToLocalFileTime(&oldfiletime,&localfiletime);
			FileTimeToSystemTime(&localfiletime,&st);
			st.wHour=FieldTime->wHour;
			st.wMinute=FieldTime->wMinute;
			st.wSecond=FieldTime->wSecond;
			st.wMilliseconds=0;
			SystemTimeToFileTime(&st,&localfiletime);
			LocalFileTimeToFileTime(&localfiletime,&oldfiletime);
			if (!SetFileTime(f,p1,p2,p3))
				retval=ft_fileerror;
			CloseHandle(f);
			break;
		}
	}
	return retval;
}

PLUGFUNC void __stdcall ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{

}

PLUGFUNC void __stdcall ContentStopGetValue	(TCHAR* FileName)
{
	GetValueAborted=true;
}

PLUGFUNC int __stdcall ContentGetSupportedFieldFlags(int FieldIndex)
{
	if (FieldIndex==-1)
		return contflags_edit | contflags_substmask;
	else if (FieldIndex<0 || FieldIndex>=fieldcount)
		return 0;
	else
		return fieldflags[FieldIndex];
}

PLUGFUNC int __stdcall ContentGetDefaultSortOrder(int FieldIndex)
{
	if (FieldIndex<0 || FieldIndex>=fieldcount)
		return 1;
	else
		return sortorders[FieldIndex];
}

#define bufsz 32768

typedef int (__stdcall *PROGRESSCALLBACKPROC)(int nextblockdata);

// 1=equal, 2=equal if text, 0=not equal, -1= could not open files, -2=abort, -3=not our file type
PLUGFUNC int __stdcall ContentCompareFiles(PROGRESSCALLBACKPROC progresscallback,int compareindex,TCHAR* filename1,TCHAR* filename2,FileDetailsStruct* filedetails)
{
	char *pbuf1,*pbuf2;
	HANDLE f1,f2;

	if (compareindex!=10000)
		return -3;

	pbuf1=(char*)malloc(2*bufsz);  // we need twice the space for resync!
	if (!pbuf1)
		return -1;
	pbuf2=(char*)malloc(2*bufsz);
	if (!pbuf2) {
		free(pbuf1);
		return -1;
	}
	f1=CreateFile(filename1,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (f1==INVALID_HANDLE_VALUE) {
		free(pbuf1);
		free(pbuf2);
		return -1;
	}
	f2=CreateFile(filename2,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (f2==INVALID_HANDLE_VALUE) {
		CloseHandle(f1);
		free(pbuf1);
		free(pbuf2);
		return -1;
	}
	DWORD lasttime=GetCurrentTime();
	DWORD sizesincelastmessage=0;
	DWORD read1,read2;
	int same=1;
	DWORD remain1=0;
	DWORD remain2=0;
	BOOL skipnextlf1=false;
	BOOL skipnextlf2=false;
	BOOL dataavail=true;
	// first, try binary compare!
	do {
		BOOL ok1=true;
		BOOL ok2=true;
		dataavail=true;
		ok1=ReadFile(f1,pbuf1,bufsz,&read1,NULL);
		if (ok1) {
			remain1=read1;
			sizesincelastmessage+=read1;
			dataavail=read1>0;
		}
		ok2=ReadFile(f2,pbuf2,bufsz,&read2,NULL);
		if (ok2) {
			remain2=read2;
			dataavail=dataavail || (read2>0);
		}
		if (!ok1 || !ok2) {  // read error
			same=0;
			break;
		}
		if (remain1 && remain2 && remain1==remain2) {
			if (memcmp(pbuf1,pbuf2,remain1)==0) {
				skipnextlf1=(pbuf1[remain1-1]==13);
				skipnextlf2=(pbuf2[remain2-1]==13);
				remain1=0;
				remain2=0;
			} else
				break;
		} else
			break;
		DWORD newtime=GetCurrentTime();
		if (newtime-lasttime>500) {
			lasttime=newtime;
			if (progresscallback(sizesincelastmessage)!=0)
				same=-2;  // aborted by user
			sizesincelastmessage=0;
		}
	} while (same==1 && dataavail);


	// now compare text files
	if (same && (remain1 || remain2)) do {
		BOOL ok1=true;
		BOOL ok2=true;
		dataavail=true;
		if (remain1<bufsz) {
			ok1=ReadFile(f1,pbuf1+remain1,bufsz,&read1,NULL);
			if (ok1) {
				remain1+=read1;
				sizesincelastmessage+=read1;
			}
			dataavail=read1>0;
		}
		if (remain2<bufsz) {
			ok2=ReadFile(f2,pbuf2+remain2,bufsz,&read2,NULL);
			if (ok2)
				remain2+=read2;
			dataavail=dataavail || (read2>0);
		}
		if (!ok1 || !ok2) {  // read error
			same=0;
			break;
		}
		// compare, take as equal: CR, LF, CRLF
		char* pend1=pbuf1+remain1;
		char* pend2=pbuf2+remain2;
		char* p1=pbuf1;
		char* p2=pbuf2;
		while (p1<pend1 && p2<pend2) {
			char ch1=p1[0];
			char ch2=p2[0];
			if (skipnextlf1 && ch1==10) {
				p1++;
				skipnextlf1=false;
			} else if (skipnextlf2 && ch2==10) {
				p2++;
				skipnextlf2=false;
			} else if (ch1==ch2) {
				p1++;
				p2++;
				skipnextlf1=(ch1==13);
				skipnextlf2=(ch2==13);
			} else {
		 		same=100;
				if ((ch1==13 && ch2==10) ||
                    (ch2==13 && ch1==10)) {
					p1++;
					p2++;
					skipnextlf1=(ch1==13);
					skipnextlf2=(ch2==13);
				} else {
					same=0;
					break;
				}
			}
		}
		if (same>0) {
			// special case: one file has LF at end, other is at end
			remain1=pend1-p1;
			if (remain1)
				memmove(pbuf1,p1,remain1);
			remain2=pend2-p2;
			if (remain2)
				memmove(pbuf2,p2,remain2);
		}
		DWORD newtime=GetCurrentTime();
		if (newtime-lasttime>500) {
			lasttime=newtime;
			if (progresscallback(sizesincelastmessage)!=0)
				same=-2;  // aborted by user
			sizesincelastmessage=0;
		}
	} while (same>=1 && ((remain1>0 && remain2>0) || dataavail));
	CloseHandle(f1);
	CloseHandle(f2);
	if (same>0 && (remain1>0 || remain2>0)) {    // some unmatched data remaining at end ->error!
		if (remain1==1 && remain2==0 && pbuf1[0]==10 && skipnextlf1) {
		} else if (remain2==1 && remain1==0 && pbuf2[0]==10 && skipnextlf2) {
		} else
			same=0;
	}
	free(pbuf1);
	free(pbuf2);
	return same;
}


