// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/EpicUnrealMCPNiagaraCommands.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraActor.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSimulationStageBase.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterStore.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "EngineUtils.h"

FEpicUnrealMCPNiagaraCommands::FEpicUnrealMCPNiagaraCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("spawn_niagara_system"))
    {
        return HandleSpawnNiagaraSystem(Params);
    }
    else if (CommandType == TEXT("spawn_niagara_system_attached"))
    {
        return HandleSpawnNiagaraSystemAttached(Params);
    }
    else if (CommandType == TEXT("get_niagara_systems"))
    {
        return HandleGetNiagaraSystems(Params);
    }
    else if (CommandType == TEXT("set_niagara_float_parameter"))
    {
        return HandleSetNiagaraFloatParameter(Params);
    }
    else if (CommandType == TEXT("set_niagara_vector_parameter"))
    {
        return HandleSetNiagaraVectorParameter(Params);
    }
    else if (CommandType == TEXT("set_niagara_color_parameter"))
    {
        return HandleSetNiagaraColorParameter(Params);
    }
    else if (CommandType == TEXT("set_niagara_bool_parameter"))
    {
        return HandleSetNiagaraBoolParameter(Params);
    }
    else if (CommandType == TEXT("set_niagara_int_parameter"))
    {
        return HandleSetNiagaraIntParameter(Params);
    }
    else if (CommandType == TEXT("set_niagara_texture_parameter"))
    {
        return HandleSetNiagaraTextureParameter(Params);
    }
    else if (CommandType == TEXT("get_niagara_parameters"))
    {
        return HandleGetNiagaraParameters(Params);
    }
    else if (CommandType == TEXT("activate_niagara_system"))
    {
        return HandleActivateNiagaraSystem(Params);
    }
    else if (CommandType == TEXT("deactivate_niagara_system"))
    {
        return HandleDeactivateNiagaraSystem(Params);
    }
    else if (CommandType == TEXT("destroy_niagara_system"))
    {
        return HandleDestroyNiagaraSystem(Params);
    }
    else if (CommandType == TEXT("get_niagara_assets"))
    {
        return HandleGetNiagaraAssets(Params);
    }
    else if (CommandType == TEXT("get_niagara_asset_details"))
    {
        return HandleGetNiagaraAssetDetails(Params);
    }
    else
    {
        TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Unknown Niagara command: %s"), *CommandType));
        return Result;
    }
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSpawnNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    // Get parameters
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: asset_path");
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* LocationJsonValueArray;
    if (!Params->TryGetArrayField(TEXT("location"), LocationJsonValueArray) || LocationJsonValueArray->Num() != 3)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing or invalid parameter: location (expected [x, y, z])");
        return Result;
    }
    FVector Location((*LocationJsonValueArray)[0]->AsNumber(), (*LocationJsonValueArray)[1]->AsNumber(), (*LocationJsonValueArray)[2]->AsNumber());
    
    // Optional rotation (default: zero)
    FRotator Rotation = FRotator::ZeroRotator;
    const TArray<TSharedPtr<FJsonValue>>* RotationJsonValueArray;
    if (Params->TryGetArrayField(TEXT("rotation"), RotationJsonValueArray) && RotationJsonValueArray->Num() == 3)
    {
        Rotation = FRotator((*RotationJsonValueArray)[0]->AsNumber(), (*RotationJsonValueArray)[1]->AsNumber(), (*RotationJsonValueArray)[2]->AsNumber());
    }
    
    // Optional scale (default: 1,1,1)
    FVector Scale(1.0f);
    const TArray<TSharedPtr<FJsonValue>>* ScaleJsonValueArray;
    if (Params->TryGetArrayField(TEXT("scale"), ScaleJsonValueArray) && ScaleJsonValueArray->Num() == 3)
    {
        Scale = FVector((*ScaleJsonValueArray)[0]->AsNumber(), (*ScaleJsonValueArray)[1]->AsNumber(), (*ScaleJsonValueArray)[2]->AsNumber());
    }
    
    // Optional auto_destroy (default: true)
    bool bAutoDestroy = true;
    Params->TryGetBoolField(TEXT("auto_destroy"), bAutoDestroy);
    
    // Optional auto_activate (default: true)
    bool bAutoActivate = true;
    Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate);
    
    // Optional name for the actor
    FString ActorName;
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    
    // Load Niagara system asset
    UNiagaraSystem* NiagaraSystem = LoadNiagaraSystemAsset(AssetPath);
    if (!NiagaraSystem)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
        return Result;
    }
    
    // Get world
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "No world available");
        return Result;
    }
    
    // Spawn Niagara system at location
    UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World,
        NiagaraSystem,
        Location,
        Rotation,
        Scale,
        bAutoDestroy,
        bAutoActivate
    );
    
    if (!NiagaraComponent)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Failed to spawn Niagara system");
        return Result;
    }
    
    // Set actor name if specified
    if (!ActorName.IsEmpty() && NiagaraComponent->GetOwner())
    {
        NiagaraComponent->GetOwner()->Rename(*ActorName);
    }
    
    Result->SetBoolField("success", true);
    Result->SetStringField("component_name", NiagaraComponent->GetName());
    Result->SetStringField("actor_name", NiagaraComponent->GetOwner() ? NiagaraComponent->GetOwner()->GetName() : TEXT(""));
    Result->SetStringField("system_name", NiagaraSystem->GetName());
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSpawnNiagaraSystemAttached(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    // Get parameters
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: asset_path");
        return Result;
    }
    
    FString AttachToActorName;
    if (!Params->TryGetStringField(TEXT("attach_to_actor"), AttachToActorName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: attach_to_actor");
        return Result;
    }
    
    // Find the actor to attach to
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "No world available");
        return Result;
    }
    
    AActor* AttachActor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if ((*It)->GetName() == AttachToActorName || (*It)->GetActorLabel() == AttachToActorName)
        {
            AttachActor = *It;
            break;
        }
    }
    
    if (!AttachActor)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Actor not found: %s"), *AttachToActorName));
        return Result;
    }
    
    // Get attachment point (optional)
    FName AttachPointName = NAME_None;
    FString AttachPointString;
    if (Params->TryGetStringField(TEXT("attach_point"), AttachPointString))
    {
        AttachPointName = *AttachPointString;
    }
    
    // Get location offset (optional)
    FVector LocationOffset = FVector::ZeroVector;
    const TArray<TSharedPtr<FJsonValue>>* LocationJsonValueArray;
    if (Params->TryGetArrayField(TEXT("location_offset"), LocationJsonValueArray) && LocationJsonValueArray->Num() == 3)
    {
        LocationOffset = FVector((*LocationJsonValueArray)[0]->AsNumber(), (*LocationJsonValueArray)[1]->AsNumber(), (*LocationJsonValueArray)[2]->AsNumber());
    }
    
    // Get rotation (optional)
    FRotator Rotation = FRotator::ZeroRotator;
    const TArray<TSharedPtr<FJsonValue>>* RotationJsonValueArray;
    if (Params->TryGetArrayField(TEXT("rotation"), RotationJsonValueArray) && RotationJsonValueArray->Num() == 3)
    {
        Rotation = FRotator((*RotationJsonValueArray)[0]->AsNumber(), (*RotationJsonValueArray)[1]->AsNumber(), (*RotationJsonValueArray)[2]->AsNumber());
    }
    
    // Optional auto_destroy (default: true)
    bool bAutoDestroy = true;
    Params->TryGetBoolField(TEXT("auto_destroy"), bAutoDestroy);
    
    // Optional auto_activate (default: true)
    bool bAutoActivate = true;
    Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate);
    
    // Load Niagara system asset
    UNiagaraSystem* NiagaraSystem = LoadNiagaraSystemAsset(AssetPath);
    if (!NiagaraSystem)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
        return Result;
    }
    
    // Find root component or use first available scene component
    USceneComponent* AttachComponent = AttachActor->GetRootComponent();
    if (!AttachComponent)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Target actor has no root component");
        return Result;
    }
    
    // Spawn attached Niagara system
    UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
        NiagaraSystem,
        AttachComponent,
        AttachPointName,
        LocationOffset,
        Rotation,
        EAttachLocation::KeepRelativeOffset,
        bAutoDestroy,
        bAutoActivate
    );
    
    if (!NiagaraComponent)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Failed to spawn attached Niagara system");
        return Result;
    }
    
    Result->SetBoolField("success", true);
    Result->SetStringField("component_name", NiagaraComponent->GetName());
    Result->SetStringField("attached_to", AttachToActorName);
    Result->SetStringField("system_name", NiagaraSystem->GetName());
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleGetNiagaraSystems(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "No world available");
        return Result;
    }
    
    TArray<TSharedPtr<FJsonValue>> SystemsArray;
    
    // Find all Niagara actors
    for (TActorIterator<ANiagaraActor> It(World); It; ++It)
    {
        ANiagaraActor* NiagaraActor = *It;
        if (NiagaraActor)
        {
            TSharedPtr<FJsonObject> SystemInfo = MakeShareable(new FJsonObject);
            SystemInfo->SetStringField("actor_name", NiagaraActor->GetName());
            SystemInfo->SetStringField("actor_label", NiagaraActor->GetActorLabel());
            
            UNiagaraComponent* NiagaraComp = NiagaraActor->GetNiagaraComponent();
            if (NiagaraComp)
            {
                SystemInfo->SetStringField("component_name", NiagaraComp->GetName());
                
                if (NiagaraComp->GetAsset())
                {
                    SystemInfo->SetStringField("system_asset", NiagaraComp->GetAsset()->GetPathName());
                    SystemInfo->SetStringField("system_name", NiagaraComp->GetAsset()->GetName());
                }
                
                SystemInfo->SetBoolField("is_active", NiagaraComp->IsActive());
                
                // Get transform
                TArray<TSharedPtr<FJsonValue>> LocationArray;
                FVector Location = NiagaraActor->GetActorLocation();
                LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
                LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
                LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
                SystemInfo->SetArrayField("location", LocationArray);
            }
            
            SystemsArray.Add(MakeShareable(new FJsonValueObject(SystemInfo)));
        }
    }
    
    // Also find any Niagara components on other actors
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Cast<ANiagaraActor>(Actor))
        {
            TArray<UNiagaraComponent*> NiagaraComponents;
            Actor->GetComponents<UNiagaraComponent>(NiagaraComponents);
            
            for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
            {
                TSharedPtr<FJsonObject> SystemInfo = MakeShareable(new FJsonObject);
                SystemInfo->SetStringField("actor_name", Actor->GetName());
                SystemInfo->SetStringField("actor_label", Actor->GetActorLabel());
                SystemInfo->SetStringField("component_name", NiagaraComp->GetName());
                
                if (NiagaraComp->GetAsset())
                {
                    SystemInfo->SetStringField("system_asset", NiagaraComp->GetAsset()->GetPathName());
                    SystemInfo->SetStringField("system_name", NiagaraComp->GetAsset()->GetName());
                }
                
                SystemInfo->SetBoolField("is_active", NiagaraComp->IsActive());
                SystemsArray.Add(MakeShareable(new FJsonValueObject(SystemInfo)));
            }
        }
    }
    
    Result->SetBoolField("success", true);
    Result->SetArrayField("systems", SystemsArray);
    Result->SetNumberField("count", SystemsArray.Num());
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraFloatParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName;
    float Value;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    if (!Params->TryGetNumberField(TEXT("value"), Value))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: value");
        return Result;
    }
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->SetFloatParameter(*ParameterName, Value);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    Result->SetNumberField("value", Value);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraVectorParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* ValueJsonValueArray;
    if (!Params->TryGetArrayField(TEXT("value"), ValueJsonValueArray) || ValueJsonValueArray->Num() < 3)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing or invalid parameter: value (expected [x, y, z])");
        return Result;
    }
    
    FVector Value((*ValueJsonValueArray)[0]->AsNumber(), (*ValueJsonValueArray)[1]->AsNumber(), (*ValueJsonValueArray)[2]->AsNumber());
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->SetVectorParameter(*ParameterName, Value);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraColorParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* ValueJsonValueArray;
    if (!Params->TryGetArrayField(TEXT("value"), ValueJsonValueArray) || ValueJsonValueArray->Num() < 4)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing or invalid parameter: value (expected [r, g, b, a])");
        return Result;
    }
    
    FLinearColor Value((*ValueJsonValueArray)[0]->AsNumber(), (*ValueJsonValueArray)[1]->AsNumber(), (*ValueJsonValueArray)[2]->AsNumber(), (*ValueJsonValueArray)[3]->AsNumber());
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->SetColorParameter(*ParameterName, Value);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraBoolParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName;
    bool Value;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    if (!Params->TryGetBoolField(TEXT("value"), Value))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: value");
        return Result;
    }
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->SetBoolParameter(*ParameterName, Value);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    Result->SetBoolField("value", Value);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraIntParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName;
    int32 Value;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    if (!Params->TryGetNumberField(TEXT("value"), Value))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: value");
        return Result;
    }
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->SetIntParameter(*ParameterName, Value);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    Result->SetNumberField("value", Value);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleSetNiagaraTextureParameter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName, ParameterName, TexturePath;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: parameter_name");
        return Result;
    }
    
    if (!Params->TryGetStringField(TEXT("texture_path"), TexturePath))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Missing required parameter: texture_path");
        return Result;
    }
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    // Load texture
    UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
    if (!Texture)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Failed to load texture: %s"), *TexturePath));
        return Result;
    }
    
    UNiagaraFunctionLibrary::SetTextureObject(NiagaraComp, ParameterName, Texture);
    
    Result->SetBoolField("success", true);
    Result->SetStringField("parameter_name", ParameterName);
    Result->SetStringField("texture_path", TexturePath);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleGetNiagaraParameters(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    // Get system info
    if (NiagaraComp->GetAsset())
    {
        Result->SetStringField("system_asset", NiagaraComp->GetAsset()->GetPathName());
    }
    
    Result->SetBoolField("is_active", NiagaraComp->IsActive());
    
    // Note: Getting parameter values requires accessing the override parameters store
    // This is a simplified version - full implementation would iterate over all parameters
    
    Result->SetBoolField("success", true);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleActivateNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    bool bReset = false;
    Params->TryGetBoolField(TEXT("reset"), bReset);
    
    NiagaraComp->Activate(bReset);
    
    Result->SetBoolField("success", true);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleDeactivateNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    NiagaraComp->Deactivate();
    
    Result->SetBoolField("success", true);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleDestroyNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString ActorName, ComponentName;
    
    Params->TryGetStringField(TEXT("actor_name"), ActorName);
    Params->TryGetStringField(TEXT("component_name"), ComponentName);
    
    UNiagaraComponent* NiagaraComp = FindNiagaraComponent(ComponentName, ActorName);
    if (!NiagaraComp)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Niagara component not found");
        return Result;
    }
    
    AActor* Owner = NiagaraComp->GetOwner();
    if (Owner && Cast<ANiagaraActor>(Owner))
    {
        // If it's a Niagara actor, destroy the whole actor
        Owner->Destroy();
    }
    else
    {
        // Otherwise just destroy the component
        NiagaraComp->DestroyComponent();
    }
    
    Result->SetBoolField("success", true);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleGetNiagaraAssets(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    FString SearchPath = "/Game";
    Params->TryGetStringField(TEXT("search_path"), SearchPath);
    
    TArray<TSharedPtr<FJsonValue>> AssetsArray;
    
    // Use asset registry to find all Niagara systems
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    FARFilter Filter;
    Filter.PackagePaths.Add(*SearchPath);
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Niagara.NiagaraSystem")));
    Filter.bRecursivePaths = true;
    
    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);
    
    for (const FAssetData& Asset : AssetList)
    {
        TSharedPtr<FJsonObject> AssetInfo = MakeShareable(new FJsonObject);
        AssetInfo->SetStringField("name", Asset.AssetName.ToString());
        AssetInfo->SetStringField("path", Asset.PackageName.ToString());
        AssetInfo->SetStringField("full_path", Asset.GetFullName());
        AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetInfo)));
    }
    
    Result->SetBoolField("success", true);
    Result->SetArrayField("assets", AssetsArray);
    Result->SetNumberField("count", AssetsArray.Num());
    
    return Result;
}

