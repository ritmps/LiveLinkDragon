// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "LiveLinkDragonConnectionSettings.generated.h"

USTRUCT()
struct FLiveLinkDragonConnectionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FString IPAddress = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, Category = "Settings")
	int32 Port = 55555;

	UPROPERTY(EditAnywhere, Category = "Settings")
	FName SubjectName = TEXT("DragonBridgeDevice");
};
