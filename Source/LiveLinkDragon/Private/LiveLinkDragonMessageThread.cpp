// Copyright (c) RITMPS, Rochester Institute of Technology, 2022

#include "LiveLinkDragonMessageThread.h"

#include "HAL/RunnableThread.h"

#include "Serialization/ArrayReader.h"
#include "Serialization/ArrayWriter.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Misc/DateTime.h"
#include "Misc/SecureHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogLiveLinkDragonMessageThread, Log, All);

//~ Constant data defined in the Dragon API
// see https://www.dragonframe.com/ufaqs/using-the-json-interface-to-receive-notifications-and-send-commands-to-dragonframe-via-udp/

const double   DragonAPIVersion = 1.0;

const FString EventString = FString(TEXT("event"));
const FString CommandString = FString(TEXT("command"));

// events
const FString KeepAliveString = FString(TEXT("hello"));
const FString PositionString = FString(TEXT("position"));
const FString CaptureStateString = FString(TEXT("captureState"));
const FString ShootString = FString(TEXT("shoot"));
const FString DeleteString = FString(TEXT("delete"));
const FString CaptureCompleteString = FString(TEXT("captureComplete"));
const FString FrameCompleteString = FString(TEXT("frameComplete"));
const FString ViewFrameString = FString(TEXT("viewFrame"));

// fields
const FString MinVersionString = FString(TEXT("minVersion"));
const FString MaxVersionString = FString(TEXT("maxVersion"));
const FString ProductionString = FString(TEXT("production"));
const FString SceneString = FString(TEXT("scene"));
const FString TakeString = FString(TEXT("take"));
const FString ExposureNameString = FString(TEXT("exposureName"));
const FString FrameString = FString(TEXT("frame"));
const FString MocoFrameString = FString(TEXT("mocoFrame"));
const FString ExposureString = FString(TEXT("exposure"));
const FString StereoIndexString = FString(TEXT("stereoIndex"));
const FString ImageFileNameString = FString(TEXT("imageFileName"));
const FString ReadyToCaptureString = FString(TEXT("readyToCapture"));
const FString StateString = FString(TEXT("state"));


// commands
const FString PlayString = FString(TEXT("play"));
const FString LiveString = FString(TEXT("live"));
const FString MuteString = FString(TEXT("mute"));
const FString BlackString = FString(TEXT("black"));
const FString LoopString = FString(TEXT("loop"));
const FString OpacityDownString = FString(TEXT("opacityDown"));
const FString OpacityUpString = FString(TEXT("opacityUp"));
const FString StepForwardString = FString(TEXT("stepForward"));
const FString StepBackString = FString(TEXT("stepBackward"));
const FString ShortPlayString = FString(TEXT("shortPlay"));
const FString LiveToggleString = FString(TEXT("liveToggle"));
const FString AutoToggleString = FString(TEXT("autoToggle"));
const FString HighResToggleString = FString(TEXT("highResToggle"));
const FString ViewFrameUpdatesString = FString(TEXT("viewFrameUpdates"));

// fields
const FString FramesString = FString(TEXT("frames"));
const FString DoNotPingString = FString(TEXT("doNotPing"));
const FString VersionString = FString(TEXT("version"));
const FString PressedString = FString(TEXT("pressed"));
const FString ReleasedString = FString(TEXT("released"));
const FString ActiveString = FString(TEXT("active"));
//~ End constant data defined in the Dragon API

// const FString FDragonDevice::ZeissLensName = FString(TEXT("Carl Zeiss AG"));

FLiveLinkDragonMessageThread::FLiveLinkDragonMessageThread(FSocket *InSocket) : Socket(InSocket)
{
}

FLiveLinkDragonMessageThread::~FLiveLinkDragonMessageThread()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
	}
}

void FLiveLinkDragonMessageThread::Start()
{
	Thread.Reset(FRunnableThread::Create(this, TEXT("Dragon UDP Message Thread"), ThreadStackSize, TPri_AboveNormal));
}

bool FLiveLinkDragonMessageThread::Init()
{
	bIsThreadRunning = true;
	return true;
}

void FLiveLinkDragonMessageThread::Stop()
{
	bIsThreadRunning = false;
}

