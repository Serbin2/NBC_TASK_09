// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBPlayerController.generated.h"

class UNBChatWidget;
/**
 * 
 */
UCLASS()
class NUMBERBASEBALL_API ANBPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

	void SendChatMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FString& SenderName, const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientReceiveSystemMessage(const FString& Message, FLinearColor Color);

protected:
	UPROPERTY(EditDefaultsOnly, Category="NumberGame|Chat")
	TSubclassOf<UNBChatWidget> ChatInputWidgetClass;

	UPROPERTY()
	TObjectPtr<UNBChatWidget> ChatInputWidgetInstance;

private:
	void HandleCommand(const FString& CommandInput);

	//	유효한 추측 숫자인지 검사한다. (세 자리, 각 자리 숫자가 모두 서로 다름)
	bool IsValidGuessNumber(const FString& Number) const;

	//	본인 화면에만 표시하는 로컬 시스템 메시지. (네트워크 전송 없음)
	void ShowLocalSystemMessage(const FString& Message, FLinearColor Color = FLinearColor::Red) const;

	UFUNCTION(Server, Reliable)
	void ServerSendMessage(const FString& Message);

	UFUNCTION(Server, Reliable)
	void ServerRequestGameOn();

	//	게임 로직 패킷: 클라이언트가 검증한 추측 숫자를 서버로 전송한다.
	UFUNCTION(Server, Reliable)
	void ServerSubmitNumber(const FString& Number);
};
