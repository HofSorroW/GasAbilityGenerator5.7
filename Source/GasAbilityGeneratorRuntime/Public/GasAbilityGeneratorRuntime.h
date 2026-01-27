// GasAbilityGeneratorRuntime v7.5.3
// Runtime module for BP-callable bridge components
// Split from Editor module to allow runtime Blueprint access

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGasAbilityGeneratorRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
