#pragma once
#include "Sphere3D.h"

class CSphereViewerDlg : public CDialog {
public:
    CSphereViewerDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_SPHEREVIEWER_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonAnimate();
    afx_msg void OnBnClickedButtonSave();
    afx_msg void OnBnClickedButtonLoad();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    
    DECLARE_MESSAGE_MAP()

private:
    HICON m_hIcon;
    Sphere3D m_sphere;
    bool m_bAnimating;
    UINT_PTR m_nTimer;
    bool m_bUseHiddenSurfaceRemoval;
    CRect m_renderRect;
    
    void DrawCoordinateSystem(CDC* pDC);
    void UpdateSphere();
};
