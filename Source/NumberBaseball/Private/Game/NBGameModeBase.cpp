// Fill out your copyright notice in the Description page of Project Settings.


#include "NumberBaseball/Public/Game/NBGameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "NumberBaseball/Public/Game/NBGameState.h"
#include "NumberBaseball/Public/Player/NBPlayerController.h"
#include "NumberBaseball/Public/Player/NBPlayerState.h"

ANBGameModeBase::ANBGameModeBase()
{
	GameStateClass = ANBGameState::StaticClass();
}

void ANBGameModeBase::BroadcastChatMessage(const FString& SenderName, const FString& Message)
{
	//	연결되어있는 플레이어 컨트롤러 순회
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANBPlayerController* PC = Cast<ANBPlayerController>(It->Get()))
		{	//	클라이언트에게 메세지 전송
			PC->ClientReceiveMessage(SenderName, Message);
		}
	}
}

void ANBGameModeBase::SendSystemMessageToPlayer(APlayerController* Target, const FString& Message, FLinearColor Color)
{
	if (ANBPlayerController* PC = Cast<ANBPlayerController>(Target))
	{
		PC->ClientReceiveSystemMessage(Message, Color);
	}
}

void ANBGameModeBase::RequestGameOn(APlayerController* Requester)
{
	ANBGameState* NBGameState = GetGameState<ANBGameState>();
	if (!IsValid(NBGameState))	return;

	if (NBGameState->IsGameOn())
	{
		SendSystemMessageToPlayer(Requester, TEXT("이미 게임이 진행 중입니다."), FLinearColor::Red);
		return;
	}

	//	GameState에 설정하면 복제를 통해 모든 클라이언트로 자동 동기화된다.
	NBGameState->SetGameOn(true);

	//	게임 시작 시점에 연결된 플레이어로 턴 순서를 고정한다. (이후 중간 참가 불가)
	TurnOrder.Empty();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ANBPlayerState* PS = PC->GetPlayerState<ANBPlayerState>())
			{
				PS->InitGame();
				TurnOrder.Add(PC);
			}
		}
	}

	BroadcastChatMessage(TEXT("System"), TEXT("게임이 시작되었습니다!"));

	//	첫 번째 차례는 참가자 중 무작위로 선택한다.
	if (TurnOrder.Num() > 0)
	{
		const int32 FirstIndex = FMath::RandRange(0, TurnOrder.Num() - 1);
		GiveTurnTo(TurnOrder[FirstIndex].Get());
	}
}

void ANBGameModeBase::SubmitNumber(APlayerController* Requester, const FString& Number)
{
	//	서버 권위 측 방어: 게임이 진행 중이 아니라면 무시한다.
	ANBGameState* NBGameState = GetGameState<ANBGameState>();
	if (!IsValid(NBGameState) || !NBGameState->IsGameOn())	return;

	ANBPlayerState* NBPlayerState = Requester ? Requester->GetPlayerState<ANBPlayerState>() : nullptr;
	if (!IsValid(NBPlayerState))	return;

	//	서버 권위 측 방어: 자신의 차례인 플레이어만 도전할 수 있다.
	if (!NBPlayerState->IsMyTurn())
	{
		SendSystemMessageToPlayer(Requester, TEXT("당신의 차례가 아닙니다."), FLinearColor::Red);
		return;
	}

	//	서버 권위 측 방어: 남은 기회가 있어야 한다.
	if (!NBPlayerState->SpendChance())
	{
		SendSystemMessageToPlayer(Requester, TEXT("남은 기회가 없습니다."), FLinearColor::Red);
		return;
	}

	const FString SenderName = NBPlayerState->GetPlayerName();

	//	추측 내용을 모두에게 알린다.
	BroadcastChatMessage(SenderName, FString::Printf(TEXT("추측: %s"), *Number));

	//	정답과 비교해 판정한다.
	int32 Strikes = 0;
	int32 Balls = 0;
	JudgeGuess(NBGameState->GetCorrectAnswer(), Number, Strikes, Balls);

	if (Strikes == 3)
	{
		//	정답: 게임을 종료한다.
		BroadcastChatMessage(TEXT("System"), FString::Printf(TEXT("%s 님 정답! 정답은 %s 였습니다. 게임 종료"), *SenderName, *Number));
		EndGame();
		return;
	}

	//	시도 결과와 함께 해당 플레이어의 남은 도전 횟수도 알린다.
	const int32 RemainChance = NBPlayerState->GetRemainChance();
	if ( Strikes + Balls == 0)
	{
		BroadcastChatMessage(TEXT("System"), FString::Printf(TEXT("%s : Out (남은 기회 %d)"), *SenderName, RemainChance));
	}
	else
	{
		BroadcastChatMessage(TEXT("System"), FString::Printf(TEXT("%s : %dS %dB (남은 기회 %d)"), *SenderName, Strikes, Balls, RemainChance));
	}

	//	이번 추측으로 기회를 모두 소진했다면 본인에게 알린다.
	if (!NBPlayerState->HasRemainChance())
	{
		SendSystemMessageToPlayer(Requester, TEXT("남은 기회를 모두 소진했습니다."), FLinearColor::Red);
	}

	//	다음 차례로 넘긴다. 남은 기회가 있는 플레이어가 아무도 없으면 무승부로 종료한다.
	AdvanceTurnFrom(FindTurnIndex(Requester));
}

