#include "pch.h"
#include "framework.h"
#include "SphereViewer.h"
#include "SphereViewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSphereViewerDlg::CSphereViewerDlg(CWnd* pParent)
    : CDialog(IDD_SPHEREVIEWER_DIALOG, pParent),
      m_bAnimating(false),
      m_nTimer(0),
      m_bUseHiddenSurfaceRemoval(true) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSphereViewerDlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSphereViewerDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_ANIMATE, &CSphereViewerDlg::OnBnClickedButtonAnimate)
    ON_BN_CLICKED(IDC_BUTTON_SAVE, &CSphereViewerDlg::OnBnClickedButtonSave)
    ON_BN_CLICKED(IDC_BUTTON_LOAD, &CSphereViewerDlg::OnBnClickedButtonLoad)
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

BOOL CSphereViewerDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // Initialize sphere with geographic subdivision
    // radius = 1.0, 20 latitudes, 24 longitudes
    m_sphere.Initialize(1.0, 20, 24);

    // Get render area
    CWnd* pRenderWnd = GetDlgItem(IDC_STATIC_RENDER);
    if (pRenderWnd) {
        pRenderWnd->GetClientRect(&m_renderRect);
    }

    // Set button text
    CButton* pButton = (CButton*)GetDlgItem(IDC_BUTTON_ANIMATE);
    if (pButton) {
        pButton->SetWindowText(_T("播放动画"));
    }

    pButton = (CButton*)GetDlgItem(IDC_BUTTON_SAVE);
    if (pButton) {
        pButton->SetWindowText(_T("保存"));
    }

    pButton = (CButton*)GetDlgItem(IDC_BUTTON_LOAD);
    if (pButton) {
        pButton->SetWindowText(_T("加载"));
    }

    return TRUE;
}

void CSphereViewerDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CPaintDC dc(this);
        
        // Get render control
        CWnd* pRenderWnd = GetDlgItem(IDC_STATIC_RENDER);
        if (pRenderWnd) {
            CDC* pDC = pRenderWnd->GetDC();
            CRect rect;
            pRenderWnd->GetClientRect(&rect);
            
            // Create memory DC for double buffering
            CDC memDC;
            memDC.CreateCompatibleDC(pDC);
            CBitmap memBitmap;
            memBitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
            CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);
            
            // Clear background
            memDC.FillSolidRect(&rect, RGB(255, 255, 255));
            
            // Draw coordinate system
            DrawCoordinateSystem(&memDC);
            
            // Draw sphere
            m_sphere.Draw(&memDC, rect, m_bUseHiddenSurfaceRemoval);
            
            // Copy to screen
            pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
            
            memDC.SelectObject(pOldBitmap);
            pRenderWnd->ReleaseDC(pDC);
        }
        
        CDialog::OnPaint();
    }
}

HCURSOR CSphereViewerDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

void CSphereViewerDlg::DrawCoordinateSystem(CDC* pDC) {
    CWnd* pRenderWnd = GetDlgItem(IDC_STATIC_RENDER);
    if (!pRenderWnd) return;
    
    CRect rect;
    pRenderWnd->GetClientRect(&rect);
    
    int centerX = rect.Width() / 2;
    int centerY = rect.Height() / 2;
    int axisLength = min(centerX, centerY) / 4;
    
    // Draw X axis (red, horizontal right)
    CPen penX(PS_SOLID, 2, RGB(255, 0, 0));
    CPen* pOldPen = pDC->SelectObject(&penX);
    pDC->MoveTo(centerX, centerY);
    pDC->LineTo(centerX + axisLength, centerY);
    pDC->TextOutW(centerX + axisLength + 5, centerY - 10, _T("X"));
    
    // Draw Y axis (green, vertical up)
    CPen penY(PS_SOLID, 2, RGB(0, 255, 0));
    pDC->SelectObject(&penY);
    pDC->MoveTo(centerX, centerY);
    pDC->LineTo(centerX, centerY - axisLength);
    pDC->TextOutW(centerX + 5, centerY - axisLength - 15, _T("Y"));
    
    // Draw Z axis (blue, toward viewer - diagonal)
    CPen penZ(PS_SOLID, 2, RGB(0, 0, 255));
    pDC->SelectObject(&penZ);
    pDC->MoveTo(centerX, centerY);
    pDC->LineTo(centerX - axisLength / 2, centerY + axisLength / 2);
    pDC->TextOutW(centerX - axisLength / 2 - 15, centerY + axisLength / 2 + 5, _T("Z"));
    
    pDC->SelectObject(pOldPen);
}

