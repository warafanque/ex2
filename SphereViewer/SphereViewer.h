#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CSphereViewerApp : public CWinApp {
public:
    CSphereViewerApp() noexcept;

public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CSphereViewerApp theApp;
