// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

#include "Sockets.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

#include "Misc/FrameRate.h"
#include "Misc/Timecode.h"
#include "Misc/QualifiedFrameTime.h"

#include "Serialization/ArrayReader.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

class FRunnable;
class FSocket;

class FArrayReader;
class FBufferArchive;
typedef FBufferArchive FArrayWriter;

struct FLensPacket;

DECLARE_DELEGATE_OneParam(FOnFrameDataReady, FLensPacket InData);
DECLARE_DELEGATE(FOnHandshakeEstablished);

struct FLensPacket
{
	FQualifiedFrameTime FrameTime;

	float FocusDistance = 0.0f;
	float Aperture = 0.0f;
	float HorizontalFOV = 0.0f;
	int16 EntrancePupilPosition = 0;
	float ShadingData[6] = { 0.0f };
	float DistortionData[6] = { 0.0f };

	float FocalLength = 0.0f;
};

enum class EDragonDeviceType : uint8
{
	Unknown = 0x00,
	Camera = 0x01,
	SoundRecorder = 0x02,
	TimecodeGenerator = 0x03,
	SlateInfoDevice = 0x04,
	Slate = 0x05,
	Storage = 0x06,
	Lens = 0x07,
};

enum class EDragonUnitType : uint8
{
	Unknown = 0x00,
	Imperial = 0x01,
	Metric = 0x02,
};

enum class EDragonLensType : uint8
{
	Unknown = 0x00,
	Prime = 0x01,
	Zoom = 0x02,
};

struct FDragonDevice
{
	bool bHasAppeared = false;

	double MinAPIVersion = 0.0;
	double MaxAPIVersion = 0.0;

	//~ Begin scene state information
	FString Production;
	FString Scene;
	FString Take;
	FString ExposureName;

	uint16 Frame = 0;
	uint16 MocoFrame = 0;
	uint16 Exposure = 0;

	uint16 StereoIndex = 0;

	bool ReadyToCapture = false;
	FString CaptureState;
	FString ImageFileName;
	//~ End scene state information

};

struct FDragonProtocol
{
	static const double DragonAPIVersion;

	// The two classes of things, events and commands
	static const FString EventString;
	static const FString CommandString;

	// Events - thins we need to respond to
	static const FString KeepAliveString;
	static const FString PositionString;
	static const FString CaptureStateString;
	static const FString ShootString;
	static const FString DeleteString;
	static const FString CaptureCompleteString;
	static const FString FrameCompleteString;
	static const FString ViewFrameString;

	// Fields
	static const FString MinVersionString;
	static const FString MaxVersionString;
	static const FString ProductionString;
	static const FString SceneString;
	static const FString TakeString;
	static const FString ExposureNameString;
	static const FString FrameString;
	static const FString MocoFrameString;
	static const FString ExposureString;
	static const FString StereoIndexString;
	static const FString ImageFileNameString;
	static const FString ReadyToCaptureString;
	static const FString StateString;

	// commands - some are symmetric to events
	// static const FString ShootString
	// static const FString DeleteString
	static const FString PlayString;
	static const FString LiveString;
	static const FString MuteString;
	static const FString BlackString;
	static const FString LoopString;
	static const FString OpacityDownString;
	static const FString OpacityUpString;
	static const FString StepForward;
	static const FString StepBackward;
	static const FString ShortPlayString;
	static const FString LiveToggleString;
	static const FString AutoToggleString;
	static const FString HighResToggleString;
	static const FString ViewFrameUpdatesString;

	static const FString FramesString; // nframes
	// static const FString StateString;
	static const FString DoNotPingString;
	static const FString VersionString;
	static const FString PressedString;
	static const FString ReleasedString;
	static const FString ActiveString;
};

// experiment 1
// USTRUCT()
// struct FStayAlive
// {
// 	GENERATED_BODY()

// public:
// 	UPROPERTY()
// 	FString event;

// 	UPROPERTY()
// 	double minVersion;

// 	UPROPERTY()
// 	double maxVersion;
// };

class FLiveLinkDragonMessageThread : public FRunnable
{
public:

	FLiveLinkDragonMessageThread(FSocket* InSocket);
	~FLiveLinkDragonMessageThread();

	void Start();

	/**
	 * Returns a delegate that is executed when frame data is ready.
	 *
	 * @return The delegate.
	 */
	FOnFrameDataReady& OnFrameDataReady_AnyThread()
	{
		return FrameDataReadyDelegate;
	}

	FOnHandshakeEstablished& OnHandshakeEstablished_AnyThread()
	{
		return HandshakeEstablishedDelegate;
	}

public:

	//~ FRunnable Interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	// End FRunnable Interface

private:

	void GenerateFrameRateMap();

	void ParsePacket(const FString InPacket);

	void HandleKeepAliveEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandlePositionEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleCaptureStateEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleShootEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleDeleteEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleCaptureCompleteEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleFrameCompleteEvent(const TSharedPtr<FJsonObject> InEvent);
	void HandleViewFrameEvent(const TSharedPtr<FJsonObject> InEvent);
	
	void InitiateHandshake();

	void SendMessageToServer(const TSharedPtr<FJsonObject> InMessageToSend);
	void AcknowledgeMessageFromServer(const TArray<uint8> InMessageFromServer, const uint32 InServerMessageLength);

	void HashDataRequestMessage(const FArrayWriter InMessage, const FString InRequestName);

	void SubscribeToDeviceMetadataUpdates(const EDragonDeviceType InDeviceType);
	void SubscribeToVolatileDataUpdates(uint64 InDeviceID, const EDragonDeviceType InDeviceType);

	void ParseZeissLensData(FArrayReader& InData, const FDragonDevice& Device);

	
private:
	
	//ISocketSubsystem *SocketSubsystem = nullptr;
	FSocket* const Socket;

	// The IP address and port of the Dragonframe client
	FString RemoteIP;
	int32 RemotePort;

	TUniquePtr<FRunnableThread>	Thread;
	bool bIsThreadRunning = false;

	// handshaky stuff -  first time through
	bool bIsHandshook = false;

	// // probably unnecessary
	// TMap<FMessageHash, FString> DataRequests;
	// TMap<uint64, FDragonDevice> DetectedDevices;

	FDragonDevice DragonDevice;

	// useful for something
	TMap<FString, FFrameRate> FrameRates;
	FFrameRate DragonFrameRate = { 24, 0 };
	bool bIsDropFrameRate = false;

	FLensPacket LensData;

	FOnHandshakeEstablished HandshakeEstablishedDelegate;
	FOnFrameDataReady FrameDataReadyDelegate;

	// Pre-allocated space to copy the data for one Dragon packet into
	// FArrayReader Packet;

	// Pre-allocated space to read raw (unparsed) Zeiss lens data into
	// FZeissRawPacket ZeissRawPacket;

private:

	static constexpr uint32 ReceiveBufferSize = 1024; // these are pretty small tezxt strings
	static constexpr uint32 ThreadStackSize = 1024 * 128;
	static constexpr float Timeout = 10.0f;
};
