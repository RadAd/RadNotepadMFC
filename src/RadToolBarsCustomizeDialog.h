#pragma once

class CRadToolBarsCustomizeDialog : public CMFCToolBarsCustomizeDialog
{
public:
    using CMFCToolBarsCustomizeDialog::CMFCToolBarsCustomizeDialog;

    static void CreateDefaultTools();

protected:
    virtual void OnInitToolsPage();
    virtual void OnBeforeChangeTool(CUserTool* pSelTool);
    virtual void OnAfterChangeTool(CUserTool* pSelTool);
    virtual BOOL CheckToolsValidity(const CObList& /*lstTools*/) { return TRUE; }

private:
    CButton btnCapture;
    CEdit   editTab;
    CButton btnSave;
};
