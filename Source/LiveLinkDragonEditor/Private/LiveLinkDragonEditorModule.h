// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "Modules/ModuleManager.h"

class FLiveLinkDragonEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};