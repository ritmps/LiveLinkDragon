// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "LiveLinkDragonFactory.h"
#include "LiveLinkDragonConnectionSettings.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SLiveLinkDragonSourcePanel : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SLiveLinkDragonSourcePanel) {}
		SLATE_ARGUMENT(const ULiveLinkDragonSourceFactory*, Factory)
		SLATE_EVENT(ULiveLinkSourceFactory::FOnLiveLinkSourceCreated, OnSourceCreated)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

public:
	FReply CreateNewSource(bool bShouldCreateSource);

private:
	FLiveLinkDragonConnectionSettings ConnectionSettings;
	TWeakObjectPtr<const ULiveLinkDragonSourceFactory> SourceFactory;
	ULiveLinkSourceFactory::FOnLiveLinkSourceCreated OnSourceCreated;
};