uint32 FLiveLinkDragonMessageThread::Run()
{
	ISocketSubsystem *SocketSub = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedRef<FInternetAddr> RemoteAddress = SocketSub->CreateInternetAddr();

	// Pre-allocate a buffer to receive data into from the socket
	uint8 ReceiveBuffer[ReceiveBufferSize];

	while (bIsThreadRunning) 
	{
		double TimeLeft = Timeout;
		double StartTime = FPlatformTime::Seconds();

		if (Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(TimeLeft)))
		{
			double EndTime = FPlatformTime::Seconds();
			int32 NumBytesReceived = 0;

			if (Socket->RecvFrom(ReceiveBuffer, ReceiveBufferSize, NumBytesReceived, *RemoteAddress))
			{
				if(NumBytesReceived == 0)
				{
					UE_LOG(LogLiveLinkDragonMessageThread, Warning, TEXT("Received 0 bytes from socket."));
					continue;
				}

				FString addy = RemoteAddress->ToString(true);
				UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Received %d from %s"), NumBytesReceived, *addy);

				RemotePort = RemoteAddress->GetPort(); // assume localhost for now
				RemoteIP = RemoteAddress->ToString(false);

				// terminate the packet?
				ReceiveBuffer[NumBytesReceived] = 0x00;
				FString Packet = FString((char*) ReceiveBuffer);

				ParsePacket(Packet); 
			}
			else if (Socket->GetConnectionState() == ESocketConnectionState::SCS_ConnectionError)
			{
				UE_LOG(LogLiveLinkDragonMessageThread, Warning, TEXT("Socket Error."));
				break;
			}
		}
	}
	return 0;
}

void FLiveLinkDragonMessageThread::ParsePacket(const FString InPacket)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<ANSICHAR>::Create(InPacket);
	FJsonSerializer::Deserialize(JsonReader, JsonObject);

	FString EventType;
	if (JsonObject->TryGetStringField(EventString, EventType))
	{
		UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Received event of type %s"), *EventType);

		if (EventType == KeepAliveString) {
			HandleKeepAliveEvent(JsonObject);
		}
		else if (EventType == PositionString)
		{
			HandlePositionEvent(JsonObject);
		}
		else if (EventType == CaptureStateString)
		{
			HandleCaptureStateEvent(JsonObject);
		}
		else if (EventType == ShootString) {
			HandleShootEvent(JsonObject);
		}
		else if (EventType == DeleteString) {
			HandleDeleteEvent(JsonObject);
		}
		else if (EventType == CaptureCompleteString) {
			HandleCaptureCompleteEvent(JsonObject);
		}
		else if (EventType == FrameCompleteString) {
			HandleFrameCompleteEvent(JsonObject);
		}
		else if (EventType == ViewFrameString) {
			HandleViewFrameEvent(JsonObject);
		}
		else {
			if( !EventType.IsEmpty() )
			{
				UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Received unknown event type: %s"), *EventType);
			}
			else 
			{
				UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Received empty event!"));
			}
		}
	}
	else {
		UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Received event with no type: %s"), *InPacket);
	}
}

void FLiveLinkDragonMessageThread::HandleKeepAliveEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// Parse the keep alive event
	// {"event" : "hello",
	//  "minVersion" : 1.0,
	//  "maxVersion" : 1.0
	// }

	// Update the Dragon device information
	DragonDevice.MinAPIVersion = InJsonObject->GetNumberField(MinVersionString);
	DragonDevice.MaxAPIVersion = InJsonObject->GetNumberField(MaxVersionString);

	// Check that the Dragon API version is supported
	if (DragonDevice.MinAPIVersion <= DragonAPIVersion && DragonDevice.MaxAPIVersion >= DragonAPIVersion)
	{
		UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Dragon API version %f supported"), DragonAPIVersion);
	}
	else
	{
		UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Dragon API version %f not supported"), DragonAPIVersion);
	}

	// Send a handshake reply
	InitiateHandshake();
}

// NB - ther is a 'better' way to do this that decodes the JSON into a struct directly
// 

