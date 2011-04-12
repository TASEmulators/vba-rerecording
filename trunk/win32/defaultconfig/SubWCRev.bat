if exist "userconfig\svnrev.h" goto :SkipCopy
copy /Y "defaultconfig\svnrev.h" "userconfig\svnrev.h"

:SkipCopy
defaultconfig\SubWCRev.exe .. ".\defaultconfig\svnrev_template.h" ".\userconfig\svnrev.h"
exit 0
