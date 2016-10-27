#ifndef ACCOUNTSMODULE_H
#define ACCOUNTSMODULE_H

#include "moduleinterface.h"
#include "accountswidget.h"
#include "accountsworker.h"
#include "user.h"
#include "usermodel.h"

#include <com_deepin_daemon_accounts.h>
#include <com_deepin_daemon_accounts_accountsuser.h>

class AccountsModule : public QObject, public ModuleInterface
{
    Q_OBJECT

public:
    explicit AccountsModule(FrameProxyInterface *frame, QObject *parent = 0);

    void initialize();
    void moduleActive();
    void moduleDeactive();

    ModuleWidget *moduleWidget();

    const QString name() const;

private:
    void showAccountsDetail();
    void contentPopped(ContentWidget * const w);

private:
    UserModel *m_userList;

    AccountsWidget *m_accountsWidget;
    AccountsWorker *m_accountsWorker;

    ContentWidget *m_accountsDetail;
};

#endif // ACCOUNTSMODULE_H
