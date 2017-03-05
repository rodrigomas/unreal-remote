// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Networking.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "Templates/SharedPointer.h"
#include "GameFramework/Actor.h"
#include "TCPListenerActor.generated.h"

UCLASS()
class REMOTEGAME_API ATCPListenerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	FTimerHandle TCPTimerHandle;

	ATCPListenerActor();

	UPROPERTY(EditAnywhere)
	FString ServerIP = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere)
	int32 ServerPort = 10000;

	UPROPERTY(EditAnywhere)
	int32 ReceiveBufferSize = 2048;

	bool Connect();

	bool Disconnect();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	FSocket* ClientSocket;
	FSocket* ConnectionSocket;		

	FIPv4Endpoint RemoteAddressForConnection;

	void ProcessData();

	void TCPConnectionListener();

	FString StringFromBinaryArray(TArray<uint8> BinaryArray);

	bool FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4]);
	
	
};