void FLiveLinkDragonMessageThread::HandlePositionEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// Parse the position event
	// {"event" : "position",
	//  "production" : "[PRODUCTION]",
	//  "scene" : "[SCENE]",
	//  "take" : "[TAKE]",
	//  "frame" : [FRAME],
	//  "mocoFrame" : [MOCO FRAME],
	//  "exposure" : [EXPOSURE],
	//  "exposureName" : "[EXPOSURE NAME]",
	//  "stereoIndex" : [INDEX]}

	DragonDevice.Production = InJsonObject->GetStringField(ProductionString);
	DragonDevice.Scene = InJsonObject->GetStringField(SceneString);
	DragonDevice.Take = InJsonObject->GetStringField(TakeString);
	DragonDevice.ExposureName = InJsonObject->GetStringField(ExposureNameString);

	DragonDevice.Frame = InJsonObject->GetNumberField(FrameString);
	DragonDevice.MocoFrame = InJsonObject->GetNumberField(MocoFrameString);
	DragonDevice.Exposure = InJsonObject->GetNumberField(ExposureString);
	DragonDevice.StereoIndex = InJsonObject->GetNumberField(StereoIndexString);

	// respond to this?
	FrameDataReadyDelegate.ExecuteIfBound(LensData);
}

void FLiveLinkDragonMessageThread::HandleShootEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// { "event" : "shoot"
	// 	"production" : "PRODUCTION",
	// 	"scene" : "SCENE",
	// 	"take" : "READY"
	// 	"frame" : 1,
	// 	"exposurte" : 1,
	// 	"exposureName" : "EXPOSURE NAME",

	// }

	DragonDevice.Production = InJsonObject->GetStringField(ProductionString);
	DragonDevice.Scene = InJsonObject->GetStringField(SceneString);
	DragonDevice.Take = InJsonObject->GetStringField(TakeString);
	DragonDevice.ExposureName = InJsonObject->GetStringField(ExposureNameString);

	DragonDevice.Frame = InJsonObject->GetNumberField(FrameString);
	DragonDevice.Exposure = InJsonObject->GetNumberField(ExposureString);
	DragonDevice.StereoIndex = InJsonObject->GetNumberField(StereoIndexString);

	FrameDataReadyDelegate.ExecuteIfBound(LensData);
	// respond to this?
}

void FLiveLinkDragonMessageThread::HandleDeleteEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// { "event" : "delete",
	// 	"production" : "PRODUCTION",
	// 	"scene" : "SCENE",
	// 	"take" : "READY"	

	DragonDevice.Production = InJsonObject->GetStringField(ProductionString);
	DragonDevice.Scene = InJsonObject->GetStringField(SceneString);
	DragonDevice.Take = InJsonObject->GetStringField(TakeString);

	FrameDataReadyDelegate.ExecuteIfBound(LensData);
	// respond to this?
}

void FLiveLinkDragonMessageThread::HandleCaptureStateEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// { "event" : "captureState", 
	// 	"readyToCapture" : true, 
	// 	"state" : "READY" 
	// }

	DragonDevice.ReadyToCapture = InJsonObject->GetBoolField(ReadyToCaptureString);
	DragonDevice.CaptureState = InJsonObject->GetStringField(StateString);

	// respond to this?
	FrameDataReadyDelegate.ExecuteIfBound(LensData);
}

void FLiveLinkDragonMessageThread::HandleCaptureCompleteEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// { "event" : "captureComplete",
	// 	"production" : "PRODUCTION",
	// 	"scene" : "SCENE",
	// 	"take" : "TAKE",
	// 	"frame" : 1,
	// 	"exposure" : 1,
	// 	"exposureName" : "EXPOSURE NAME",
	// 	"stereoIndex" : 0,
	// }

	DragonDevice.Production = InJsonObject->GetStringField(ProductionString);
	DragonDevice.Scene = InJsonObject->GetStringField(SceneString);
	DragonDevice.Take = InJsonObject->GetStringField(TakeString);
	DragonDevice.ExposureName = InJsonObject->GetStringField(ExposureNameString);

	DragonDevice.Frame = InJsonObject->GetNumberField(FrameString);
	DragonDevice.Exposure = InJsonObject->GetNumberField(ExposureString);
	DragonDevice.StereoIndex = InJsonObject->GetNumberField(StereoIndexString);

	DragonDevice.ImageFileName = InJsonObject->GetStringField(ImageFileNameString);

	FrameDataReadyDelegate.ExecuteIfBound(LensData);
}

