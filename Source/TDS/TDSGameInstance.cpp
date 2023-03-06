// Fill out your copyright notice in the Description page of Project Settings.


#include "TDSGameInstance.h"


bool UTDSGameInstance::GetWeaponInfoByName(FName NameWeapon, FWeaponInfo& OutInfo)
{
	bool bIsFinde = false;
	FWeaponInfo* WeaponInfoRow;

	if (WeaponInfoTable)
	{
		WeaponInfoRow = WeaponInfoTable->FindRow<FWeaponInfo>(NameWeapon, "", false);
		if(WeaponInfoRow)
		{
			bIsFinde = true;
			OutInfo = *WeaponInfoRow;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TDSGameInstance::GetWeaponInfoByName - Weapon not found the table -NULL"));
	}
    
	return bIsFinde;
}

