#include "accountsworker.h"

const QString AccountsService("com.deepin.daemon.Accounts");

AccountsWorker::AccountsWorker(UserModel *userList, QObject *parent)
    : QObject(parent),
      m_accountsInter(new Accounts(AccountsService, "/com/deepin/daemon/Accounts", QDBusConnection::systemBus(), this)),
      m_userList(userList)
{
    connect(m_accountsInter, &Accounts::UserListChanged, this, &AccountsWorker::onUserListChanged);

    m_accountsInter->setSync(false);
    m_accountsInter->getAllProperties();
}

void AccountsWorker::onUserListChanged(const QStringList &userList)
{
    for (const QString &userPath : userList)
    {
        if (m_userInters.values().contains(userPath))
            continue;

        AccountsUser *userInter = new AccountsUser(AccountsService, userPath, QDBusConnection::systemBus(), this);
        userInter->setSync(false);

        m_userInters.insert(userInter, userPath);

        User * user = new User(m_userList);
        user->setName(userInter->userName());
        connect(userInter, &AccountsUser::UserNameChanged, user, &User::setName);

        m_userList->addUser(userPath, user);
    }
}
