# WDX_GitCommander
Git plugin for Total Commander!
(WDX, that means column information and mouseover hints over a repository or a file in repository)

Dear guys! This project is aimed to try to compile libgit2 using C++Builder XE and making a plugin for total commander.
It should work without git backend and without the git tools in the path (thats why i compiled libgit anyway). 

To use the plugin - download Release.zip and set it up in total commander.
Yes it might not be officially noticed by TCmd yet (and it will show an ugly message possibly about missing dll which I dont understand yet) and thats why there is the plugin manager in the file also (thx guys!), use it to install it.

The easiest way to use is to create your own columns, I suggest just replacing the original SIZE solumn with the column "SizeAndBranch".
Because that will show the branch instead of <DIR> for directories.

For like-github functionality in total commander, there is the possibility to create plugin defined hints in total commander.
Just go to Configuration->Display and down there on the setting page tick some boxes with "hint texts" to make visible the small button with "+".
Then it just remains to add a rule for all files "*" with this formatting:

Option a) last commit info:
```
[=gitcmd.Branch]([=gitcmd.CommitAge]) [=gitcmd.FirstRemoteUrl]\n[=gitcmd.CommitMessage]\n[=gitcmd.CommitAuthor] [=gitcmd.CommitMail] [=gitcmd.CommitDate.D.M.Y h:m:s]
```

Option b) Last commit affecting given file (or last commit info for folders)
```
[=gitcmd.Branch] [=gitcmd.FirstRemoteUrl]\n[=gitcmd.FallIsLast] [=gitcmd.FallAge]\n[=gitcmd.FallMessage]\n[=gitcmd.FallAuthor] [=gitcmd.FallMail] [=gitcmd.FallDate.D.M.Y h:m:s]\n[=gitcmd.GeneralStatus]
```


In the future I would like to make it with better installators and so. If you love total commander, feel free to do it faster than me.

Martin.

PS: Thread on the totalcommander site: http://www.ghisler.ch/board/viewtopic.php?t=42074
