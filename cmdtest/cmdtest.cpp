// ---------------------------------------------------------------------------
// pada trebas na dfirst.dfm
#pragma hdrstop

#include <tchar.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream.h>
#include <algorithm>

#define _BCB

// spec defines:
#ifdef _BCB
// #define _BUILD_DLL_
// GIT:      normally we should do this
// #include "include/git2.h"
// but we include GitForBCB, things ripped from libgit2:
#include "uGitForBCB.h"
#include "time.h"
#include <sys/stat.h>
#endif

#include <conio.h>

// ---------------------------------------------------------------------------
void path_strip_filename(TCHAR *Path)
{
  size_t Len = _tcslen(Path);
  if (Len == 0)
  {
    return;
  };
  size_t Idx = Len - 1;
  while (TRUE)
  {
    TCHAR Chr = Path[Idx];
    if (Chr == TEXT('\\') || Chr == TEXT('/'))
    {
      if (Idx == 0 || Path[Idx - 1] == ':')
      {
        Idx++;
      };
      break;
    }
    else if (Chr == TEXT(':'))
    {
      Idx++;
      break;
    }
    else
    {
      if (Idx == 0)
      {
        break;
      }
      else
      {
        Idx--;
      };
    };
  };
  Path[Idx] = TEXT('\0');
};

bool dirExists(const char *path)
{
  struct stat info;

  if (stat(path, &info) != 0)
    return false;
  else if (info.st_mode & S_IFDIR)
    return true;
  else
    return false;
}

// git and caching repos (to come)
enum EPositionType
{
  EPTNothing, // not git;
    EPTGitOK,
  // The viewed directory has .git subfolder (filename is a directory)
    EPTGitDirect,
  // this is the .git folder                 (filename is a directory)
    EPTInside
}; // this folder is some folder inside current repository

git_repository *CheckRepo(WIN32_FIND_DATA fd, TCHAR* FileName,
  EPositionType &pRet, bool pDoSearch, TCHAR **pRelFilePath)
{
  char xTemp[2048];
  strcpy(xTemp, FileName);
  pRet = EPTNothing;
  if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    // for files - check the directory we are in
  {
    // strcpy(xTemp,FileName);
    path_strip_filename(xTemp);
  }
  int xOriglen = strlen(xTemp);

  git_repository *repo = NULL;
  if (strstr(xTemp, "\\.git") == xTemp + xOriglen - 5)
    // now we are INSIDE a repo and this is exactly the information for the invisible git dir
  {
    pRet = EPTGitDirect;
    if (0 != git_repository_open(&repo, xTemp))
      repo = NULL;
  }
  else
  {
    strcat(xTemp, "\\.git");
    if (dirExists(xTemp)) // we are looking at the repo
    {
      pRet = EPTGitOK;
      if (0 != git_repository_open(&repo, xTemp))
        repo = NULL;
    }
    // else let git find it.... ..
    else if (pDoSearch)
    {
      xTemp[xOriglen] = 0;
      if (0 != git_repository_open_ext(&repo, xTemp, 0, NULL))
        repo = NULL;
      else
        pRet = EPTInside;
    }
  }
  if (pRelFilePath)
    // we want to know the relative path in git repo of this file.
  {
    if (repo && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      const char *xRepdir = git_repository_path(repo);
      // returns different than windows slashes!!!
      int i;
      for (i = 0; i < sizeof(xTemp); i++)
      {
        if (FileName[i] == 0 || xRepdir == 0 ||
          (tolower(FileName[i]) != tolower(xRepdir[i]) &&
          ((FileName[i] != '\\' || xRepdir[i] != '/') && (FileName[i] != '/' ||
          xRepdir[i] != '\\'))))
          break;
      }
      (*pRelFilePath) = FileName + i;
    }
    else
      (*pRelFilePath) = NULL;
  }
  return repo;
}

void ByeRepo(git_repository *pBye)
{
  git_repository_free(pBye);
}

// end git
TCHAR* _tcslcpy(TCHAR* p, const TCHAR* p2, int maxlen)
{
  if ((int)_tcslen(p2) >= maxlen)
  {
    _tcsncpy(p, p2, maxlen);
    p[maxlen] = 0;
  }
  else
    _tcscpy(p, p2);
  return p;
}

