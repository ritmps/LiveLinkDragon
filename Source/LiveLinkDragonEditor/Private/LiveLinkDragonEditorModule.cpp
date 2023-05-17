// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#include "LiveLinkDragonEditorModule.h"

#include "LiveLinkDragonFactory.h"
#include "LiveLinkDragonSourcePanel.h"

#define LOCTEXT_NAMESPACE "LiveLinkDragonEditorModule"

void FLiveLinkDragonEditorModule::StartupModule()
{
	// register the LiveLinkFactory panel
	auto BuildCreationPanel = [](const ULiveLinkDragonSourceFactory* Factory, ULiveLinkSourceFactory::FOnLiveLinkSourceCreated OnSourceCreated)
		-> TSharedPtr<SWidget>
	{
		return SNew(SLiveLinkDragonSourcePanel)
			.Factory(Factory)
			.OnSourceCreated(OnSourceCreated);
	};
	ULiveLinkDragonSourceFactory::OnBuildCreationPanel.BindLambda(BuildCreationPanel);
}

void FLiveLinkDragonEditorModule::ShutdownModule()
{
	ULiveLinkDragonSourceFactory::OnBuildCreationPanel.Unbind();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLiveLinkDragonEditorModule, LiveLinkDragonEditor)
