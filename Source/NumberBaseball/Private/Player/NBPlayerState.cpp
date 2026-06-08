// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NBPlayerState.h"

#include "Net/UnrealNetwork.h"

void ANBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANBPlayerState, RemainChance);
	DOREPLIFETIME(ANBPlayerState, MaxChance);
	DOREPLIFETIME(ANBPlayerState, bIsMyTurn);
}

void ANBPlayerState::InitGame()
{
	RemainChance = MaxChance;
	bIsMyTurn = false;
}

void ANBPlayerState::SetMyTurn(bool bInMyTurn)
{
	if (!HasAuthority())	return;

	bIsMyTurn = bInMyTurn;
}

bool ANBPlayerState::SpendChance()
{
	if (!HasAuthority())	return false;
	if (RemainChance < 1)	return false;
	
	RemainChance--;
	return true;
}