UNiagaraComponent* FEpicUnrealMCPNiagaraCommands::FindNiagaraComponent(const FString& ComponentName, const FString& ActorName)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return nullptr;
    }
    
    // If component name is provided, search by component name
    if (!ComponentName.IsEmpty())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UNiagaraComponent*> NiagaraComponents;
            (*It)->GetComponents<UNiagaraComponent>(NiagaraComponents);
            
            for (UNiagaraComponent* Comp : NiagaraComponents)
            {
                if (Comp->GetName() == ComponentName)
                {
                    return Comp;
                }
            }
        }
    }
    
    // If actor name is provided, search by actor name
    if (!ActorName.IsEmpty())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if ((*It)->GetName() == ActorName || (*It)->GetActorLabel() == ActorName)
            {
                UNiagaraComponent* Comp = (*It)->FindComponentByClass<UNiagaraComponent>();
                if (Comp)
                {
                    return Comp;
                }
            }
        }
    }
    
    return nullptr;
}

UNiagaraSystem* FEpicUnrealMCPNiagaraCommands::LoadNiagaraSystemAsset(const FString& AssetPath)
{
    return LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
}

// ============================================================================
// Asset Analysis Implementation (New Feature)
// ============================================================================

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::HandleGetNiagaraAssetDetails(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    
    // Get asset path
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Missing required parameter: asset_path"));
        return Result;
    }
    
    // Load the system
    UNiagaraSystem* NiagaraSystem = LoadNiagaraSystemAsset(AssetPath);
    if (!NiagaraSystem)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
        return Result;
    }
    
    // Get detail level (default: overview)
    FString DetailLevel = TEXT("overview");
    Params->TryGetStringField(TEXT("detail_level"), DetailLevel);
    
    // Parse include sections
    TArray<FString> IncludeSections = ParseIncludeSections(Params);
    
    // Get requested emitters (if specified)
    TArray<FString> RequestedEmitters;
    const TArray<TSharedPtr<FJsonValue>>* EmittersArray;
    if (Params->TryGetArrayField(TEXT("emitters"), EmittersArray))
    {
        for (const auto& Val : *EmittersArray)
        {
            RequestedEmitters.Add(Val->AsString());
        }
    }
    
    // Build response based on detail level
    Result->SetStringField(TEXT("asset_name"), NiagaraSystem->GetName());
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    Result->SetNumberField(TEXT("emitter_count"), NiagaraSystem->GetNumEmitters());
    
    // Emitter data
    TArray<TSharedPtr<FJsonValue>> EmittersJson;
    
    for (FNiagaraEmitterHandle& Handle : NiagaraSystem->GetEmitterHandles())
    {
        if (!Handle.IsValid()) continue;
        
        FString EmitterName = Handle.GetName().ToString();
        
        // Filter by requested emitters if specified
        if (RequestedEmitters.Num() > 0 && !RequestedEmitters.Contains(EmitterName))
        {
            continue;
        }
        
        if (DetailLevel == TEXT("overview"))
        {
            // Basic info only
            TSharedPtr<FJsonObject> EmitterOverview = MakeShareable(new FJsonObject);
            EmitterOverview->SetStringField(TEXT("name"), EmitterName);
            EmitterOverview->SetBoolField(TEXT("is_enabled"), Handle.GetIsEnabled());
            
            FString ModeStr = TEXT("Standard");
            if (Handle.GetEmitterMode() == ENiagaraEmitterMode::Stateless)
            {
                ModeStr = TEXT("Stateless");
            }
            EmitterOverview->SetStringField(TEXT("mode"), ModeStr);
            
            EmittersJson.Add(MakeShareable(new FJsonValueObject(EmitterOverview)));
        }
        else
        {
            // Full details
            TSharedPtr<FJsonObject> EmitterDetails = GetEmitterDetails(Handle, NiagaraSystem, IncludeSections);
            EmittersJson.Add(MakeShareable(new FJsonValueObject(EmitterDetails)));
        }
    }
    
    Result->SetArrayField(TEXT("emitters"), EmittersJson);
    Result->SetBoolField(TEXT("success"), true);
    
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::GetNiagaraSystemOverview(UNiagaraSystem* System)
{
    TSharedPtr<FJsonObject> Overview = MakeShareable(new FJsonObject);
    
    Overview->SetStringField(TEXT("name"), System->GetName());
    Overview->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());
    
    return Overview;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::GetEmitterDetails(FNiagaraEmitterHandle& Handle, 
    UNiagaraSystem* System, const TArray<FString>& IncludeSections)
{
    TSharedPtr<FJsonObject> EmitterJson = MakeShareable(new FJsonObject);
    
    // Basic info
    FString EmitterName = Handle.GetName().ToString();
    EmitterJson->SetStringField(TEXT("name"), EmitterName);
    EmitterJson->SetBoolField(TEXT("is_enabled"), Handle.GetIsEnabled());
    
    FString ModeStr = TEXT("Standard");
    if (Handle.GetEmitterMode() == ENiagaraEmitterMode::Stateless)
    {
        ModeStr = TEXT("Stateless");
    }
    EmitterJson->SetStringField(TEXT("mode"), ModeStr);
    
    // Get emitter data
    FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
    if (!EmitterData)
    {
        return EmitterJson;
    }
    
    // Scripts
    if (ShouldInclude(IncludeSections, TEXT("scripts")) || ShouldInclude(IncludeSections, TEXT("all")))
    {
        TSharedPtr<FJsonObject> ScriptsJson = MakeShareable(new FJsonObject);
        
        if (EmitterData->SpawnScriptProps.Script)
        {
            ScriptsJson->SetObjectField(TEXT("spawn"), GetScriptDetails(EmitterData->SpawnScriptProps.Script));
        }
        if (EmitterData->UpdateScriptProps.Script)
        {
            ScriptsJson->SetObjectField(TEXT("update"), GetScriptDetails(EmitterData->UpdateScriptProps.Script));
        }
        
        // Event handlers
        TArray<TSharedPtr<FJsonValue>> EventHandlersJson;
        for (const FNiagaraEventScriptProperties& EventHandler : EmitterData->GetEventHandlers())
        {
            if (EventHandler.Script)
            {
                TSharedPtr<FJsonObject> EventJson = GetScriptDetails(EventHandler.Script);
                EventJson->SetStringField(TEXT("execution_mode"), 
                    EventHandler.ExecutionMode == EScriptExecutionMode::EveryParticle ? TEXT("EveryParticle") : 
                    (EventHandler.ExecutionMode == EScriptExecutionMode::SpawnedParticles ? TEXT("SpawnedParticles") : TEXT("SingleParticle")));
                EventHandlersJson.Add(MakeShareable(new FJsonValueObject(EventJson)));
            }
        }
        if (EventHandlersJson.Num() > 0)
        {
            ScriptsJson->SetArrayField(TEXT("event_handlers"), EventHandlersJson);
        }
        
        EmitterJson->SetObjectField(TEXT("scripts"), ScriptsJson);
    }
    
    // Renderers
    if (ShouldInclude(IncludeSections, TEXT("renderers")) || ShouldInclude(IncludeSections, TEXT("all")))
    {
        TArray<TSharedPtr<FJsonValue>> RenderersJson;
        for (UNiagaraRendererProperties* Renderer : EmitterData->RendererProperties)
        {
            if (Renderer)
            {
                RenderersJson.Add(MakeShareable(new FJsonValueObject(GetRendererDetails(Renderer))));
            }
        }
        EmitterJson->SetArrayField(TEXT("renderers"), RenderersJson);
        EmitterJson->SetNumberField(TEXT("renderer_count"), RenderersJson.Num());
    }
    
    // Simulation stages
    if (ShouldInclude(IncludeSections, TEXT("simulation_stages")) || ShouldInclude(IncludeSections, TEXT("all")))
    {
        TArray<TSharedPtr<FJsonValue>> StagesJson;
        for (UNiagaraSimulationStageBase* Stage : EmitterData->GetSimulationStages())
        {
            if (Stage)
            {
                StagesJson.Add(MakeShareable(new FJsonValueObject(GetSimulationStageDetails(Stage))));
            }
        }
        EmitterJson->SetArrayField(TEXT("simulation_stages"), StagesJson);
        EmitterJson->SetNumberField(TEXT("simulation_stage_count"), StagesJson.Num());
    }
    
    // Parameters (simplified version - just count and names)
    if (ShouldInclude(IncludeSections, TEXT("parameters")) || ShouldInclude(IncludeSections, TEXT("all")))
    {
        TArray<TSharedPtr<FJsonValue>> ParametersJson;
        
        // From spawn script
        if (EmitterData->SpawnScriptProps.Script)
        {
            // Note: Full parameter extraction would require accessing the script's parameter store
            // This is a simplified version
            TSharedPtr<FJsonObject> ParamInfo = MakeShareable(new FJsonObject);
            ParamInfo->SetStringField(TEXT("source"), TEXT("spawn_script"));
            ParamInfo->SetStringField(TEXT("script_name"), EmitterData->SpawnScriptProps.Script->GetName());
            ParametersJson.Add(MakeShareable(new FJsonValueObject(ParamInfo)));
        }
        
        EmitterJson->SetArrayField(TEXT("parameters"), ParametersJson);
    }
    
    return EmitterJson;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::GetScriptDetails(UNiagaraScript* Script)
{
    TSharedPtr<FJsonObject> ScriptJson = MakeShareable(new FJsonObject);
    
    if (!Script)
    {
        return ScriptJson;
    }
    
    ScriptJson->SetStringField(TEXT("name"), Script->GetName());
    
    // Script usage type
    FString UsageStr = TEXT("Unknown");
    switch (Script->GetUsage())
    {
        case ENiagaraScriptUsage::Function: UsageStr = TEXT("Function"); break;
        case ENiagaraScriptUsage::Module: UsageStr = TEXT("Module"); break;
        case ENiagaraScriptUsage::DynamicInput: UsageStr = TEXT("DynamicInput"); break;
        case ENiagaraScriptUsage::ParticleSpawnScript: UsageStr = TEXT("ParticleSpawn"); break;
        case ENiagaraScriptUsage::ParticleUpdateScript: UsageStr = TEXT("ParticleUpdate"); break;
        case ENiagaraScriptUsage::EmitterSpawnScript: UsageStr = TEXT("EmitterSpawn"); break;
        case ENiagaraScriptUsage::EmitterUpdateScript: UsageStr = TEXT("EmitterUpdate"); break;
        case ENiagaraScriptUsage::SystemSpawnScript: UsageStr = TEXT("SystemSpawn"); break;
        case ENiagaraScriptUsage::SystemUpdateScript: UsageStr = TEXT("SystemUpdate"); break;
        case ENiagaraScriptUsage::ParticleEventScript: UsageStr = TEXT("ParticleEvent"); break;
        default: break;
    }
    ScriptJson->SetStringField(TEXT("usage"), UsageStr);
    
    return ScriptJson;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::GetRendererDetails(UNiagaraRendererProperties* Renderer)
{
    TSharedPtr<FJsonObject> RendererJson = MakeShareable(new FJsonObject);
    
    if (!Renderer)
    {
        return RendererJson;
    }
    
    // Renderer type
    FString RendererType = Renderer->GetClass()->GetName();
    RendererType.RemoveFromEnd(TEXT("Properties"));
    RendererJson->SetStringField(TEXT("type"), RendererType);
    
    // Common properties
    RendererJson->SetBoolField(TEXT("is_enabled"), Renderer->GetIsEnabled());
    
    // Material info (if available)
    // Note: Material access depends on renderer type
    // This is a simplified version
    
    return RendererJson;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPNiagaraCommands::GetSimulationStageDetails(UNiagaraSimulationStageBase* Stage)
{
    TSharedPtr<FJsonObject> StageJson = MakeShareable(new FJsonObject);
    
    if (!Stage)
    {
        return StageJson;
    }
    
    StageJson->SetStringField(TEXT("name"), Stage->GetName());
    StageJson->SetBoolField(TEXT("enabled"), Stage->bEnabled);
    
    return StageJson;
}

TArray<FString> FEpicUnrealMCPNiagaraCommands::ParseIncludeSections(const TSharedPtr<FJsonObject>& Params)
{
    TArray<FString> IncludeSections;
    
    const TArray<TSharedPtr<FJsonValue>>* IncludeArray;
    if (Params->TryGetArrayField(TEXT("include"), IncludeArray))
    {
        for (const auto& Val : *IncludeArray)
        {
            IncludeSections.Add(Val->AsString().ToLower());
        }
    }
    else
    {
        // Default: include all
        IncludeSections.Add(TEXT("all"));
    }
    
    return IncludeSections;
}

bool FEpicUnrealMCPNiagaraCommands::ShouldInclude(const TArray<FString>& IncludeSections, const FString& Section)
{
    if (IncludeSections.Contains(TEXT("all")))
    {
        return true;
    }
    return IncludeSections.Contains(Section.ToLower());
}
