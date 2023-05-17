// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"

#include "LiveLinkDragonSourceSettings.h"
#include "LiveLinkDragonConnectionSettings.h"

#include "LiveLinkDragonMessageThread.h"

#include <atomic>

static constexpr uint16 DragonPortNumber = 55555;  // need to make this selectable in the settings panel
static constexpr uint32 DragonBufferSize = 1024 * 32;
;

// TODO: move this into DL class
struct Bone {
    FString Name;
    FString ParentName;
    FTransform BoneTransform;
    int Index[6] = { -1,-1,-1,-1,-1,-1 };
    bool IsRoot = true;
    int Id=0;
};

struct BoneProperty {
    FString Name;
    int Index = -1;
};

struct Subject {
    FName SubjectName;
    TArray<Bone> Bones;
    TArray<BoneProperty> Properties;
    TArray<float> Values;
};
struct RobotData {
	float xv = 0.0f;
	float yv = 0.0f;
	float zv = 0.0f;
	float xt = 0.0f;
	float yt = 0.0f;
	float zt = 0.0f;
	float roll = 0.0f;
	float focus = 0.0f;
	float zoom = 0.0f;
};

class ISocketSubsystem;

class LIVELINKDRAGON_API FLiveLinkDragonSource : public ILiveLinkSource
{
public:

	FLiveLinkDragonSource(FLiveLinkDragonConnectionSettings ConnectionSettings);
	~FLiveLinkDragonSource();

	// Begin ILiveLinkSource Implementation
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;

	virtual bool IsSourceStillValid() const override;

	virtual bool RequestSourceShutdown() override;

	virtual FText GetSourceType() const override;
	virtual FText GetSourceMachineName() const override { return SourceMachineName; }
	virtual FText GetSourceStatus() const override;

	virtual TSubclassOf<ULiveLinkSourceSettings> GetSettingsClass() const { return ULiveLinkDragonSourceSettings::StaticClass(); }
	// End ILiveLinkSourceImplementation

private:
	void OpenConnection();

	// LiveLink 
	ILiveLinkClient* Client = nullptr;

	void OnHandshakeEstablished_AnyThread();
	void OnFrameDataReady_AnyThread(FLensPacket InData); // todo: change to dragon packet

	// Socketry
	FSocket* Socket = nullptr;
	ISocketSubsystem* SocketSubsystem = nullptr;
	FString SocketDescription = "Dragin Live Link Socket";
	FIPv4Endpoint DragonEndpoint;	// IP address and port number of the Dragon instance

	// Buffers
	TArray<uint8> ReceivedData;

	FLiveLinkDragonConnectionSettings ConnectionSettings;

	FLiveLinkSubjectKey SubjectKey;
	FText SourceMachineName;

	TUniquePtr<FLiveLinkDragonMessageThread> MessageThread;

	std::atomic<double> LastTimeDataReceived;
	std::atomic<bool> bReceivedData;

	const float DataReceivedTimeout = 20.0f;
};