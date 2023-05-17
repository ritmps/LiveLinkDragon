#include "udp_module.h"
#include <string>

Audp_module::Audp_module()
{
	PrimaryActorTick.bCanEverTick = true;
	Socket = nullptr;
}

Audp_module::~Audp_module()
{
}

void Audp_module::BeginPlay()
{
	Super::BeginPlay();
	SocketSubsystem = nullptr;
	if (SocketSubsystem == nullptr)	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	SendSize = 2 * 1024 * 1024;
	BufferSize = 2 * 1024 * 1024;

	LocalEndpoint = FIPv4Endpoint(FIPv4Address::Any, LocalPort);

	FIPv4Address::Parse(IP, RemoteAddress);
	RemoteEndpoint = FIPv4Endpoint(RemoteAddress, RemotePort);

	Socket = nullptr;

	if (SocketSubsystem != nullptr)
	{
		if (Socket == nullptr)
		{
			Socket = FUdpSocketBuilder(SocketDescription)
				.AsNonBlocking()
				.AsReusable()
				.BoundToEndpoint(LocalEndpoint)
				.WithReceiveBufferSize(SendSize)
				.WithReceiveBufferSize(BufferSize)
				.WithBroadcast();
		}
	}
}

void Audp_module::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	SocketSubsystem->DestroySocket(Socket);
	Socket = nullptr;
	SocketSubsystem = nullptr;
}

void Audp_module::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Listen(); // Listen for messages

	FString t = "test222";
	sendMessage(t); // Send Message Test
}

void Audp_module::Listen()
{
	TSharedRef<FInternetAddr> targetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	uint32 Size;
	while (Socket->HasPendingData(Size))
	{
		uint8* Recv = new uint8[Size];
		int32 BytesRead = 0;

		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
		Socket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *targetAddr);

		char ansiiData[1024];
		memcpy(ansiiData, ReceivedData.GetData(), BytesRead);
		ansiiData[BytesRead] = 0;

		FString data = ANSI_TO_TCHAR(ansiiData);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Message by UDP: " + data);
	}
}

bool Audp_module::sendMessage(FString Message)
{
	if (!Socket) return false;
	int32 BytesSent;

	FTimespan waitTime = FTimespan(10);

	TCHAR* serializedChar = Message.GetCharArray().GetData();
	int32 size = FCString::Strlen(serializedChar);

	bool success = Socket->SendTo((uint8*)TCHAR_TO_UTF8(serializedChar), size, BytesSent, *RemoteEndpoint.ToInternetAddr());
	UE_LOG(LogTemp, Warning, TEXT("Sent message: %s : %s : Address - %s : BytesSent - %d"), *Message, (success ? TEXT("true"):TEXT("false")), *RemoteEndpoint.ToString(), BytesSent);

	if (success && BytesSent > 0) return true;
	else return false;
}

void Audp_module::messageQueue()
{
}
