// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class UNiagaraSystem;
class UNiagaraEmitter;
class UNiagaraScript;
class UNiagaraRendererProperties;
class UNiagaraSimulationStageBase;
class UNiagaraComponent;

/**
 * Handler class for Niagara-related MCP commands
 * Handles spawning, controlling, and analyzing Niagara particle systems
 */
class UNREALMCP_API FEpicUnrealMCPNiagaraCommands
{
public:
    FEpicUnrealMCPNiagaraCommands();

    // Handle Niagara commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // ============================================================================
    // Asset Analysis (New Feature)
    // ============================================================================
    
    /**
     * Get detailed information about a Niagara asset
     * Supports selective data retrieval via detail_level and include parameters
     */
    TSharedPtr<FJsonObject> HandleGetNiagaraAssetDetails(const TSharedPtr<FJsonObject>& Params);
    
    // Helper functions for asset analysis
    TSharedPtr<FJsonObject> GetNiagaraSystemOverview(UNiagaraSystem* System);
    TSharedPtr<FJsonObject> GetEmitterDetails(class FNiagaraEmitterHandle& Handle, UNiagaraSystem* System, 
        const TArray<FString>& IncludeSections);
    TSharedPtr<FJsonObject> GetScriptDetails(UNiagaraScript* Script);
    TSharedPtr<FJsonObject> GetRendererDetails(UNiagaraRendererProperties* Renderer);
    TSharedPtr<FJsonObject> GetSimulationStageDetails(UNiagaraSimulationStageBase* Stage);
    
    // Parse include sections from params
    TArray<FString> ParseIncludeSections(const TSharedPtr<FJsonObject>& Params);
    bool ShouldInclude(const TArray<FString>& IncludeSections, const FString& Section);
    
    // ============================================================================
    // Spawn & Control (Existing Features)
    // ============================================================================
    
    TSharedPtr<FJsonObject> HandleSpawnNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnNiagaraSystemAttached(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraSystems(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraFloatParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraVectorParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraColorParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraBoolParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraIntParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNiagaraTextureParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraParameters(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleActivateNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeactivateNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDestroyNiagaraSystem(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetNiagaraAssets(const TSharedPtr<FJsonObject>& Params);
    
    // Helpers
    UNiagaraComponent* FindNiagaraComponent(const FString& ComponentName, const FString& ActorName);
    UNiagaraSystem* LoadNiagaraSystemAsset(const FString& AssetPath);
};