void CSphereViewerDlg::OnBnClickedButtonAnimate() {
    m_bAnimating = !m_bAnimating;
    
    CButton* pButton = (CButton*)GetDlgItem(IDC_BUTTON_ANIMATE);
    if (m_bAnimating) {
        // Start animation
        m_nTimer = SetTimer(1, 50, nullptr); // 50ms = 20 FPS
        if (pButton) {
            pButton->SetWindowText(_T("停止动画"));
        }
    } else {
        // Stop animation
        if (m_nTimer) {
            KillTimer(m_nTimer);
            m_nTimer = 0;
        }
        if (pButton) {
            pButton->SetWindowText(_T("播放动画"));
        }
    }
}

void CSphereViewerDlg::OnBnClickedButtonSave() {
    CFileDialog dlg(FALSE, _T("txt"), _T("sphere.txt"),
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
    
    if (dlg.DoModal() == IDOK) {
        CString filename = dlg.GetPathName();
        if (m_sphere.SaveToFile(filename)) {
            MessageBox(_T("球体数据保存成功！"), _T("保存"), MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(_T("球体数据保存失败！"), _T("错误"), MB_OK | MB_ICONERROR);
        }
    }
}

void CSphereViewerDlg::OnBnClickedButtonLoad() {
    CFileDialog dlg(TRUE, _T("txt"), _T("sphere.txt"),
                    OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                    _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));
    
    if (dlg.DoModal() == IDOK) {
        CString filename = dlg.GetPathName();
        if (m_sphere.LoadFromFile(filename)) {
            MessageBox(_T("球体数据加载成功！"), _T("加载"), MB_OK | MB_ICONINFORMATION);
            UpdateSphere();
        } else {
            MessageBox(_T("球体数据加载失败！"), _T("错误"), MB_OK | MB_ICONERROR);
        }
    }
}

void CSphereViewerDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1 && m_bAnimating) {
        // Rotate sphere automatically
        m_sphere.RotateY(0.05);
        UpdateSphere();
    }
    
    CDialog::OnTimer(nIDEvent);
}

BOOL CSphereViewerDlg::OnEraseBkgnd(CDC* pDC) {
    return TRUE; // Prevent flicker
}

void CSphereViewerDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
    const double rotateAmount = 0.1;
    
    switch (nChar) {
        case VK_LEFT:
            m_sphere.RotateY(-rotateAmount);
            UpdateSphere();
            break;
        case VK_RIGHT:
            m_sphere.RotateY(rotateAmount);
            UpdateSphere();
            break;
        case VK_UP:
            m_sphere.RotateX(-rotateAmount);
            UpdateSphere();
            break;
        case VK_DOWN:
            m_sphere.RotateX(rotateAmount);
            UpdateSphere();
            break;
        case VK_PRIOR: // Page Up
            m_sphere.RotateZ(rotateAmount);
            UpdateSphere();
            break;
        case VK_NEXT: // Page Down
            m_sphere.RotateZ(-rotateAmount);
            UpdateSphere();
            break;
    }
    
    CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSphereViewerDlg::UpdateSphere() {
    CWnd* pRenderWnd = GetDlgItem(IDC_STATIC_RENDER);
    if (pRenderWnd) {
        pRenderWnd->Invalidate(FALSE);
    }
}

BOOL CSphereViewerDlg::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_KEYDOWN) {
        OnKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
        return TRUE;
    }
    return CDialog::PreTranslateMessage(pMsg);
}
