// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/NBGameState.h"

#include "Net/UnrealNetwork.h"

void ANBGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBGameState, bIsGameOn);
	DOREPLIFETIME(ANBGameState, TurnEndServerTime);
	DOREPLIFETIME(ANBGameState, TurnDuration);
}

void ANBGameState::StartTurnTimer(float Duration)
{
	if (!HasAuthority())	return;

	TurnDuration = Duration;
	TurnEndServerTime = GetServerWorldTimeSeconds() + Duration;
}

void ANBGameState::ClearTurnTimer()
{
	if (!HasAuthority())	return;

	TurnDuration = 0.f;
	TurnEndServerTime = 0.f;
}

float ANBGameState::GetRemainingTurnRatio() const
{
	if (TurnDuration <= 0.f)	return 0.f;

	const float Remaining = TurnEndServerTime - GetServerWorldTimeSeconds();
	return FMath::Clamp(Remaining / TurnDuration, 0.f, 1.f);
}

float ANBGameState::GetRemainingTurnSeconds() const
{
	if (TurnDuration <= 0.f)	return 0.f;

	const float Remaining = TurnEndServerTime - GetServerWorldTimeSeconds();
	return FMath::Max(Remaining, 0.f);
}

void ANBGameState::SetGameOn(bool bInGameOn)
{
	//	복제 변수는 서버 권위 측에서만 변경해야 한다.
	if (!HasAuthority())	return;
	if (bIsGameOn == bInGameOn)	return;
	bIsGameOn = bInGameOn;
	
	if (!bIsGameOn)	return;

	//	0~9 풀에서 서로 다른 세 자리를 무작위로 뽑아 정답을 만든다.
	//	(클라이언트의 추측 검증 규칙과 동일하게 각 자리 숫자가 모두 다르며, 리딩제로를 허용한다.)
	TArray<int32> DigitPool;
	for (int32 Digit = 0; Digit <= 9; ++Digit)
	{
		DigitPool.Add(Digit);
	}

	CorrectAnswer.Empty(3);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const int32 PickIndex = FMath::RandRange(0, DigitPool.Num() - 1);
		CorrectAnswer.AppendInt(DigitPool[PickIndex]);
		DigitPool.RemoveAt(PickIndex);
	}
}
