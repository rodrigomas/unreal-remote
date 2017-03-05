// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteGame.h"
#include "TCPListenerActor.h"
#include "TimerManager.h"

// Sets default values
ATCPListenerActor::ATCPListenerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

bool ATCPListenerActor::Connect()
{
	uint8 IP4Nums[4];
	if (!FormatIP4ToNumber(ServerIP, IP4Nums))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TCP Client>> Invalid IP! Expecting 4 parts separated by .")));
		return false;
	}

	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), ServerPort);

	ClientSocket = FTcpSocketBuilder(TEXT("TCPClient"))
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	//Set Buffer Size
	int32 NewSize = 0;
	ClientSocket->SetReceiveBufferSize(ReceiveBufferSize, NewSize);

	if (!ClientSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TCP Client>> Client socket could not be created!: %s %d"), *ServerIP, ServerPort));
		return false;
	}

	GetWorldTimerManager().SetTimer(TCPTimerHandle, this, &ATCPListenerActor::TCPConnectionListener, 0.01, true);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TCP Client>> Server Initialized")));
	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Server Initialized"));

	return true;
}

bool ATCPListenerActor::Disconnect()
{
	try
	{
		if (ConnectionSocket)
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}

		if (ClientSocket)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		}

		GetWorld()->GetTimerManager().ClearTimer(TCPTimerHandle);
	}
	catch (...)
	{

	}

	return true;
}

void ATCPListenerActor::TCPConnectionListener()
{
	if (!ClientSocket) return;

	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	//UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Server Listening"));

	// handle incoming connections
	if (ClientSocket->HasPendingConnection(Pending) && Pending)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCP Client>> New Connection"));
		//Already have a Connection? destroy previous
		if (ConnectionSocket)
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}

		//New Connection receive!
		ConnectionSocket = ClientSocket->Accept(*RemoteAddress, TEXT("TCP Received Socket Connection"));

		UE_LOG(LogTemp, Warning, TEXT("TCP Received Socket Connection"));

		if (ConnectionSocket != NULL)
		{
			//Global cache of current Remote Address
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);

			//can thread this too
			GetWorldTimerManager().SetTimer(TCPTimerHandle, this, &ATCPListenerActor::ProcessData, 0.01, true);
		}
	}
}

void ATCPListenerActor::ProcessData()
{
	if (!ConnectionSocket) return;

	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> New Data From Client"));

	//Binary Array!
	TArray<uint8> ReceivedData;

	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		ReceivedData.Init(0, FMath::Min(Size, 65507u));

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Read! %d"), ReceivedData.Num()));
	}

	if (ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Bytes Read: %d"), ReceivedData.Num()));

	const FString ReceivedUE4String = StringFromBinaryArray(ReceivedData);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("As String Data ~> %s"), *ReceivedUE4String));

	UE_LOG(LogTemp, Warning, TEXT("As String Data ~> %s"), *ReceivedUE4String);
}

bool ATCPListenerActor::FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4])
{
	//IP Formatting
	TheIP.Replace(TEXT(" "), TEXT(""));

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//						   IP 4 Parts

	//String Parts
	TArray<FString> Parts;
	TheIP.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() != 4)
		return false;

	//String to Number Parts
	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}

	return true;
}

FString ATCPListenerActor::StringFromBinaryArray(TArray<uint8> BinaryArray)
{
	BinaryArray.Add(0); // Add 0 termination. Even if the string is already 0-terminated, it doesn't change the results.
						// Create a string from a byte array. The string is expected to be 0 terminated (i.e. a byte set to 0).
						// Use UTF8_TO_TCHAR if needed.
						// If you happen to know the data is UTF-16 (USC2) formatted, you do not need any conversion to begin with.
						// Otherwise you might have to write your own conversion algorithm to convert between multilingual UTF-16 planes.
	return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(BinaryArray.GetData())));
}

// Called when the game starts or when spawned
void ATCPListenerActor::BeginPlay()
{
	Super::BeginPlay();
	
	try
	{
		//Connect();
	}
	catch (...)
	{

	}
}

// Called every frame
void ATCPListenerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

