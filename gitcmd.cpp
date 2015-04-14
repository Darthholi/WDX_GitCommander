// gitcmd.cpp : Gitcommander plugin for total commander
//
//pluginst.inf
//
//Ideas from:
//https://github.com/libgit2/GitForDelphi/blob/binary/tests/git2.dll
//https://github.com/libgit2/GitForDelphi
//
//Usage - hints in TC
//first format - last commit info:
//[=gitcmd.Branch]([=gitcmd.CommitAge]) [=gitcmd.FirstRemoteUrl]\n[=gitcmd.CommitMessage]\n[=gitcmd.CommitAuthor] [=gitcmd.CommitMail] [=gitcmd.CommitDate.D.M.Y h:m:s]
//last format different for dires and for files: (authors choice)
//[=gitcmd.Branch] [=gitcmd.FirstRemoteUrl]\n[=gitcmd.FallIsLast] [=gitcmd.FallAge]\n[=gitcmd.FallMessage]\n[=gitcmd.FallAuthor] [=gitcmd.FallMail] [=gitcmd.FallDate.D.M.Y h:m:s]\n[=gitcmd.GeneralStatus]
//
//Usage - Column with branch instead of DIR - SizeAndBranch

//if C++ builder5:
#define _BCB

//normal defines:
#include "StdAfx.h"
#include "contentplug.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream.h>
#include <algorithm>

//spec defines:
#ifdef _BCB
#define _BUILD_DLL_
//GIT:      normally we should do this
//#include "include/git2.h"
//but we include GitForBCB, things ripped from libgit2:
#include "uGitForBCB.h"
#include "time.h"
#include <sys/stat.h>
#endif
//about the files - the lib is compiled libgit2
//the uGitForBCB.h is ripped things from git2.h because I was not able to incliude them properly (and the idea is based on git for delphi)

//from here on it should be compilable with anything...
//TODO:
//-cache pointers to repos
//-more unicodes, now iam just lucky to have it working
//-

//rest of normal code:
#define _detectstring ""

#define fieldcount 21

//general info groups:
enum EGitWantSrc{ENone,EThisCommit,ELastAffecting,ELastFallthrough};//the checkout-commit where we stand, the last commit affected file OR empty, the last commit affected file OR currentcheckout
enum EGitWant{EMsg,EAuthor,EMail,EDate,EAge,   //can be specified from EGitWantSrc
EBranch,EFirstRemoteURL,   //no specifiers, just general info
EFileStatus,//Status for file, empty for dirc
EGeneralStatus,
ESizeBranch,
EFallIsThis
};

//specific fields: cant be done by formatting because of datetime
enum EFields{
EFSizeBranch=0,
EFThisMsg,EFThisAuthor,EFThisMail,EFThisDate,EFThisAge,   //commit where we stand
EFLastMsg,EFLastAuthor,EFLastMail,EFLastDate,EFLastAge,   //last commit affecting file or empty string
EFFallMsg,EFFallAuthor,EFFallMail,EFFallDate,EFFallAge,   //last commit affecting file or commit where we stand
EFBranch,EFFirstRemoteURL,   //no specifiers, just general info
EFFileStatus,//Status for file, empty for dirc
EFGeneralStatus, //File+Status for files, "GIT dir." for dirs.
EFFallIsThis
};
char* fieldnames[fieldcount]={
  "SizeAndBranch",                                                                 //Branch instead of <DIR>
	"CommitMessage","CommitAuthor","CommitMail","CommitDate", "CommitAge",
  "LastMessage","LastAuthor","LastMail","LastDate", "LastAge",
  "FallMessage","FallAuthor","FallMail","FallDate", "FallAge",
  "Branch","FirstRemoteUrl",                                                               //functionality for whole repo, commit we are standing at
  "FileStatus","GeneralStatus",
  "FallIsLast"
  };
int fieldtypes[fieldcount]={
		ft_numeric_floating,
    ft_string,ft_string,ft_string,ft_datetime,ft_string,
    ft_string,ft_string,ft_string,ft_datetime,ft_string,
    ft_string,ft_string,ft_string,ft_datetime,ft_string,
    ft_string,ft_string,
    ft_string,
    ft_string,
    ft_string
    };
