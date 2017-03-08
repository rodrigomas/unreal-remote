// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Networking.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "Templates/SharedPointer.h"
#include "GameFramework/Actor.h"
#include "TCPListenerActor.generated.h"

UCLASS(Blueprintable)
class REMOTEGAME_API ATCPListenerActor : public AActor, public FRunnable
{
	GENERATED_BODY()
	
public:	
	
	FTimerHandle TCPTimerHandle;
	FTimerHandle TCPClientTimerHandle;

	ATCPListenerActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Settings")
	FString ServerIP = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server Settings")
	int32 ServerPort = 10000;

	UPROPERTY(EditAnywhere, Category = "Server Settings")
	int32 ReceiveBufferSize = 2048;

	UPROPERTY(EditAnywhere, Category = "Server Settings")
	float ThreadSleepTime = 0.03f;
	
	UFUNCTION(BlueprintCallable, Category = "Connection")
	bool Connect();

	UFUNCTION(BlueprintCallable, Category = "Connection")
	bool Disconnect();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection")
	void OnConnect();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection")
	void OnDisconnect();

	UFUNCTION(BlueprintCallable, Category = "Connection")
	bool IsConnected() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool Init();

	virtual uint32 Run();

	virtual void Stop();

	void EnsureCompletion();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	static void Shutdown();

private:
	FSocket* ClientSocket;
	FTcpListener* ListenerSocket;
	FSocket* ConnectionSocket;		

	// Singleton
	static  ATCPListenerActor* Runnable;	

	FRunnableThread* Thread;

	FThreadSafeCounter StopTaskCounter;

	FIPv4Endpoint RemoteAddressForConnection;

	void ProcessData();

	void TCPConnectionListener();

	bool ConnectionAccepted(FSocket* LocalClientSocket, const FIPv4Endpoint& ClientEndpoint);

	FString StringFromBinaryArray(TArray<uint8> BinaryArray);

	bool TextToIPArray(const FString& TheIP, uint8(&Out)[4]);
	
	
};
