#include "pch.h"
#include "additionaltxd.h"
#include <CTxdStore.h>
#include <CStreaming.h>
#include <DynAddress.h>

using f_RwTexDictionaryFindNamedTexture = RwTexture *(*__cdecl)(RwTexDictionary *dict, const char *name);
using f_AssignRemapTxd = int (*__cdecl)(const char *txdName, __int16 txdID);
f_RwTexDictionaryFindNamedTexture ogFindNamedTex;
f_AssignRemapTxd ogAssignRemapTex;

RwTexture *CAdditionalTXD::hkRwTexDictionaryFindNamedTexture(RwTexDictionary *dict, const char *name)
{
    RwTexture *pTex = ogFindNamedTex(dict, name);
    if (!pTex && bAdditionalTxdUsed)
    {
        for (auto &e : TxdDictStore)
        {
            pTex = RwTexDictionaryFindNamedTexture(e, name);
            if (pTex)
            {
                break;
            }
        }
    }

    return pTex;
}

void CAdditionalTXD::hkAssignRemapTxd(const char *txdName, uint16_t txdId)
{
    if (!txdName)
    {
        return;
    }

    size_t len = strlen(txdName);
    if (strlen(txdName) > 10 && isdigit(txdName[len - 1]) && !strncmp(txdName, "fastloader", 10))
    {
        TxdIDStore.push_back(txdId);
        CTxdStore::AddRef(txdId);
        bAdditionalTxdUsed = true;
    }
    else
    {
        ogAssignRemapTex(txdName, txdId);
    }
}

void CAdditionalTXD::Load()
{
    if (bAdditionalTxdUsed)
    {
        for (auto &e : TxdIDStore)
        {
            CStreaming::RequestTxdModel(e, (eStreamingFlags::PRIORITY_REQUEST | eStreamingFlags::KEEP_IN_MEMORY));
        }

        CStreaming::LoadAllRequestedModels(true);

        for (auto &e : TxdIDStore)
        {
            auto *dict = ((RwTexDictionary * (__cdecl *)(int))0x408340)(e); // size_t __cdecl getTexDictionary(int txdIndex)
            TxdDictStore.push_back(dict);
        }
    }
}

void CAdditionalTXD::Init()
{
    plugin::Events::initRwEvent.after += []()
    {
        ogFindNamedTex = (f_RwTexDictionaryFindNamedTexture)plugin::patch::GetPointer(0x731733 + 1);
        ogFindNamedTex = f_RwTexDictionaryFindNamedTexture((int)ogFindNamedTex + (plugin::GetGlobalAddress(0x731733) + 5));
        plugin::patch::RedirectCall(0x731733, hkRwTexDictionaryFindNamedTexture, true);

        ogAssignRemapTex = (f_AssignRemapTxd)plugin::patch::GetPointer(0x5B62C2 + 1);
        ogAssignRemapTex = f_AssignRemapTxd((int)ogAssignRemapTex + (plugin::GetGlobalAddress(0x5B62C2) + 5));
        plugin::patch::RedirectCall(0x5B62C2, hkAssignRemapTxd, true);
    };

    plugin::Events::initScriptsEvent += []()
    {
        Load();
    };
}