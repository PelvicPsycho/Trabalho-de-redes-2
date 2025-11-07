// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "MyNetworkSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnected);

/**
 * UMyNetworkSubsystem is a custom network subsystem derived from UNetworkSubsystem,
 * which extends the functionality of the base class specific to the REDESCONNECT project.
 *
 * This class is designed to manage network-related operations and configurations
 * tailored to the needs of a custom application or system. It provides the
 * necessary framework to implement specialized behavior for networking subsystems.
 *
 * Inherit from this class to implement or override existing network subsystem features.
 *
 * This subsystem is intended to be used within the scope of the REDESCONNECT module.
 */

//class REDESCONNECT_API UMyNetworkSubsystem : public UNetworkSubsystem
/**
 * UMyNetworkSubsystem is a custom network subsystem for connecting to third-party socket servers.
 * It handles TCP socket connections, sending messages, and receiving data from external servers.
 */
UCLASS()
class REDESCONNECT_API UMyNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Subsystem lifecycle
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Connection management
	UFUNCTION(BlueprintCallable, Category = "Network")
	bool ConnectToServer(const FString& IPAddress, int32 Port);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void Disconnect();

	UFUNCTION(BlueprintPure, Category = "Network")
	bool IsConnected() const;

	// Message sending
	UFUNCTION(BlueprintCallable, Category = "Network")
	bool SendMessage(const FString& Message);

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnMessageReceived OnMessageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnConnected OnConnected;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnDisconnected OnDisconnected;

private:
	// Socket members
	FSocket* Socket;
	TSharedPtr<FInternetAddr> RemoteAddress;
	
	// Tick for receiving messages
	FTimerHandle ReceiveTimerHandle;
	
	void ReceiveData();
	void CleanupSocket();
};