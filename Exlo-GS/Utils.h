#pragma once
#include "pch.h"

#define ANY_PACKAGE (UObject*)-1

template<class T>
static T* Cast(UObject* Object)
{
    return Object && (Object->IsA(T::StaticClass())) ? (T*)Object : NULL;
}

class Utils
{
public:
    template <class T>
    static inline T* SpawnActor(FVector Location = FVector(), FRotator Rotation = FRotator(0, 0, 0), UClass* InClass = T::StaticClass(), AActor* Owner = NULL)
    {
        FTransform Transform = FTransform{};

        Transform.Rotation = UKismetMathLibrary::Conv_RotatorToTransform(Rotation).Rotation;
        Transform.Translation = Location;
        Transform.Scale3D = { 1, 1, 1 };

        AActor* Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), InClass, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);

        if (Actor)
            UGameplayStatics::FinishSpawningActor(Actor, Transform);

        return (T*)Actor;
    }

    template<typename T = UObject>
    static T* StaticFindObject(const TCHAR* OrigInName, UObject* InObjectPackage = nullptr, UClass* ObjectClass = nullptr)
    {
        static UObject* (*_StaticFindObject)(UClass*, UObject*, const TCHAR*, bool) = decltype(_StaticFindObject)(InSDKUtils::GetImageBase() + 0x2001E30);
        return (T*)_StaticFindObject(ObjectClass, InObjectPackage, OrigInName, false);
    }

    template<typename T = UObject>
    static T* StaticLoadObject(const TCHAR* Path, UClass* InClass = T::StaticClass(), UObject* InOuter = nullptr)
    {
        static UObject* (*_StaticLoadObject)(UClass*, UObject*, const TCHAR*, const TCHAR*, uint32, UPackageMap*, bool) = decltype(_StaticLoadObject)(InSDKUtils::GetImageBase() + 0x2003110);
        return (T*)_StaticLoadObject(InClass, InOuter, Path, NULL, 0, NULL, false);
    }

    template<typename T>
    static T* GetInterfaceAddress(UObject* Object)
    {
        return (T*)(((void* (*)(SDK::UObject*, SDK::UClass*))(InSDKUtils::GetImageBase() + 0x1FF3650)) (Object, T::StaticClass()));
    }

    template<typename T>
    static T* NewObject(UObject* Object, UClass* Class = NULL)
    {
        return (T*)UGameplayStatics::SpawnObject(Class ? Class : T::StaticClass(), Object);
    }

    static void ShowFoundation(ABuildingFoundation* BuildingFoundation)
    {
        if (BuildingFoundation == NULL)
            return;

        BuildingFoundation->DynamicFoundationType = EDynamicFoundationType::Static;
        BuildingFoundation->FoundationEnabledState = EDynamicFoundationEnabledState::Enabled;

        BuildingFoundation->SetDynamicFoundationEnabled(true);

        BuildingFoundation->bServerStreamedInLevel = true;
        BuildingFoundation->OnRep_ServerStreamedInLevel();
    }

    template <typename T = void*>
    static void Exec(const TCHAR* Name, void* Detour, T* Original = NULL) {
        UFunction* Func = StaticFindObject<UFunction>(Name);

        if (Func == NULL)
            return;

        if (Original)
            *Original = reinterpret_cast<T>(Func->ExecFunction);

        Func->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(Detour);
    }
        
    static void Hook(uintptr_t Target, void* Detour, void** Original = nullptr)
    {
        MH_CreateHook((LPVOID)Target, Detour, Original);
        MH_EnableHook((LPVOID)Target);
    }

    template<typename Class>
    static void Virtual(uintptr_t Target, void* Detour, void** Original = nullptr)
    {
        UObject* Object = Class::GetDefaultObj();

        if (Object == NULL)
            return;

        if (Original)
            *Original = Object->VTable[(int)Target];

        DWORD dwProt;
        VirtualProtect(&Object->VTable[(int)Target], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProt);

        Object->VTable[(int)Target] = Detour;

        DWORD dwTemp;
        VirtualProtect(&Object->VTable[(int)Target], sizeof(void*), dwProt, &dwTemp);
    }

    template<typename Class>
    static void VirtualEvery(uintptr_t Target, void* Detour, void** Original = nullptr)
    {
        UObject* Object = Class::GetDefaultObj();

        if (Object == NULL)
            return;

        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            UObject* Obj = UObject::GObjects->GetByIndex(i);

            if (Obj)
            {
                if (Obj->IsA(Object->Class))
                {
                    if (Original)
                        *Original = Obj->VTable[(int)Target];

                    DWORD dwProt;
                    VirtualProtect(&Obj->VTable[(int)Target], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProt);

                    Obj->VTable[(int)Target] = Detour;

                    DWORD dwTemp;
                    VirtualProtect(&Obj->VTable[(int)Target], sizeof(void*), dwProt, &dwTemp);
                }
            }
        }
    }

    static void Patch(uintptr_t Target, uintptr_t Byte)
    {
        DWORD OldProtect;

        unsigned char v = Byte;
        VirtualProtect(reinterpret_cast<LPVOID>(Target), 1, PAGE_EXECUTE_READWRITE, &OldProtect);

        *reinterpret_cast<unsigned char*>(Target) = v;
        VirtualProtect(reinterpret_cast<LPVOID>(Target), 1, OldProtect, &OldProtect);
    }

    static uint8_t* AllocateNearbyPage(void* targetAddr)
    {
        SYSTEM_INFO SysInfo;

        GetSystemInfo(&SysInfo);

        const uint64_t PageSize = SysInfo.dwPageSize;
        const uint64_t StartAddr = (uint64_t(targetAddr) & ~(PageSize - 1));
        const uint64_t MinAddr = min(StartAddr - 0x7FFFFF00, (uint64_t)SysInfo.lpMinimumApplicationAddress);
        const uint64_t MaxAddr = max(StartAddr + 0x7FFFFF00, (uint64_t)SysInfo.lpMaximumApplicationAddress);
        const uint64_t StartPage = (StartAddr - (StartAddr % PageSize));

        for (uint64_t PageOffset = 1; PageOffset; PageOffset++)
        {
            uint64_t ByteOffset = PageOffset * PageSize;
            uint64_t HighAddr = StartPage + ByteOffset;
            uint64_t LowAddr = (StartPage > ByteOffset) ? StartPage - ByteOffset : 0;

            bool NeedsExit = HighAddr > MaxAddr && LowAddr < MinAddr;

            if (HighAddr < MaxAddr)
            {
                if (void* OutAddr = VirtualAlloc((void*)HighAddr, PageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
                {
                    return (uint8_t*)OutAddr;
                }
            }

            if (LowAddr > MinAddr)
            {
                if (void* OutAddr = VirtualAlloc((void*)LowAddr, PageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
                {
                    return (uint8_t*)OutAddr;
                }
            }

            if (NeedsExit)
            {
                break;
            }
        }

        return nullptr;
    }

    static void Rel32(uintptr_t Target, void* Detour, void** Original = nullptr)
    {
        auto Impl = (uint8*)(Target);

        auto NearPage = AllocateNearbyPage(Impl);

        if (!NearPage)
        {
            return;
        }

        uint8_t Shellcode[] =
        {
            0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        if (Detour != NULL)
        {
            memcpy(Shellcode + 6, &Detour, 8);
            memcpy(NearPage, Shellcode, sizeof(Shellcode));

            auto Offset = NearPage - (Impl + (int)5);

            memcpy(Impl + 1, &Offset, sizeof(int));
        }
        else
        {
            memset(Impl, 0x90, sizeof(int) + 1);
        }
    }
};

class FOutputDevice
{
public:
    void** VTable;
    bool bSuppressEventTag;
    bool bAutoEmitLineTerminator;
};

class FFrame : public FOutputDevice
{
public:
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    void* MostRecentProperty;
    uint8_t* MostRecentPropertyAddress;
    uint8_t _Padding1[0x40];
    const UField* PropertyChainForCompiledIn;

public:
    inline void StepCompiledIn(void* const Result = nullptr)
    {
        if (Code)
        {
            static void(*StepCompiledInCode)(FFrame*, UObject*, void* const) = decltype(StepCompiledInCode)(InSDKUtils::GetImageBase() + 0x20037A0);
            StepCompiledInCode(this, Object, Result);
        }
        else
        {
            const UField* _Prop = *(const UField**)(__int64(this) + 0x80);

            if (_Prop)
            {
                *(const UField**)(__int64(this) + 0x80) = *(const UField**)(__int64(_Prop) + 0x28);

                static void(*StepExplicitProperty)(FFrame*, void* const, const UField*) = decltype(StepExplicitProperty)(InSDKUtils::GetImageBase() + 0x20037D0);
                StepExplicitProperty(this, Result, _Prop);
            }
        }
    }

    __forceinline void IncrementCode()
    {
        Code += !!Code;
    }
};