void FLiveLinkDragonMessageThread::HandleFrameCompleteEvent(const TSharedPtr<FJsonObject> InJsonObject)
{
	// { "event" : "captureComplete",
	// 	"production" : "PRODUCTION",
	// 	"scene" : "SCENE",
	// 	"take" : "TAKE",
	// 	"frame" : 1,
	// 	"exposure" : 1,
	// 	"exposureName" : "EXPOSURE NAME",
	// 	"stereoIndex" : 0,
	// }

	DragonDevice.Production = InJsonObject->GetStringField(ProductionString);
	DragonDevice.Scene = InJsonObject->GetStringField(SceneString);
	DragonDevice.Take = InJsonObject->GetStringField(TakeString);
	DragonDevice.ExposureName = InJsonObject->GetStringField(ExposureNameString);

	DragonDevice.Frame = InJsonObject->GetNumberField(FrameString);
	DragonDevice.Exposure = InJsonObject->GetNumberField(ExposureString);
	DragonDevice.StereoIndex = InJsonObject->GetNumberField(StereoIndexString);

	DragonDevice.ImageFileName = InJsonObject->GetStringField(ImageFileNameString);

	FrameDataReadyDelegate.ExecuteIfBound(LensData);
}

void FLiveLinkDragonMessageThread::HandleViewFrameEvent(const TSharedPtr<FJsonObject> InJsonObject)
{

	DragonDevice.Frame = InJsonObject->GetNumberField(FrameString);
	DragonDevice.Exposure = InJsonObject->GetNumberField(ExposureString);

	FrameDataReadyDelegate.ExecuteIfBound(LensData);
}

//////////////////////////////////////////////////////////////////////////
//
// Low level goodness
//
void FLiveLinkDragonMessageThread::InitiateHandshake()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	// should tokenize these strings
	JsonObject->SetStringField(CommandString, KeepAliveString);
	JsonObject->SetNumberField(VersionString, DragonAPIVersion);
	JsonObject->SetBoolField(DoNotPingString, true); // keep it from timing out
	SendMessageToServer( JsonObject );

	JsonObject->Values.Empty();

	JsonObject->SetStringField(CommandString, ViewFrameUpdatesString);
	JsonObject->SetBoolField(ActiveString, true);
	SendMessageToServer( JsonObject );

	bIsHandshook = true;

	// Call the delegate to let the rest of UE know that the handshake is complete
	HandshakeEstablishedDelegate.ExecuteIfBound();

	// After this, it sends a position and a 'ready to go' event

	// SubscribeToDeviceMetadataUpdates(EDragonDeviceType::TimecodeGenerator);
	// SubscribeToDeviceMetadataUpdates(EDragonDeviceType::Camera);
	// SubscribeToDeviceMetadataUpdates(EDragonDeviceType::Lens);
}

void FLiveLinkDragonMessageThread::SendMessageToServer(const TSharedPtr<FJsonObject> JsonObject)
{
	FString Msg;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Msg);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	
	bool OK;
	RemoteAddress->SetIp(*RemoteIP, OK);
	RemoteAddress->SetPort(RemotePort);

	int32 Sent;
	Socket->SendTo((uint8 *)TCHAR_TO_UTF8(*Msg), Msg.Len(), Sent, *RemoteAddress);

	if (Sent != Msg.Len())
		UE_LOG(LogLiveLinkDragonMessageThread, Warning, TEXT("Full message was not sent to the Dragon server %d vs %d"), Sent, Msg.Len());
}

void FLiveLinkDragonMessageThread::AcknowledgeMessageFromServer(const TArray<uint8> InMessageFromServer, const uint32 InServerMessageLength)
{
	UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Message from server..."));
}


void FLiveLinkDragonMessageThread::SubscribeToDeviceMetadataUpdates(const EDragonDeviceType InDeviceType)
{
	UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Device metadata update..."));
}

void FLiveLinkDragonMessageThread::SubscribeToVolatileDataUpdates(uint64 InDeviceID, const EDragonDeviceType InDeviceType)
{
	UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Device volatile data update..."));
}

void FLiveLinkDragonMessageThread::HashDataRequestMessage(const FArrayWriter InMessage, const FString InRequestName)
{
	UE_LOG(LogLiveLinkDragonMessageThread, Log, TEXT("Bleh update..."));
}
