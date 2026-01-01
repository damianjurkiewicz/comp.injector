#pragma once
#include <vector>

// Taken from VehFuncs
class CAdditionalTXD
{
private:
    static inline std::vector<int> TxdIDStore;
    static inline std::vector<RwTexDictionary *> TxdDictStore;

    static inline bool bAdditionalTxdUsed = false;

    static void hkAssignRemapTxd(const char *txdName, uint16_t txdId);
    static RwTexture *__cdecl hkRwTexDictionaryFindNamedTexture(RwTexDictionary *dict, const char *name);

    static void Load();

public:
    static void Init();
};

extern CAdditionalTXD AdditionalTXD;