#pragma once

#include "CSession.h"
#include <memory>

class OfflineSyncService
{
public:
    static void syncAfterLogin(std::shared_ptr<CSession> session, int uid);
};