void ANBGameModeBase::Logout(AController* Exiting)
{
	APlayerController* ExitingPC = Cast<APlayerController>(Exiting);
	const int32 Index = FindTurnIndex(ExitingPC);

	//	진행 중인 게임의 참가자가 나간 경우에만 차례 처리를 한다.
	if (Index != INDEX_NONE)
	{
		const bool bWasCurrentTurn = (CurrentTurnPC.Get() == ExitingPC);

		//	나간 플레이어는 턴 순서에서 제거한다. (차례 몰수)
		TurnOrder.RemoveAt(Index);

		ANBGameState* NBGameState = GetGameState<ANBGameState>();
		if (IsValid(NBGameState) && NBGameState->IsGameOn() && bWasCurrentTurn)
		{
			CurrentTurnPC = nullptr;

			if (TurnOrder.Num() == 0)
			{
				//	참가자가 모두 나갔다: 게임을 종료한다.
				BroadcastChatMessage(TEXT("System"), TEXT("모든 플레이어가 나갔습니다. 게임 종료"));
				EndGame();
			}
			else
			{
				//	나간 자리에는 다음 플레이어가 당겨져 들어와 있다. 그 자리부터 다음 차례를 찾는다.
				BroadcastChatMessage(TEXT("System"), TEXT("현재 차례 플레이어가 나가 차례를 넘깁니다."));
				const int32 SearchStart = Index % TurnOrder.Num();
				if (!TryAdvanceTurn(SearchStart))
				{
					EndGameAsDraw();
				}
			}
		}
	}

	Super::Logout(Exiting);
}

int32 ANBGameModeBase::FindTurnIndex(const APlayerController* PC) const
{
	if (!IsValid(PC))	return INDEX_NONE;

	for (int32 i = 0; i < TurnOrder.Num(); ++i)
	{
		if (TurnOrder[i].Get() == PC)	return i;
	}

	return INDEX_NONE;
}

void ANBGameModeBase::ClearAllTurnFlags()
{
	for (const TWeakObjectPtr<APlayerController>& WeakPC : TurnOrder)
	{
		if (APlayerController* PC = WeakPC.Get())
		{
			if (ANBPlayerState* PS = PC->GetPlayerState<ANBPlayerState>())
			{
				PS->SetMyTurn(false);
			}
		}
	}
}

void ANBGameModeBase::GiveTurnTo(APlayerController* PC)
{
	if (!IsValid(PC))	return;

	ANBPlayerState* PS = PC->GetPlayerState<ANBPlayerState>();
	if (!IsValid(PS))	return;

	//	이전 차례를 끄고 새 차례를 켠다.
	ClearAllTurnFlags();
	PS->SetMyTurn(true);
	CurrentTurnPC = PC;

	//	이번 턴의 제한시간 타이머를 (재)시작한다.
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ANBGameModeBase::OnTurnTimeout, TurnTimeLimit, false);

	//	UI(프로그레스바)용으로 턴 종료 시각을 GameState에 복제한다.
	if (ANBGameState* NBGameState = GetGameState<ANBGameState>())
	{
		NBGameState->StartTurnTimer(TurnTimeLimit);
	}

	BroadcastChatMessage(TEXT("System"), FString::Printf(TEXT("%s 님의 차례입니다. (제한시간 %d초)"), *PS->GetPlayerName(), FMath::RoundToInt(TurnTimeLimit)));
}

