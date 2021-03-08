#include <Windows.h>
#include <iostream>


#include "IdaQuickLaunch.h"
#include "registry.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    IdaQuickLaunch q{};
    
    try {
        q.ProcessCommandLine(lpCmdLine);
    }
    catch (BadCmdArgsException) {
        q.DisplayMsgBoxUsages();
    }
    catch (WinApiException e) {
       q.DisplayMsgBox(e.msg + L" error: " + e.formated());
    }
    catch (Exception e) {
        q.DisplayMsgBox(e.what());
    }
}


