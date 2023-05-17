// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "LiveLinkSourceFactory.h"

#include "LiveLinkDragonFactory.generated.h"

class SLiveLinkDragonSourcePanel;
struct FLiveLinkDragonConnectionSettings;

UCLASS()
class LIVELINKDRAGON_API ULiveLinkDragonSourceFactory : public ULiveLinkSourceFactory
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedPtr<SWidget>, FBuildCreationPanelDelegate, const ULiveLinkDragonSourceFactory*, FOnLiveLinkSourceCreated);
	static FBuildCreationPanelDelegate OnBuildCreationPanel;

public:
	GENERATED_BODY()

	//~ Begin ULiveLinkSourceFactory interface
	virtual FText GetSourceDisplayName() const;
	virtual FText GetSourceTooltip() const;

	virtual EMenuType GetMenuType() const override { return EMenuType::SubPanel; }
	virtual TSharedPtr<SWidget> BuildCreationPanel(FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const override;
	virtual TSharedPtr<ILiveLinkSource> CreateSource(const FString& ConnectionString) const override;
	//~ End ULiveLinkSourceFactory interface

	static FString CreateConnectionString(const FLiveLinkDragonConnectionSettings& Settings);
};
