// Copyright (c) RITMPS, Rochester Institute of Technology, 2022


#include "LiveLinkDragonSource.h"

#include "ILiveLinkClient.h"

#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Roles/LiveLinkCameraRole.h"
#include "Roles/LiveLinkCameraTypes.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h" // for the fluent socket builder

#include <cmath>
#include <chrono>

#define LOCTEXT_NAMESPACE "LiveLinkDragonSource"

DEFINE_LOG_CATEGORY_STATIC(LogLiveLinkDragonPlugin, Log, All);


// #define RECV_BUFFER_SIZE 1024 * 1024
using namespace std::chrono;

FLiveLinkDragonSource::FLiveLinkDragonSource(FLiveLinkDragonConnectionSettings InConnectionSettings)
	: SocketSubsystem(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	, ConnectionSettings(MoveTemp(InConnectionSettings))
	, LastTimeDataReceived(0.0)
	, bReceivedData(false)
{
	SourceMachineName = FText::Format(LOCTEXT("MachineName", "{0}:{1}"), 
        FText::FromString(ConnectionSettings.IPAddress), 
        FText::AsNumber(DragonPortNumber, 
       	 &FNumberFormattingOptions::DefaultNoGrouping()));
}

FLiveLinkDragonSource::~FLiveLinkDragonSource()
{
	RequestSourceShutdown();
}


// FLiveLinkDragonSource::FLiveLinkDragonSource(FIPv4Endpoint InEndpoint)
// : Socket(nullptr)
// , Stopping(false)
// , Thread(nullptr)
// , isRunning(false)
// , WaitTime(FTimespan::FromMilliseconds(100))
// {
//     UE_LOG(LogLiveLinkDragonPlugin, Warning, TEXT("%s"), *version);
    
// 	// defaults
// 	DeviceEndpoint = InEndpoint;
//     FIPv4Address::Parse("127.0.0.1", DeviceEndpoint.Address);
//     DeviceEndpoint.Port = 55533;

// 	SourceStatus = LOCTEXT("SourceStatus_DeviceNotFound", "Device Not Found");
// 	SourceType = LOCTEXT("LiveLinkDragonSourceType", "Dragonbridge LiveLink");
// 	SourceMachineName = LOCTEXT("LiveLinkDragonSourceMachineName", "localhost");

// 	//setup socket
// 	if (DeviceEndpoint.Address.IsMulticastAddress())
// 	{
// 		Socket = FUdpSocketBuilder(TEXT("JSONSOCKET"))
// 			.AsNonBlocking()
// 			.AsReusable()
// 			.BoundToPort(DeviceEndpoint.Port)
// 			.WithReceiveBufferSize(RECV_BUFFER_SIZE)

// 			.BoundToAddress(FIPv4Address::Any)
// 			.JoinedToGroup(DeviceEndpoint.Address)
// 			.WithMulticastLoopback()
// 			.WithMulticastTtl(2);
					
// 	}
// 	else
// 	{
// 		Socket = FUdpSocketBuilder(TEXT("JSONSOCKET"))
// 			.AsNonBlocking()
// 			.AsReusable()
// 			.BoundToAddress(DeviceEndpoint.Address)
// 			.BoundToPort(DeviceEndpoint.Port)
// 			.WithReceiveBufferSize(RECV_BUFFER_SIZE);
// 	}

// 	RecvBuffer.SetNumUninitialized(RECV_BUFFER_SIZE);

// 	if ((Socket != nullptr) && (Socket->GetSocketType() == SOCKTYPE_Datagram))
// 	{
// 		SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

// 		Start();

// 		SourceStatus = LOCTEXT("SourceStatus_Receiving", "Receiving");
//         isRunning = true;
// 	}
// }

// FLiveLinkDragonSource::~FLiveLinkDragonSource()
// {
// 	Stop();
// 	if (Thread != nullptr)
// 	{
// 		Thread->WaitForCompletion();
// 		delete Thread;
// 		Thread = nullptr;
// 	}
// 	if (Socket != nullptr)
// 	{
// 		Socket->Close();
// 		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
//         isRunning = false;
// 	}
// }

void FLiveLinkDragonSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	Client = InClient;

	SubjectKey = FLiveLinkSubjectKey(InSourceGuid, ConnectionSettings.SubjectName);

	FLiveLinkStaticDataStruct DragonStaticDataStruct(FLiveLinkCameraStaticData::StaticStruct());
	FLiveLinkCameraStaticData* DragonStaticData = DragonStaticDataStruct.Cast<FLiveLinkCameraStaticData>();

	DragonStaticData->bIsFocalLengthSupported = true;
	DragonStaticData->bIsApertureSupported = true;
	DragonStaticData->bIsFocusDistanceSupported = true;

	DragonStaticData->bIsFieldOfViewSupported = false;
	DragonStaticData->bIsAspectRatioSupported = false;
	DragonStaticData->bIsProjectionModeSupported = false;

	Client->PushSubjectStaticData_AnyThread(SubjectKey, ULiveLinkCameraRole::StaticClass(), MoveTemp(DragonStaticDataStruct));

	OpenConnection();
}

