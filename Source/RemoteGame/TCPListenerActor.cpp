// Fill out your copyright notice in the Description page of Project Settings.

#include "RemoteGame.h"
#include "TCPListenerActor.h"
#include "TimerManager.h"
#include "Async.h"

ATCPListenerActor* ATCPListenerActor::Runnable = NULL;

// Sets default values
ATCPListenerActor::ATCPListenerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;	
}

bool ATCPListenerActor::Init()
{

	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Worker Thread Initialized"));
	return true;
}

uint32 ATCPListenerActor::Run()
{
	FPlatformProcess::Sleep(0.03);

	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Worker Thread Running"));

	while (StopTaskCounter.GetValue() == 0)
	{
		ProcessData();

		FPlatformProcess::Sleep(ThreadSleepTime);
		//AsyncTask(ENamedThreads::GameThread, []() {
			
		//});
		
	}

	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Worker Thread Finished"));

	return 0;
}

void ATCPListenerActor::Stop()
{
	StopTaskCounter.Increment();
}

void ATCPListenerActor::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

void ATCPListenerActor::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

bool ATCPListenerActor::Connect()
{	
	uint8 IP4Nums[4];
	if (!TextToIPArray(ServerIP, IP4Nums))
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

	ListenerSocket = new FTcpListener(*ClientSocket);

	if (!ListenerSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TCP Client>> Client socket could not be created!: %s %d"), *ServerIP, ServerPort));
		return false;
	}

	ConnectionSocket = NULL;

	ListenerSocket->OnConnectionAccepted().BindUObject(this, &ATCPListenerActor::ConnectionAccepted);

	GetWorldTimerManager().ClearTimer(TCPTimerHandle);
	//GetWorldTimerManager().SetTimer(TCPTimerHandle, this, &ATCPListenerActor::TCPConnectionListener, 0.01, true);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TCP Client>> Server Initialized")));
	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Server Initialized"));

	ListenerSocket->Init();

	StopTaskCounter.Reset();

	Thread = FRunnableThread::Create(this, TEXT("TCPListenerThread"), 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify more

	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = this;
	}

	//GetWorldTimerManager().ClearTimer(TCPClientTimerHandle);
	//GetWorldTimerManager().SetTimer(TCPClientTimerHandle, this, &ATCPListenerActor::ProcessData, 0.01, true);

	return true;
}

bool ATCPListenerActor::ConnectionAccepted(FSocket* LocalClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> New Connection"));

	try
	{
		if (ConnectionSocket)
		{
			FSocket * lsocket = ConnectionSocket;
			ConnectionSocket = NULL;

			lsocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(lsocket);
		}

	} catch(...)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error on socket destruction"));
	}


	ConnectionSocket = LocalClientSocket;

	UE_LOG(LogTemp, Warning, TEXT("TCP Received Socket Connection"));

	try
	{
		if (ConnectionSocket != NULL)
		{
			RemoteAddressForConnection = FIPv4Endpoint(ClientEndpoint.Address, ClientEndpoint.Port);
			//GetWorldTimerManager().ClearTimer(TCPClientTimerHandle);
			//GetWorldTimerManager().SetTimer(TCPClientTimerHandle, this, &ATCPListenerActor::ProcessData, 0.01, true);

			OnConnect();
		}
	} catch(...)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error on socket binding"));
	}

	return true;
}

void ATCPListenerActor::OnConnect_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Server Connected"));
}

void ATCPListenerActor::OnDisconnect_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("TCP Client>> Server Disconnected"));
}

bool ATCPListenerActor::IsConnected() const
{
	return (ConnectionSocket != NULL && ListenerSocket != NULL && ListenerSocket->IsActive());
}

bool ATCPListenerActor::Disconnect()
{
	try
	{

		if (ConnectionSocket)
		{
			ConnectionSocket->Close();
			//ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
			OnDisconnect();
		}

		if (ClientSocket)
		{
			
			ListenerSocket->Stop();
			ClientSocket->Close();
			//ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		}

		Stop();

		GetWorld()->GetTimerManager().ClearTimer(TCPTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(TCPClientTimerHandle);
	}
	catch (...)
	{

	}

	return true;
}

void ATCPListenerActor::TCPConnectionListener()
{
	if (!ClientSocket) return;

	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	if (ClientSocket->HasPendingConnection(Pending) && Pending)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCP Client>> New Connection"));

		if (ConnectionSocket)
		{
			FSocket * lsocket = ConnectionSocket;
			ConnectionSocket = NULL;

			lsocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(lsocket);
		}

		ConnectionSocket = ClientSocket->Accept(*RemoteAddress, TEXT("TCP Received Socket Connection"));

		UE_LOG(LogTemp, Warning, TEXT("TCP Received Socket Connection"));

		if (ConnectionSocket != NULL)
		{
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);
			//GetWorldTimerManager().ClearTimer(TCPClientTimerHandle);
			//GetWorldTimerManager().SetTimer(TCPClientTimerHandle, this, &ATCPListenerActor::ProcessData, 0.01, true);

			OnConnect();
		}
	}
}

void ATCPListenerActor::ProcessData()
{
	try
	{
		if (!ConnectionSocket) return;

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
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("TCP Client>> New Data From Client"));

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Bytes Read: %d"), ReceivedData.Num()));

		const FString ReceivedUE4String = StringFromBinaryArray(ReceivedData);

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("As String Data ~> %s"), *ReceivedUE4String));

		UE_LOG(LogTemp, Warning, TEXT("As String Data ~> %s"), *ReceivedUE4String);
	}
	catch (...)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error Proc"));
	}
}

bool ATCPListenerActor::TextToIPArray(const FString& TheIP, uint8(&Out)[4])
{
	TheIP.Replace(TEXT(" "), TEXT(""));

	TArray<FString> Parts;
	TheIP.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() != 4)
		return false;

	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}

	return true;
}

FString ATCPListenerActor::StringFromBinaryArray(TArray<uint8> BinaryArray)
{
	BinaryArray.Add(0);						
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

void ATCPListenerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	try
	{
		if(IsConnected())
		{
			Disconnect();
		}

		Stop();
		//Connect();
	}
	catch (...)
	{

	}
}