void PrintTimeAgo(TCHAR *pTarget, double pAgo, unsigned int pMaxLen)
{
#define NUMFMTSAGE 6
  int xDifs[NUMFMTSAGE] =
  {1, 60, 3600, 24 * 3600, 2629743, 31556926
  }; // seconds,minutes,hours,days,approxmonths,years
  char *xNam[NUMFMTSAGE] =
  {"secs.", "mins", "hrs.", "days", "months", "years"};
  int iDo = 0;
  while (iDo < NUMFMTSAGE)
  {
    if (pAgo / double(xDifs[iDo]) < 1.0)
      break;
    iDo++;
  }
  if (iDo >= 1)//the prev one
    iDo--;
  int xRes = int(0.5 + pAgo / xDifs[iDo]);
  char msg[512];
  sprintf(msg, "%d %s", xRes, xNam[iDo]);
  _tcslcpy((TCHAR*)pTarget, msg, pMaxLen - 1);
}

#pragma argsused

int _tmain(int argc, _TCHAR* argv[])
{
  EPositionType xRet;
  char *xPtrFN = NULL;
  TCHAR FieldValue[2048];
  int maxlen = 2048;
  // char FileName[]="d:\\Data\\Projects\\WinMedicalc\\WMPacs\\WMPacsViewer\\dFirst.cpp";
  char FileName[] =
    "d:\\Data\\Projects\\repoempty\\ictamxs.sty";
  char FileName2[] = "ictamxs.sty";
  WIN32_FIND_DATA fd;
  fd.dwFileAttributes = 0; // FILE_ATTRIBUTE_DIRECTORY;
  git_repository *repo = CheckRepo(fd, FileName, xRet, true, &xPtrFN);
  xPtrFN = FileName2;
  if (repo && xPtrFN)
  {
    // debug:
    /* for (int ii=0;ii<strlen(xPtrFN);ii++) //HERE we modify the original FileName, because xPtrFN points there. But we know it, ok.
     {
     if (xPtrFN[ii]=='\\') xPtrFN[ii]='/';//for git
     } */
    /* for (int ii=strlen(xPtrFN)-1;ii>=0;ii--) //only filename:
     {
     if (xPtrFN[ii]=='\\' || xPtrFN[ii]=='/')
     {
     xPtrFN=xPtrFN+ii+1;
     break;
     }
     } */

    git_strarray xRs;
    unsigned int xStatF = 0;
    if (0 == git_status_file(&xStatF, repo, xPtrFN))
      // +strlen(git_repository_path(repo))
    {
      if (xStatF & GIT_STATUS_IGNORED)
        printf("ignored");
      else if (xStatF & GIT_STATUS_WT_NEW)
        printf("new");
      else if (xStatF & GIT_STATUS_WT_MODIFIED)
        printf("modified");
      else if (xStatF & GIT_STATUS_WT_DELETED)
        printf("deleted");
      else if (xStatF & GIT_STATUS_WT_TYPECHANGE)
        printf("typechange");
      else if (xStatF & GIT_STATUS_WT_RENAMED)
        printf("renamed");
      else if (xStatF & GIT_STATUS_WT_UNREADABLE)
        printf("unreadable");
    }
    if (xStatF == 0) // nothing printed or error
        printf(xPtrFN);
    printf("\n");
    const git_error *error = giterr_last();
    if (error)
      printf(error->message);

    // get commit and commit message
    git_commit * commit = NULL; /* the result */
    git_oid oid_parent_commit; /* the SHA1 for last commit */
    /* resolve HEAD into a SHA1 */
    int rc = git_reference_name_to_id(&oid_parent_commit, repo, "HEAD");
    if (rc == 0)
    {
      /* get the actual commit structure */
      rc = git_commit_lookup(&commit, repo, &oid_parent_commit);
      if (rc == 0)
      {
        // we have the last commit. Now to walk it:
        git_oid xLastAffecting;
        git_commit *xFilesCommit = NULL;
        if (0 == git_commit_entry_last_commit_id(&xLastAffecting, repo, commit,FileName2))
        {
          rc = git_commit_lookup(&xFilesCommit, repo, &xLastAffecting);
          if (rc == 0)
          {

            git_time_t comtim = git_commit_time(xFilesCommit);
            time_t xNow;
            time(&xNow);
            double xAgo = difftime(xNow, comtim); // difference in seconds
            PrintTimeAgo((TCHAR*)FieldValue, xAgo, maxlen);

            git_commit_free(xFilesCommit);
          }
        }
        git_commit_free(commit);
      }
    }

    // _tcslcpy((TCHAR*)FieldValue,git_repository_path(repo),maxlen-1);//debug del.
    ByeRepo(repo);
  }
  // printf("%s",FieldValue);

  getch();

  return 0;
}
// ---------------------------------------------------------------------------