bool ANBGameModeBase::TryAdvanceTurn(int32 SearchStartIndex)
{
	const int32 Count = TurnOrder.Num();
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 TurnIndex = (SearchStartIndex + i) % Count;
		APlayerController* PC = TurnOrder[TurnIndex].Get();
		ANBPlayerState* PS = IsValid(PC) ? PC->GetPlayerState<ANBPlayerState>() : nullptr;

		//	남은 기회가 있는 다음 플레이어에게 차례를 넘긴다.
		if (IsValid(PS) && PS->HasRemainChance())
		{
			GiveTurnTo(PC);
			return true;
		}
	}

	return false;
}

void ANBGameModeBase::AdvanceTurnFrom(int32 CurrentTurnIndex)
{
	//	현재 플레이어 바로 다음 순서부터 남은 기회가 있는 플레이어를 찾는다.
	const int32 SearchStart = (TurnOrder.Num() > 0)
		? ((CurrentTurnIndex == INDEX_NONE ? 0 : CurrentTurnIndex + 1) % TurnOrder.Num())
		: 0;

	if (!TryAdvanceTurn(SearchStart))
	{
		EndGameAsDraw();
	}
}

void ANBGameModeBase::OnTurnTimeout()
{
	ANBGameState* NBGameState = GetGameState<ANBGameState>();
	if (!IsValid(NBGameState) || !NBGameState->IsGameOn())	return;

	APlayerController* TimedOutPC = CurrentTurnPC.Get();
	const int32 CurrentIndex = FindTurnIndex(TimedOutPC);

	//	제한시간 내에 도전하지 않은 플레이어의 기회를 차감한다.
	if (ANBPlayerState* PS = IsValid(TimedOutPC) ? TimedOutPC->GetPlayerState<ANBPlayerState>() : nullptr)
	{
		PS->SpendChance();

		BroadcastChatMessage(TEXT("System"),
			FString::Printf(TEXT("%s 님이 제한시간을 초과했습니다. 기회가 차감되고 차례가 넘어갑니다. (남은 기회 %d)"), *PS->GetPlayerName(), PS->GetRemainChance()));

		if (!PS->HasRemainChance())
		{
			SendSystemMessageToPlayer(TimedOutPC, TEXT("남은 기회를 모두 소진했습니다."), FLinearColor::Red);
		}
	}

	//	다음 차례로 넘긴다. 남은 기회가 있는 플레이어가 아무도 없으면 무승부로 종료한다.
	AdvanceTurnFrom(CurrentIndex);
}

void ANBGameModeBase::EndGame()
{
	//	진행 중인 턴 타이머를 정리한다.
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	ClearAllTurnFlags();
	CurrentTurnPC = nullptr;
	TurnOrder.Empty();

	if (ANBGameState* NBGameState = GetGameState<ANBGameState>())
	{
		NBGameState->ClearTurnTimer();
		NBGameState->SetGameOn(false);
	}
}

void ANBGameModeBase::EndGameAsDraw()
{
	const ANBGameState* NBGameState = GetGameState<ANBGameState>();
	const FString Answer = IsValid(NBGameState) ? NBGameState->GetCorrectAnswer() : FString();

	BroadcastChatMessage(TEXT("System"),
		FString::Printf(TEXT("무승부! 아무도 맞히지 못했습니다. 정답은 %s 였습니다. 게임 종료"), *Answer));

	EndGame();
}

void ANBGameModeBase::JudgeGuess(const FString& Answer, const FString& Guess, int32& OutStrikes, int32& OutBalls) const
{
	OutStrikes = 0;
	OutBalls = 0;

	//	각 자리 숫자가 서로 다르다는 전제(클라이언트 검증)하에 자리별로 비교한다.
	for (int32 GuessIndex = 0; GuessIndex < Guess.Len(); ++GuessIndex)
	{
		for (int32 AnswerIndex = 0; AnswerIndex < Answer.Len(); ++AnswerIndex)
		{
			if (Guess[GuessIndex] != Answer[AnswerIndex])	continue;

			//	같은 숫자가 같은 자리면 스트라이크, 다른 자리면 볼.
			if (GuessIndex == AnswerIndex)	++OutStrikes;
			else							++OutBalls;

			break;
		}
	}
}