void FLiveLinkDragonSource::OnHandshakeEstablished_AnyThread()
{
	bReceivedData = true;
}

bool FLiveLinkDragonSource::IsSourceStillValid() const
{
	if (Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
	{
		return false;
	}
	else if (bReceivedData == false)
	{
		return false;
	}

	return true;
}

// void FLiveLinkDragonSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
// {
// 	Client = InClient;
// 	SourceGuid = InSourceGuid;
// }


// bool FLiveLinkDragonSource::IsSourceStillValid() const
// {
// 	// Source is valid if we have a valid thread and socket
// 	bool bIsSourceValid = !Stopping && Thread != nullptr && Socket != nullptr;
// 	return bIsSourceValid;
// }


// bool FLiveLinkDragonSource::RequestSourceShutdown()
// {
// 	Stop();

// 	return true;
// }



bool FLiveLinkDragonSource::RequestSourceShutdown()
{
	if (MessageThread)
	{
		MessageThread->Stop();
		MessageThread.Reset();
	}

	if (Socket)
	{
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
	}

	return true;
}

FText FLiveLinkDragonSource::GetSourceType() const
{
	return LOCTEXT("DragonSourceType", "Dragonframe");
}

FText FLiveLinkDragonSource::GetSourceStatus() const
{
	if (Socket->GetConnectionState() == ESocketConnectionState::SCS_ConnectionError)
	{
		return LOCTEXT("FailedConnectionStatus", "Failed to connect");
	}
	else if (bReceivedData == false)
	{
		return LOCTEXT("InvalidConnectionStatus", "Connected...waiting for handshake");
	}
	else if (FPlatformTime::Seconds() - LastTimeDataReceived > DataReceivedTimeout)
	{
		return LOCTEXT("WaitingForDataStatus", "Connected...waiting for data");
	}
	return LOCTEXT("ActiveStatus", "Active");
}

void FLiveLinkDragonSource::OpenConnection()
{
	check(!Socket);

	SocketSubsystem = nullptr;
	if (SocketSubsystem == nullptr)
		SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	FIPv4Address IPAddr;
	if (FIPv4Address::Parse(ConnectionSettings.IPAddress, IPAddr) == false)
	{
		UE_LOG(LogLiveLinkDragonPlugin, Warning, TEXT("Ill-formed IP Address"));
		return;
	}

	// need to parse this from the settings
	uint16 PortNumber = ConnectionSettings.Port;
	if (PortNumber == 0)
	{
		UE_LOG(LogLiveLinkDragonPlugin, Warning, TEXT("Ill-formed Port Number, %d, defaulting to %d"), PortNumber, DragonPortNumber);
		PortNumber = DragonPortNumber; // default port
	}

	// All good, let's a gooooo
	DragonEndpoint = FIPv4Endpoint(FIPv4Address::Any, PortNumber);

	// TSharedRef<FInternetAddr> Addr = DragonEndpoint.ToInternetAddr();

	Socket = FUdpSocketBuilder(TEXT("Dragon Socket"))
				 // .AsNonBlocking()
				 .AsReusable()
				 .BoundToEndpoint(DragonEndpoint)
				 .WithReceiveBufferSize(DragonBufferSize)
				 .WithSendBufferSize(DragonBufferSize)
				 .WithBroadcast();

	MessageThread = MakeUnique<FLiveLinkDragonMessageThread>(Socket);

	MessageThread->OnHandshakeEstablished_AnyThread().BindRaw(this, &FLiveLinkDragonSource::OnHandshakeEstablished_AnyThread);
	MessageThread->OnFrameDataReady_AnyThread().BindRaw(this, &FLiveLinkDragonSource::OnFrameDataReady_AnyThread);

	MessageThread->Start();
}

void FLiveLinkDragonSource::OnFrameDataReady_AnyThread(FLensPacket InData)
{
	FLiveLinkFrameDataStruct LensFrameDataStruct(FLiveLinkCameraFrameData::StaticStruct());
	FLiveLinkCameraFrameData* LensFrameData = LensFrameDataStruct.Cast<FLiveLinkCameraFrameData>();

	LastTimeDataReceived = FPlatformTime::Seconds();
	LensFrameData->WorldTime = LastTimeDataReceived.load();
	LensFrameData->MetaData.SceneTime = InData.FrameTime;
	LensFrameData->FocusDistance = InData.FocusDistance;
	LensFrameData->FocalLength = InData.FocalLength;
	LensFrameData->Aperture = InData.Aperture;
	LensFrameData->FieldOfView = InData.HorizontalFOV;

	Client->PushSubjectFrameData_AnyThread(SubjectKey, MoveTemp(LensFrameDataStruct));
}

#undef LOCTEXT_NAMESPACE



// FRunnable interface

// void FLiveLinkDragonSource::Start()
// {
// 	ThreadName = "Dragon UDP Receiver";
// 	ThreadName.AppendInt(FAsyncThreadIndex::GetNext());
	
// 	Thread = FRunnableThread::Create(this, *ThreadName, 128 * 1024, TPri_AboveNormal, FPlatformAffinity::GetPoolThreadMask());
    
//     UE_LOG(LogTemp, Warning, TEXT("Starting"));

// }

// void FLiveLinkDragonSource::Stop()
// {
// 	Stopping = true;
// }

// uint32 FLiveLinkDragonSource::Run()
// {
//     TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();

// 	while (!Stopping)
// 	{
// 		if (Socket->Wait(ESocketWaitConditions::WaitForRead, WaitTime))
// 		{
// 			uint32 Size;

// 			while (Socket->HasPendingData(Size))
// 			{
// 				int32 Read = 0;

// 				if (Socket->RecvFrom(RecvBuffer.GetData(), RecvBuffer.Num(), Read, *Sender))
// 				{
// 					if (Read > 0)
// 					{
// 						TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData = MakeShareable(new TArray<uint8>());
// 						ReceivedData->SetNumUninitialized(Read);
// 						memcpy(ReceivedData->GetData(), RecvBuffer.GetData(), Read);
// 						AsyncTask(ENamedThreads::GameThread, [this, ReceivedData]() { HandleReceivedData(ReceivedData); });
// 					}
// 				}
// 			}
// 		}
// 	}
// 	return 0;
// }
// void  FLiveLinkDragonSource::SetupSubjects(const FString JsonString, TArray<Subject> &SubjectList)
// {

//     FString InputJson =
// R"({ "sources": [{ 
//          "subject": "robot_camera", 
//              "properties": ["Roll", "Focus", "Zoom"],
//              "propertyIndex": [6, 7, 8],
//              "bones" : [{ 
//                  "name": "top", 
//                  "parent" : ""  ,
//                  "index": [-1, -1, -1, -1, -1, -1]
//               }, 
//               { 
//                  "name": "CameraPose", 
//                  "parent" : "top",
//                  "index": [0, 1, 2, 3, 4, 5]
//               }] 
//          },
//          { 
//          "subject": "camera_target",
//             "properties": ["CameraTarget_xt", "CameraTarget_yt", "CameraTarget_zt"], 
//              "propertyIndex": [9, 10, 11],
//             "bones" : [{ 
//                  "name": "top", 
//                  "parent" : "" ,
//                  "index": [-1, -1, -1, -1, -1, -1]
//             }, 
//             { 
//                  "name": "CameraTarget", 
//                  "parent" : "top" ,
//                  "index": [ 9, 10, 11, -1, -1, -1]
//             }]
//          }] 
// })";



//    // UE_LOG(LogTemp, Warning, TEXT("%s"), *InputJson);
//     UE_LOG(LogTemp, Warning, TEXT("%s"), *InputJson);

//     TSharedPtr<FJsonObject> JsonObject;
//     TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputJson);
//     if (FJsonSerializer::Deserialize(Reader, JsonObject))
//     {
//         auto SubjectArray = JsonObject->GetArrayField(TEXT("sources"));
//         for (auto Subj : SubjectArray) {
//             Subject NewSubject;
//             auto SubjectObject = Subj->AsObject();
//             FName SubjectName = *SubjectObject->GetStringField("subject");
//             NewSubject.SubjectName = SubjectName;

//             FLiveLinkStaticDataStruct StaticDataStruct = FLiveLinkStaticDataStruct(FLiveLinkSkeletonStaticData::StaticStruct());
//             FLiveLinkSkeletonStaticData& StaticData = *StaticDataStruct.Cast<FLiveLinkSkeletonStaticData>();
//             Client->RemoveSubject_AnyThread({ SourceGuid, SubjectName });

//             auto BoneArray = SubjectObject->GetArrayField(TEXT("bones"));

//             StaticData.BoneNames.Reset(BoneArray.Num());
//             StaticData.BoneParents.Reset(BoneArray.Num());

//             FString sname;
//             int BoneIdx = 0;
//             for (auto BoneItem : BoneArray) {
//                 Bone NewBone;
//                 auto BoneObject = BoneItem->AsObject();

//                 FString BoneName;
//                 if (BoneObject->TryGetStringField(TEXT("name"), BoneName))
//                 {
//                     NewBone.Name = BoneName;
//                     NewBone.Id = BoneIdx;
//                 }

//                 FString BoneParent;
//                 if (BoneObject->TryGetStringField("parent", BoneParent))
//                 {
//                     NewBone.ParentName = BoneParent;
//                     if (!BoneParent.IsEmpty()) {
//                         NewBone.IsRoot = false;
//                     }
//                 }
//                 auto IndexArray = BoneObject->GetArrayField(TEXT("index"));
//                 for (int i = 0; i < 6; i++) {
//                     if (i < IndexArray.Num()) {
//                         NewBone.Index[i] = IndexArray[i]->AsNumber();
//                         //UE_LOG(LogTemp, Warning, TEXT("Index:%d"), NewBone.Index[i]);
//                     }
//                 }
//                 BoneIdx++;
//                 NewSubject.Bones.Add(NewBone);

//                 StaticData.BoneNames.Add(*NewBone.Name);
//                 auto parent = NewBone.IsRoot ? INDEX_NONE : 0;
//                 StaticData.BoneParents.Add(parent);
//             }
//             auto PropertyArray = SubjectObject->GetArrayField(TEXT("properties"));
//             auto PropertyIndexArray = SubjectObject->GetArrayField(TEXT("propertyIndex"));
//             int pcnt = 0;
//             for (auto Prop : PropertyArray) {
//                 BoneProperty NewBoneProperty;
//                 NewBoneProperty.Name = Prop->AsString();
//                 UE_LOG(LogTemp, Warning, TEXT("Property:%s"), *NewBoneProperty.Name);
//                 if (pcnt < PropertyIndexArray.Num()) {
//                     NewBoneProperty.Index = PropertyIndexArray[pcnt++]->AsNumber();
//                     //UE_LOG(LogTemp, Warning, TEXT("Index:%d"), NewBoneProperty.Index);
//                 }
//                 NewSubject.Properties.Add(NewBoneProperty);
//                 StaticData.PropertyNames.Add(*NewBoneProperty.Name);
//             }
//             SubjectList.Add(NewSubject);

//             Client->PushSubjectStaticData_AnyThread({ SourceGuid, SubjectName },
//                 ULiveLinkAnimationRole::StaticClass(),
//                 MoveTemp(StaticDataStruct));

//         }
//     }
// }

// static bool SkipFrame(FQualifiedFrameTime &SceneTime)
// {
//     double CurrentSeconds = FPlatformTime::Seconds();
//     FFrameRate FrameRate = FApp::GetTimecodeFrameRate();
//     FTimecode TimeCode = FTimecode(CurrentSeconds, FrameRate, true);
//     SceneTime = FQualifiedFrameTime(TimeCode,FrameRate);

//     static double LastSeconds = 0.0;
//     //static high_resolution_clock::time_point Time1 = high_resolution_clock::now();

//     //UE_LOG(LogTemp, Warning, TEXT("source time: %f"), CurrentSeconds);
//     //UE_LOG(LogTemp, Warning, TEXT("Timecode: %s"), *TimeCode.ToString());

//     double SecondRate = 1.0 / static_cast<double>(FrameRate.Numerator);

//     high_resolution_clock::time_point Time2 = high_resolution_clock::now();

//     //duration<double> time_span = duration_cast<duration<double>>(Time2 - Time1);
//     //UE_LOG(LogTemp, Warning, TEXT("delta: %f, time span:%f"), CurrentSeconds - LastSeconds,time_span.count());
//     if (CurrentSeconds < LastSeconds + SecondRate)
//     {
//         //UE_LOG(LogTemp, Warning, TEXT("Skipping             %f"), CurrentSeconds);
//         return true;
//     } else 
//     {
//     //UE_LOG(LogTemp, Warning, TEXT("Frame rate 1.0/%d"), FrameRate.Numerator);
//         LastSeconds = CurrentSeconds;
//         //Time1 = Time2;
//     }
//     return false;
// }

// void FLiveLinkDragonSource::SendFrameToLiveLink(const TArray<Subject> SubjectList, const TArray<float> FrameValueList)
// {

//     FQualifiedFrameTime SceneTime;
//     if (SkipFrame(SceneTime))
//     {
//         return;
//     }
//     for (auto Subj : SubjectList) {
//         FName SubjectName = Subj.SubjectName;
//         //UE_LOG(LogTemp, Warning, TEXT("SubjectList: %s"), *SubjectName.ToString());
//         FLiveLinkFrameDataStruct FrameDataStruct = FLiveLinkFrameDataStruct(FLiveLinkAnimationFrameData::StaticStruct());
//         FLiveLinkAnimationFrameData& FrameData = *FrameDataStruct.Cast<FLiveLinkAnimationFrameData>();

//         FrameData.Transforms.Reserve(Subj.Bones.Num());  
//         FrameData.WorldTime = FPlatformTime::Seconds();
//         FrameData.MetaData.SceneTime = SceneTime;

//         for (auto Bone : Subj.Bones) {
//             //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
//             //UE_LOG(LogTemp, Warning, TEXT("Parent:%s id:%d"), *Bone.ParentName,Bone.Id);
//             FTransform Trans;
//             FVector Location(0.0f, 0.0f, 0.0f);
//             FVector Rotation(0.0f, 0.0f, 0.0f);
//             int ValueSize = FrameValueList.Num();
//             int Xdx = Bone.Index[0];
//             int Ydx = Bone.Index[1];
//             int Zdx = Bone.Index[2];
//             if (Xdx > -1 && Ydx > -1 && Zdx > -1 &&
//                 Xdx < ValueSize && Ydx < ValueSize && Zdx < ValueSize) {
//                 Location.Set(FrameValueList[Xdx], FrameValueList[Ydx], FrameValueList[Zdx]);
//                 //UE_LOG(LogTemp, Warning, TEXT("Loc:%f %f %f"), Location.X,Location.Y,Location.Z);
//                 //UE_LOG(LogTemp, Warning, TEXT("Rot:%f %f %f"), Rotation.X,Rotation.Y,Rotation.Z);
//                 if (Bone.Name == "CameraPose") {
//                     //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
//                     double CurrentSeconds = FPlatformTime::Seconds();
//                     static double LastCurrentSeconds = 0.0;
//                     //UE_LOG(LogTemp, Warning, TEXT("%0.4f,%0.2f,%0.2f,%0.2f"), CurrentSeconds - LastCurrentSeconds, Location.X, Location.Y, Location.Z);
//                     LastCurrentSeconds = CurrentSeconds;
//                 }
//             }
//             int rXdx = Bone.Index[3];
//             int rYdx = Bone.Index[4];
//             int rZdx = Bone.Index[5];
//             if (rXdx > -1 && rYdx > -1 && rZdx > -1 &&
//                 rXdx < ValueSize && rYdx < ValueSize && rZdx < ValueSize) {
//                 Rotation.Set(FrameValueList[rXdx], FrameValueList[rYdx], FrameValueList[rZdx]);
//             }
//             Trans.SetLocation(Location);
//             if (Rotation.Size() > 0) {
//                 Trans.SetRotation(FQuat::MakeFromEuler(Rotation));
//                 //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
//                 //UE_LOG(LogTemp, Warning, TEXT("Rot:%f %f %f"), Rotation.X,Rotation.Y,Rotation.Z);
//             }
//             FrameData.Transforms.Add(Trans);
//         }
//         for (auto Prop : Subj.Properties) {
//             if (Prop.Index > -1 && Prop.Index < FrameValueList.Num()) {
//                 float Value = FrameValueList[Prop.Index];
//                 //UE_LOG(LogTemp, Warning, TEXT("Property:%s index:%d"), *Prop.Name, Prop.Index);
//                 //UE_LOG(LogTemp, Warning, TEXT("Value %f"), value);
//                 FrameData.PropertyValues.Add(Value);
//             }
//         }
//         Client->PushSubjectFrameData_AnyThread({ SourceGuid, SubjectName }, MoveTemp(FrameDataStruct));
//     }
// }

// void FLiveLinkDragonSource::HandleReceivedData(TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData)
// {
//     if (Stopping) {
//         return; // thread is shutting down
//     }
// 	FString JsonString;
// 	RobotData Robot_Data;
// 	JsonString.Empty(ReceivedData->Num());
// 	for (uint8& Byte : *ReceivedData.Get())
// 	{
// 		JsonString += TCHAR(Byte);
// 	}

//     UE_LOG(LogTemp, Warning, TEXT("ReceivedData: %s"), *JsonString);

//     // Keepalive message
//     TSharedPtr<FJsonObject> JsonObject;
// 	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
// 	if (FJsonSerializer::Deserialize(Reader, JsonObject))
// 	{
// 		for (TPair<FString, TSharedPtr<FJsonValue>>& JsonField : JsonObject->Values)
// 		{
// 			FName KeyName(*JsonField.Key);

//             if (KeyName == "event" && JsonField.Value->AsString() == "hello") {
//                 UE_LOG(LogTemp, Warning, TEXT("Handshake"));

//                 // Send handshake
//                 FString HandshakeJason = R"({"command":"hello","version":1.0})";
//             	TSharedPtr<FJsonObject> JsonData;

//                 // Serialize data to json text
//                 TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&HandshakeJason);
//                 if(!FJsonSerializer::Serialize(JsonData.ToSharedRef(), JsonWriter))
//                 {
//                     UE_LOG(LogTemp, Error, TEXT("%s - Couldn't serialize json data"));
//                     return;
//                 }

//                 // int32 BytesSent;
// 	            // Socket->SendTo( *HandshakeJason, HandshakeJason.Len(), BytesSent, *Sender);
//                 return;
//             }
//  			//const TArray<TSharedPtr<FJsonValue>>& BoneArray = JsonField.Value->AsArray();
//         }
//     }

    
// 	if (ReceivedData->Num() > 35) {
// 		memcpy(&Robot_Data, ReceivedData->GetData(), 36);
// 	}

//     if (NeedSubjectSeup) {
//         SetupSubjects(FString(""), Subjects);
//         NeedSubjectSeup = false;
//     }
//     FrameValues.Empty(); // init the FrameValues
//     // first lets manually setup Subj Values
// 	FVector CameraPose;
// 	FVector CameraTarget;
// 	const float METER2CENT = 100.0f;

//     // adjust Robot_data for different coord system
//     Robot_Data.yv = -Robot_Data.yv; // flip Y axis
//     Robot_Data.yt = -Robot_Data.yt; // flip Y axis


//     CameraPose = FVector(Robot_Data.xv * METER2CENT,
//                          Robot_Data.yv * METER2CENT,
//                          Robot_Data.zv * METER2CENT); // CameraPose
//     CameraTarget = FVector(Robot_Data.xt * METER2CENT,
//                            Robot_Data.yt * METER2CENT,
//                            Robot_Data.zt * METER2CENT);
//     FVector LookAt = CameraTarget - CameraPose;
//     float PanX = LookAt.X;
//     float PanY = LookAt.Y;
//     float Pan = FMath::RadiansToDegrees(atan2(PanY, PanX)); // atan2 returns radians // flip z rotation for pan
//     float TiltX = FVector(LookAt.X, LookAt.Y, 0.0).Size();
//     float TiltY = LookAt.Z;
//     float Tilt = FMath::RadiansToDegrees(atan2(TiltY, TiltX)); // atan2 returns radians
//     float Roll = -Robot_Data.roll;  //reverse roll for UE after test

//     FrameValues.Add(CameraPose.X);     // 0
//     FrameValues.Add(CameraPose.Y);     // 1
//     FrameValues.Add(CameraPose.Z);     // 2
//     // roll is xrot, tilt is yrot, pan is zrot
//     FrameValues.Add(FMath::RadiansToDegrees(Roll));             // 3
//     FrameValues.Add(Tilt);             // 4
//     FrameValues.Add(Pan);              // 5
//     FrameValues.Add(Robot_Data.roll);  // 6
//     //FrameValues.Add(Robot_Data.focus * METER2CENT); // 7
//     FrameValues.Add(LookAt.Size()); // 7  // use LookAt because Robot focus distance is wrong
//     FrameValues.Add(Robot_Data.zoom);  // 8
//     FrameValues.Add(CameraTarget.X);  // 9
//     FrameValues.Add(CameraTarget.Y);  // 10 
//     FrameValues.Add(CameraTarget.Z);  // 11 

//     SendFrameToLiveLink(Subjects, FrameValues);
// }
// #undef LOCTEXT_NAMESPACE
