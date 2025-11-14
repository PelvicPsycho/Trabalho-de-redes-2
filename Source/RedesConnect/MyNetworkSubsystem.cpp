// Fill out your copyright notice in the Description page of Project Settings.

#include "MyNetworkSubsystem.h"
#include "TimerManager.h"

void UMyNetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	Socket = nullptr;
	RemoteAddress = nullptr;
	
	UE_LOG(LogTemp, Log, TEXT("MyNetworkSubsystem initialized"));
}

void UMyNetworkSubsystem::Deinitialize()
{
	Disconnect();
	
	Super::Deinitialize();
	
	UE_LOG(LogTemp, Log, TEXT("MyNetworkSubsystem deinitialized"));
}

bool UMyNetworkSubsystem::ConnectToServer(const FString& IPAddress, int32 Port)
{
	if (Socket)
	{
		Disconnect();
	}
	
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get socket subsystem"));
		return false;
	}

	Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("default"), false);
	if (!Socket)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create socket"));
		return false;
	}

	FIPv4Address IPAddr;
	if (!FIPv4Address::Parse(IPAddress, IPAddr))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid IP address: %s"), *IPAddress);
		CleanupSocket();
		return false;
	}

	RemoteAddress = SocketSubsystem->CreateInternetAddr();
	RemoteAddress->SetIp(IPAddr.Value);
	RemoteAddress->SetPort(Port);
	
	bool bConnected = Socket->Connect(*RemoteAddress);
	
	if (bConnected)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully connected to %s:%d"), *IPAddress, Port);
		
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				ReceiveTimerHandle,
				this,
				&UMyNetworkSubsystem::ReceiveData,
				0.01f,
				true
			);
		}
		
		OnConnected.Broadcast();
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connect to %s:%d"), *IPAddress, Port);
		CleanupSocket();
		return false;
	}
}

void UMyNetworkSubsystem::Disconnect()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReceiveTimerHandle);
	}

	CleanupSocket();
	
	OnDisconnected.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("Disconnected from server"));
}

bool UMyNetworkSubsystem::IsConnected() const
{
	return Socket != nullptr && Socket->GetConnectionState() == SCS_Connected;
}

bool UMyNetworkSubsystem::SendMessage(const FString& Message)
{
	if (!IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot send message: not connected"));
		return false;
	}
	
	FTCHARToUTF8 Convert(*Message);
	const uint8* Data = (const uint8*)Convert.Get();
	int32 BytesToSend = Convert.Length();
	int32 BytesSent = 0;
	
	bool bSuccess = Socket->Send(Data, BytesToSend, BytesSent);

	if (bSuccess && BytesSent == BytesToSend)
	{
		UE_LOG(LogTemp, Log, TEXT("Sent message: %s (%d bytes)"), *Message, BytesSent);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to send message. Sent %d of %d bytes"), BytesSent, BytesToSend);
		return false;
	}	
}

void UMyNetworkSubsystem::ReceiveData()
{
	if (!IsConnected())
	{
		return;
	}

	uint32 PendingDataSize = 0;
	
	if (Socket->HasPendingData(PendingDataSize))
	{
		TArray<uint8> ReceivedData;
		ReceivedData.SetNumUninitialized(PendingDataSize);

		int32 BytesRead = 0;
		if (Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead))
		{
			if (BytesRead > 0)
			{
				FString ReceivedMessage = FString(UTF8_TO_TCHAR((const char*)ReceivedData.GetData()));
				ReceivedMessage = ReceivedMessage.Left(BytesRead);

				UE_LOG(LogTemp, Log, TEXT("Received message: %s (%d bytes)"), *ReceivedMessage, BytesRead);
				
				OnMessageReceived.Broadcast(ReceivedMessage);
			}
		}
	}
}

void UMyNetworkSubsystem::CleanupSocket()
{
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
	
	RemoteAddress = nullptr;
}