char* fieldunits_and_multiplechoicestrings[fieldcount]={
		"bytes|kbytes|Mbytes|Gbytes",
    "","","","","",
    "","","","","",
    "","","","","",
		"","",
		"",
		"",
    ""};

int fieldflags[fieldcount]={
    contflags_passthrough_size_float | contflags_edit,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,
    0,0,
    0
    };

int sortorders[fieldcount]={
  -1,
  1,1,1,1,1,
  1,1,1,1,1,
  1,1,1,1,1,
  1,1,
  1,
  1,
  1
  };

BOOL GetValueAborted=false;


BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
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

void path_strip_filename(TCHAR *Path) {
    size_t Len = _tcslen(Path);
    if (Len==0) {return;};
    size_t Idx = Len-1;
    while (TRUE) {
        TCHAR Chr = Path[Idx];
        if (Chr==TEXT('\\')||Chr==TEXT('/')) {
            if (Idx==0||Path[Idx-1]==':') {Idx++;};
            break;
        } else if (Chr==TEXT(':')) {
            Idx++; break;
        } else {
            if (Idx==0) {break;} else {Idx--;};
        };
    };
    Path[Idx] = TEXT('\0');
};
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
//times & howto
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
void PrintTimeAgo(TCHAR *pTarget,double pAgo,unsigned int pMaxLen)
{
  #define NUMFMTSAGE 6
  int xDifs[NUMFMTSAGE]={1,60,3600,24*3600,2629743,31556926};//seconds,minutes,hours,days,approxmonths,years
  char *xNam[NUMFMTSAGE]={"secs.","mins","hrs.","days","months","years"};
  int iDo=0;
  while (iDo<NUMFMTSAGE)
  {
    if (pAgo/double(xDifs[iDo])<1.0)
      break;
    iDo++;
  }
  if (iDo >= 1)//the prev one
    iDo--;
  int xRes=int(0.5+pAgo/xDifs[iDo]);
  char msg[512];
  sprintf(msg,"%d %s", xRes, xNam[iDo]);
  _tcslcpy((TCHAR*)pTarget,msg,pMaxLen-1);
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
git_repository *CheckRepo(WIN32_FIND_DATA fd, TCHAR* FileName, EPositionType &pRet, bool pDoSearch, TCHAR *pRelFilePath)
{
  char xTemp[2048];
  strcpy(xTemp,FileName);
  pRet=EPTNothing;
  if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))  //for files - check the directory we are in
  {
    //strcpy(xTemp,FileName);
    path_strip_filename(xTemp);
  }
  int xOriglen=strlen(xTemp);

  git_repository *repo = NULL;
  if (strstr(xTemp,"\\.git")==xTemp+xOriglen-5) //now we are INSIDE a repo and this is exactly the information for the invisible git dir
  {
    pRet=EPTGitDirect;
    if (0!=git_repository_open(&repo, xTemp))
      repo=NULL;
  }
  else
  {
    strcat(xTemp,"\\.git");
    if (dirExists(xTemp)) // we are looking at the repo
    {
      pRet=EPTGitOK;
      if (0!=git_repository_open(&repo, xTemp))
        repo=NULL;
     }
     //else let git find it.... ..
     else if (pDoSearch)
     {
        xTemp[xOriglen]=0;
        if (0!=git_repository_open_ext(&repo, xTemp,0,NULL))
          repo=NULL;
        else
          pRet=EPTInside;
     }
  }
  if (pRelFilePath)             //we want to know the relative path in git repo of this file.
  {
    if (repo && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      const char *xRepdir=git_repository_path(repo);   //returns different than windows slashes!!!
      int xLastK=0;
      for (int i=0;i<sizeof(xTemp);i++)
      {
        if (((FileName[i]=='\\' && xRepdir[i]=='/') || (FileName[i]=='/' && xRepdir[i]=='\\')))
          xLastK=i;
        if (FileName[i]==0 || xRepdir==0 ||
        ( tolower(FileName[i])!=tolower(xRepdir[i]) &&
         ((FileName[i]!='\\' || xRepdir[i]!='/') && (FileName[i]!='/' || xRepdir[i]!='\\')) ))
        {
          break;
        }
      }
      xLastK=xLastK+1;//just after the "\\" or /
      //(*pRelFilePath)=FileName+i;
      strcpy(pRelFilePath,FileName+xLastK);                //alternatively we can modify the original FileName, because xPtrFN points there. But we know it, ok.
      for (int ii=0;ii<strlen(pRelFilePath);ii++)
      {
        if (pRelFilePath[ii]=='\\') pRelFilePath[ii]='/';//for git
      }
    }
    else
      pRelFilePath[0]=0;
  }
  return repo;
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

	/*if (flags & CONTENT_DELAYIFSLOW) {
		if (FieldIndex==17)
			return ft_delayed;
		if (FieldIndex==18)
			return ft_ondemand;
	} */

	if (flags & CONTENT_PASSTHROUGH) {
		switch (FieldIndex) {
		case EFSizeBranch:
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

  //parsing what we want:
  EGitWant xWant;
  EGitWantSrc xWantFrom=ENone;
  switch (FieldIndex)
  {
    default:
    return ft_nosuchfield;
    break;
    case EFSizeBranch: xWant=ESizeBranch; break;
    case EFBranch: xWant=EBranch; break;
    case EFFirstRemoteURL: xWant=EFirstRemoteURL; break;
    case EFFileStatus: xWant=EFileStatus; break;
    case EFGeneralStatus: xWant=EGeneralStatus; break;
    case EFFallIsThis: xWant=EFallIsThis; xWantFrom=ELastFallthrough; break;
    case EFThisMsg:
    case EFThisAuthor:
    case EFThisMail:
    case EFThisDate:
    case EFThisAge:
      xWant=EMsg+(FieldIndex-EFThisMsg);
      xWantFrom=EThisCommit;
    break;
    case EFLastMsg:
    case EFLastAuthor:
    case EFLastMail:
    case EFLastDate:
    case EFLastAge:
      xWant=EMsg+(FieldIndex-EFLastMsg);
      xWantFrom=ELastAffecting;
    break;
    case EFFallMsg:
    case EFFallAuthor:
    case EFFallMail:
    case EFFallDate:
    case EFFallAge:
      xWant=EMsg+(FieldIndex-EFFallMsg);
      xWantFrom=ELastFallthrough;
    break;
  }

  bool xNeedfd=true; //everybody needs the information if file or directory
  bool xStdGit= FieldIndex!=EFSizeBranch;//only size is treated differently
  bool xNeedRepo=true; //everybody (in case xStdGit) needs repo
  bool xNeedCommit = xWantFrom!=ENone;
  bool xNeedLastAffCommit= (xWantFrom==ELastFallthrough || xWantFrom==ELastAffecting);

  //finito, now to actually do it.

  if (xNeedfd)
		fh=FindFirstFile(FileName,&fd);
	else
		fh=0;
	if (fh!=INVALID_HANDLE_VALUE)
  {
		if (xNeedfd)
			FindClose(fh);

    if (xStdGit)
    {
      EPositionType xRet;                               //false -> let these functions return something ONLY on directories
      git_repository *repo = NULL;
      int rc=0;
      git_commit * commit = NULL; /* the result */
      git_oid oid_parent_commit;  /* the SHA1 for last commit */
      git_oid xLastAffecting;
      git_commit *xFilesCommit = NULL;//the result commit last affecting given file
      const char *branch = NULL;
      bool xReturnEmpty=false;
      bool xDirectory=fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      char xBufFN[2048];//filename for git

      if (xNeedRepo)
        repo=CheckRepo(fd,FileName,xRet,true,xBufFN);
      if (repo)                   //possibility to display different things for different xRet;
      {                                            //if (xBufFN[0]!=0)
        //get commit and commit message
        /* resolve HEAD into a SHA1 */

        if (xNeedCommit || xNeedLastAffCommit)
        {
          rc = git_reference_name_to_id( &oid_parent_commit, repo, "HEAD" );
          if ( rc == 0 )
          {
            /* get the actual commit structure */
            rc = git_commit_lookup( &commit, repo, &oid_parent_commit );
            if ( rc == 0 && xNeedLastAffCommit)
            {
            //we have the last commit. Now to walk it:
              if ((xBufFN && xBufFN[0]!=0) && 0==git_commit_entry_last_commit_id(&xLastAffecting,repo,commit,xBufFN))
              {
                rc = git_commit_lookup( &xFilesCommit, repo, &xLastAffecting );
                //if (rc == 0)
                //{
                //  //we can do work.
                //}
              }
            }
          }
        }
        if (rc!=0)//error in getting what we wanted
        {
          if (fieldtypes[FieldIndex]==ft_string)
          {
            const git_error *error = giterr_last();
            if (error)
              _tcslcpy((TCHAR*)FieldValue,error->message,maxlen-1);
            else
              return ft_fieldempty;
          }
          else
            return ft_fieldempty;
        }

        //special first and then commit infos.
        if (xWant==EBranch) //"CurrentBranch"
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
        }
        else if (xWant==EFirstRemoteURL)
        {
          git_strarray xRs;
          if (0==git_remote_list(&xRs,repo))
          {
            if (xRs.count>=1)
            {
              git_remote *remote=NULL;
              if (0==git_remote_lookup(&remote,repo,xRs.strings[0]))//first repo
              {
                _tcslcpy((TCHAR*)FieldValue,git_remote_url(remote),maxlen-1);
                git_remote_free(remote);
              }
              else
                _tcslcpy((TCHAR*)FieldValue,xRs.strings[0],maxlen-1);  //at least the remote name
            }
            else
              _tcslcpy((TCHAR*)FieldValue,"Local rep.",maxlen-1);  //at least the remote name

            git_strarray_free(&xRs);
          }
          else
            return ft_fieldempty;
        }
        else if (xWant==EFileStatus || xWant==EGeneralStatus)
        {
          TCHAR *xTrgt;
          if (xWant==EGeneralStatus && xDirectory)
          {
            _tcslcpy((TCHAR*)FieldValue,"File ",maxlen-1);
            xTrgt=(TCHAR*)FieldValue + strlen((TCHAR*)FieldValue);
          }
          else xTrgt=(TCHAR*)FieldValue;
          if ((!xDirectory && xWant==EGeneralStatus) || xWant==EFileStatus)
          {
            unsigned int xStatF=0;
            if ((xBufFN && xBufFN[0]!=0) && 0==git_status_file(&xStatF,repo,xBufFN))
            {
              if (xStatF & GIT_STATUS_IGNORED)
                _tcslcpy(xTrgt,"ignored",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_NEW)
                _tcslcpy(xTrgt,"new",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_MODIFIED)
                _tcslcpy(xTrgt,"modified",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_DELETED)
                _tcslcpy(xTrgt,"deleted",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_TYPECHANGE)
                _tcslcpy(xTrgt,"typechange",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_RENAMED)
                _tcslcpy(xTrgt,"renamed",maxlen-1);
              else if (xStatF & GIT_STATUS_WT_UNREADABLE)
                _tcslcpy(xTrgt,"unreadable",maxlen-1);
            }
            if (xStatF==0)//nothing printed or error
            {
              //_tcslcpy((TCHAR*)FieldValue,xBufFN,maxlen-1);
              //_tcslcpy((TCHAR*)FieldValue,git_repository_path(repo),maxlen-1);//debug del.
              const git_error *error = giterr_last();
              if (error)
                _tcslcpy(xTrgt,error->message,maxlen-1);
              else
                _tcslcpy(xTrgt,"unchanged",maxlen-1);
            }
          }
          else if (xDirectory && xWant==EGeneralStatus)
          {
            _tcslcpy((TCHAR*)FieldValue,"GIT dir",maxlen-1);
          }
        }
        else //info o commitu
        {
          //EMsg,EAuthor,EMail,EDate,EAge
          //EThisCommit,ELastAffecting,ELastFallthrough

          if (xWantFrom==ELastAffecting && xFilesCommit==NULL)
          {
            xReturnEmpty=true;
          }
          else
          {
            git_commit * xPtrUse;
            if (xWantFrom==EThisCommit || (xWantFrom==ELastFallthrough && xFilesCommit==NULL))
              xPtrUse=commit;
            else
              xPtrUse=xFilesCommit;

            switch (xWant)
            {              //EMsg,EAuthor,EMail,EDate,EAge
              case EFallIsThis:
              {
                if (xPtrUse==commit)
                  _tcslcpy((TCHAR*)FieldValue,"This commit",maxlen-1);
                else
                  _tcslcpy((TCHAR*)FieldValue,"Last affecting",maxlen-1);
              }
              break;
              case EMsg: //git last commitmessage
              {
                char *xCMsg=(char*)git_commit_summary(xPtrUse);//whole message: git_commit_message
                _tcslcpy((TCHAR*)FieldValue,xCMsg,maxlen-1);
              }
              break;
              case EAuthor: //"CommitAuthor"
              case EMail:
              {
                const git_signature *xSig=git_commit_author(xPtrUse);
                if (xSig)
                {
                  switch (xWant)
                  {
                    case EAuthor:        //name
                      _tcslcpy((TCHAR*)FieldValue,xSig->name,maxlen-1);
                    break;
                    case EMail:       //email
                      _tcslcpy((TCHAR*)FieldValue,xSig->email,maxlen-1);
                    break;
                  }
                }
              }
              break;
              case EDate: //"CommitDatetime"
              {
                git_time_t comtim=git_commit_time(xPtrUse);
                int origoffset=git_commit_time_offset(xPtrUse);
                TimetToFileTime(comtim, ((FILETIME*)FieldValue));
              }
              break;
              case EAge: //"CommitAge"
              {
                git_time_t comtim=git_commit_time(xPtrUse);
                time_t xNow;
                time(&xNow);
                double xAgo=difftime(xNow, comtim);//difference in seconds
                PrintTimeAgo((TCHAR*)FieldValue,xAgo,maxlen);
              }
              break;
            }
          }
        }

        //cleanup:
        if (xFilesCommit)
          git_commit_free(xFilesCommit);
        if (commit)
          git_commit_free(commit);
        ByeRepo(repo);

        if (xReturnEmpty)
          return ft_fieldempty;
      }
      else                         //else neni repo nevracime nic.
        return ft_fieldempty;
    }
    else//operace mimo GIT.
    {
      switch (FieldIndex)
      {
        case EFSizeBranch:  // "size"  + SPECIAL FOR GIT
        {
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
              git_repository *repo = CheckRepo(fd,FileName,xRet,false,NULL);//false - will not slow down if needs to find repo
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
        }
        break;
        default:
          return ft_nosuchfield;
      }
		}
	} else
		return ft_fileerror;

	int retval=fieldtypes[FieldIndex];  // very important!
#ifdef UNICODE
	if (retval==ft_string) {
	/*	switch (FieldIndex) {
		case 0: // name
		case 9: // attrstr
		case 17:// versionstr
		case 21:// "CutNameStart"*/
			retval=ft_stringw;
	/*	}*/
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
	/*else {
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
	} */
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


/*
Orginal example for tcmd

case 2:  // "creationdate"
			FileTimeToLocalFileTime(&fd.ftCreationTime,&lt);
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
      *((FILETIME*)FieldValue)=lt;//lepsi, ma vic moznosti v novejch commanderech nastaveni formatovani.
			break;
		case 5:  // "writetime"
			FileTimeToLocalFileTime(&fd.ftLastWriteTime,&lt);
			FileTimeToSystemTime(&lt,&st);
			break;
		case 6:  // "accessdate"
			FileTimeToLocalFileTime(&fd.ftLastAccessTime,&lt);
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
		/*case 9:  // "attributestr",
			_tcslcpy((TCHAR*)FieldValue,TEXT("----"),maxlen-1);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ((TCHAR*)FieldValue)[0]='r';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)  ((TCHAR*)FieldValue)[1]='a';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)   ((TCHAR*)FieldValue)[2]='h';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)  ((TCHAR*)FieldValue)[3]='s';
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) _tcsncat((TCHAR*)FieldValue,TEXT("c"),maxlen-1);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) _tcsncat((TCHAR*)FieldValue,TEXT("e"),maxlen-1);
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


      */
