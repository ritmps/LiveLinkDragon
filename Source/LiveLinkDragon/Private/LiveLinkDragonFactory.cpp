// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#include "LiveLinkDragonFactory.h"

#include "LiveLinkDragonSource.h"
#include "LiveLinkDragonConnectionSettings.h"

#define LOCTEXT_NAMESPACE "LiveLinkDragonSourceFactory"

ULiveLinkDragonSourceFactory::FBuildCreationPanelDelegate ULiveLinkDragonSourceFactory::OnBuildCreationPanel;

FText ULiveLinkDragonSourceFactory::GetSourceDisplayName() const
{
	return LOCTEXT("SourceDisplayName", "Dragonframe");
}

FText ULiveLinkDragonSourceFactory::GetSourceTooltip() const
{
	return LOCTEXT("SourceTooltip", "Creates a connection to a Dragonframe instance");
}

TSharedPtr<SWidget> ULiveLinkDragonSourceFactory::BuildCreationPanel(FOnLiveLinkSourceCreated InOnLiveLinkSourceCreated) const
{
	if (OnBuildCreationPanel.IsBound())
	{
		return OnBuildCreationPanel.Execute(this, InOnLiveLinkSourceCreated);
	}
	return TSharedPtr<SWidget>();
}

TSharedPtr<ILiveLinkSource> ULiveLinkDragonSourceFactory::CreateSource(const FString& ConnectionString) const
{
	FLiveLinkDragonConnectionSettings Settings;
	if (!ConnectionString.IsEmpty())
	{
		FLiveLinkDragonConnectionSettings::StaticStruct()->ImportText(*ConnectionString, &Settings, nullptr, EPropertyPortFlags::PPF_None, nullptr, FLiveLinkDragonConnectionSettings::StaticStruct()->GetName(), true);
	}
	return MakeShared<FLiveLinkDragonSource>(Settings);
}

FString ULiveLinkDragonSourceFactory::CreateConnectionString(const FLiveLinkDragonConnectionSettings& Settings)
{
	FString ConnectionString;
	FLiveLinkDragonConnectionSettings::StaticStruct()->ExportText(ConnectionString, &Settings, nullptr, nullptr, EPropertyPortFlags::PPF_None, nullptr, true);
	return ConnectionString;
}

#undef LOCTEXT_NAMESPACE