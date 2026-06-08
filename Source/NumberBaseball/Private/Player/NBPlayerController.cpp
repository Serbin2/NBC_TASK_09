// Fill out your copyright notice in the Description page of Project Settings.


#include "NumberBaseball/Public/Player/NBPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "NumberBaseball/Public/Game/NBGameModeBase.h"
#include "NumberBaseball/Public/Game/NBGameState.h"
#include "NumberBaseball/Public/Player/NBPlayerState.h"
#include "UI/NBChatWidget.h"

void ANBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//	로컬에서만 UI 생성하겠다.
	if (!IsLocalController())	return;

	FInputModeUIOnly InputModeUIOnly;
	SetInputMode(InputModeUIOnly);

	if (IsValid(ChatInputWidgetClass) == true)
	{
		ChatInputWidgetInstance = CreateWidget<UNBChatWidget>(this, ChatInputWidgetClass);
		if (IsValid(ChatInputWidgetInstance) == true)
		{
			ChatInputWidgetInstance->AddToViewport();
		}
	}
}

void ANBPlayerController::SendChatMessage(const FString& Message)
{
	if (Message.IsEmpty())	return;

	if (Message.StartsWith(TEXT("/")))
	{
		HandleCommand(Message.RightChop(1));
		return;
	}

	ServerSendMessage(Message);
}

void ANBPlayerController::HandleCommand(const FString& CommandInput)
{
	FString CommandName;
	FString CommandArgs;
	if (!CommandInput.Split(TEXT(" "), &CommandName, &CommandArgs))
	{
		CommandName = CommandInput;
	}

	if (CommandName.Equals(TEXT("GameOn"), ESearchCase::IgnoreCase))
	{
		ServerRequestGameOn();
		return;
	}

	//	이하 입력은 게임 도전(세 자리 숫자)으로 해석한다.
	//	GameState로부터 게임 진행 상태를, PlayerState로부터 내 차례/남은 기회를 읽어 클라이언트에서 검증한다.
	const ANBGameState* NBGameState = GetWorld()->GetGameState<ANBGameState>();
	if (!IsValid(NBGameState) || !NBGameState->IsGameOn())
	{
		ShowLocalSystemMessage(TEXT("게임이 진행 중이 아닙니다."));
		return;
	}

	const ANBPlayerState* NBPlayerState = GetPlayerState<ANBPlayerState>();
	if (!IsValid(NBPlayerState) || !NBPlayerState->IsMyTurn())
	{
		ShowLocalSystemMessage(TEXT("당신의 차례가 아닙니다."));
		return;
	}

	if (!IsValidGuessNumber(CommandName))
	{
		ShowLocalSystemMessage(TEXT("잘못된 입력입니다. 서로 다른 세 자리 숫자를 입력하세요."));
		return;
	}

	if (!NBPlayerState->HasRemainChance())
	{
		ShowLocalSystemMessage(TEXT("남은 도전 기회가 없습니다."));
		return;
	}

	ServerSubmitNumber(CommandName);
}

void ANBPlayerController::ShowLocalSystemMessage(const FString& Message, FLinearColor Color) const
{
	if (IsValid(ChatInputWidgetInstance))
	{
		ChatInputWidgetInstance->AddMessage(TEXT("System"), Message, Color);
	}
}

bool ANBPlayerController::IsValidGuessNumber(const FString& Number) const
{
	//	정확히 세 자리여야 한다.
	if (Number.Len() != 3)	return false;

	TArray<bool> UsedDigits;
	UsedDigits.Init(false, 10);

	for (const TCHAR Char : Number)
	{
		//	각 문자가 숫자여야 한다.
		if (!FChar::IsDigit(Char))	return false;

		//	각 자리의 숫자가 서로 달라야 한다.
		const int32 Digit = Char - TEXT('0');
		if (UsedDigits[Digit])	return false;
		UsedDigits[Digit] = true;
	}

	return true;
}

void ANBPlayerController::ServerRequestGameOn_Implementation()
{
	if (ANBGameModeBase* GM = GetWorld()->GetAuthGameMode<ANBGameModeBase>())
	{
		GM->RequestGameOn(this);
	}
}

void ANBPlayerController::ServerSubmitNumber_Implementation(const FString& Number)
{
	if (ANBGameModeBase* GM = GetWorld()->GetAuthGameMode<ANBGameModeBase>())
	{
		GM->SubmitNumber(this, Number);
	}
}

void ANBPlayerController::ServerSendMessage_Implementation(const FString& Message)
{
	FString SenderName = TEXT("Player");
	if (IsValid(PlayerState))
	{
		SenderName = PlayerState->GetPlayerName();
	}

	if (ANBGameModeBase* GM = GetWorld()->GetAuthGameMode<ANBGameModeBase>())
	{
		GM->BroadcastChatMessage(SenderName, Message);
	}
}

void ANBPlayerController::ClientReceiveMessage_Implementation(const FString& SenderName, const FString& Message)
{
	if (IsValid(ChatInputWidgetInstance))
	{
		ChatInputWidgetInstance->AddMessage(SenderName, Message);
	}
}

void ANBPlayerController::ClientReceiveSystemMessage_Implementation(const FString& Message, FLinearColor Color)
{
	if (IsValid(ChatInputWidgetInstance))
	{
		ChatInputWidgetInstance->AddMessage(TEXT("System"), Message, Color);
	}
}
