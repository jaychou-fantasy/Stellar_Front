// Fill out your copyright notice in the Description page of Project Settings.


#include "SGunBase.h"
#include "SCharacter.h"
#include "SPhysicalSurfaceTypes.h"
#include "SGunCasing.h"
#include "SProjectileBase.h"
#include "Animation/AnimInstance.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MetasoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Sets default values
ASGunBase::ASGunBase()
{
	PrimaryActorTick.bCanEverTick = false;
	GunMuzzleName = "Muzzle";
	
	// Create a gun mesh component
	GunMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	GunMeshComponent->CastShadow = false;
	//GunMeshComponent->SetupAttachment(ArmComponent, TEXT("GripPoint"));
	
	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(GunMeshComponent, TEXT("Barrel"));
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Stock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stock"));
	Stock->SetupAttachment(GunMeshComponent, TEXT("Stock"));
	Stock->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Magazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine"));
	Magazine->SetupAttachment(GunMeshComponent, TEXT("Mag 1"));
	Magazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Scope = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scope"));
	Scope->SetupAttachment(GunMeshComponent, TEXT("Lens"));
	Scope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	Sight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight"));
	Sight->SetupAttachment(GunMeshComponent, TEXT("Sight"));
	Sight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASGunBase::PlayKakeSound()
{
	UGameplayStatics::PlaySoundAtLocation(this,KaKeSound,GetOwner()->GetActorLocation());
}

void ASGunBase::WeaponFire(APawn* InstigatorPawn, bool bIsAiming)
{
	ASCharacter* InstigatorCharacter = Cast<ASCharacter>(InstigatorPawn);
	if (InstigatorCharacter)
	{
		USkeletalMeshComponent* Mesh1P = InstigatorCharacter->GetArm();

		// Get the animation object for the arms mesh
		//aim fire
		//idle fire
		//sprint | walk fire
		//Play Arm|Weapon Animation && Play Muzzle FX
		UAnimInstance* ArmAnim = Mesh1P->GetAnimInstance();
		UAnimInstance* GunAnim = GunMeshComponent->GetAnimInstance();
		const FWeaponFireAnimation& FireAnimation = GetFireAnimation(InstigatorCharacter->GetCharacterState(),bIsAiming);

		if (ArmAnim && GunAnim)
		{
			UAnimMontage* ArmMontage = FireAnimation.ArmMontage;
			UAnimMontage* WeaponMontage = FireAnimation.WeaponMontage;

			ArmAnim->Montage_Play(ArmMontage);
			GunAnim->Montage_Play(WeaponMontage);
		}
		//spawn emitter
		if (MuzzleFlash)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash,Barrel,TEXT("Muzzle"),FVector::ZeroVector,FRotator::ZeroRotator,EAttachLocation::KeepRelativeOffset,true);
		}
		//play meta sound
		if (FireSound)
		{
			UGameplayStatics::PlaySound2D(this,FireSound);
			//UGameplayStatics::PlaySoundAtLocation(this, FireSound, InstigatorCharacter->GetActorLocation());//they are in the same location,no need to get gun location
		}
		//spawn casing
		SpawnCasing();

		// try and fire a projectile
		if (ensureAlways(ProjectileClass))
		{
			FVector MuzzleLocation = GunMeshComponent->GetSocketLocation(GunMuzzleName);
			FRotator MuzzleRotation = InstigatorCharacter->GetControlRotation();

			//Ray Check
			FHitResult Hit;
			FVector TraceStart = InstigatorCharacter->GetPawnViewLocation();//override as "Camera Comp's ViewLocations"
			FVector TraceEnd = TraceStart + MuzzleRotation.Vector() * 5000;

			FCollisionObjectQueryParams ObjectQueryParams;
			ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

			FCollisionShape CollisionShape;
			CollisionShape.SetSphere(ProjectileClass->GetDefaultObject<ASProjectileBase>()->GetSphereRadius());//GetDefaultObject----> Cast "UClass*" to "ASprojectileBase*"

			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActor(InstigatorCharacter);
			CollisionQueryParams.AddIgnoredActor(this);
			CollisionQueryParams.bReturnPhysicalMaterial = true;

			FRotator ProjRotation;
			// Fall back since we failed to find any blocking hit
			// Settle for a slightly less accurate direction as a fallback(tui er qiu qi ci)
			ProjRotation = FRotationMatrix::MakeFromX(TraceEnd - MuzzleLocation).Rotator();
			/*Takes your provided XAxis as the local forward direction of the object
			  Automatically calculates a suitable Y and Z axis (ensures an orthonormal basis)
			  Finally generates an FRotationMatrix (rotation matrix)*/
			if (GetWorld()->SweepSingleByObjectType(Hit,TraceStart,TraceEnd,FQuat::Identity,ObjectQueryParams,CollisionShape,CollisionQueryParams))
			{
				TraceEnd = Hit.ImpactPoint;
				ProjRotation = FRotationMatrix::MakeFromX(TraceEnd - MuzzleLocation).Rotator();
				UE_LOG(LogTemp, Warning, TEXT("[FireTrace] HitActor=%s HitComponent=%s ImpactPoint=%s TraceEnd=%s ProjRotation=%s"),
					*GetNameSafe(Hit.GetActor()),
					*GetNameSafe(Hit.GetComponent()),
					*Hit.ImpactPoint.ToCompactString(),
					*TraceEnd.ToCompactString(),
					*ProjRotation.ToCompactString());

				const FOnHitFlashSound* HitFeedback = nullptr;
				switch (UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()))
				{
				case SurfaceType_Default:
				case SURFACE_CONCRETE:
					HitFeedback = &DefaultConcreteOnHit;
					break;
				case SURFACE_DIRT:
					HitFeedback = &DirtOnHit;
					break;
				case SURFACE_WOOD:
					HitFeedback = &WoodOnHit;
					break;
				case SURFACE_GLASS:
					HitFeedback = &GlassOnHit;
					break;
				case SURFACE_ENEMY:
					HitFeedback = &EnemyOnHit;
					break;
				default:
					break;
				}

				if (HitFeedback)
				{
					//Spawn decal
					SpawnImpactDecal(TraceEnd, ProjRotation, HitFeedback->DecalMaterial, HitFeedback->DecalScale);

					if (HitFeedback->OnHitFlash)
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitFeedback->OnHitFlash, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
					}

					if (HitFeedback->OnHitSound)
					{
						UGameplayStatics::PlaySoundAtLocation(this, HitFeedback->OnHitSound, Hit.ImpactPoint,1.6f);
					}
				}
			}


			//Begin Spawn  Projectile at the Muzzle
			FTransform SpawnMT = FTransform(ProjRotation , MuzzleLocation);//combine Location w/ Rotation
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;// can be nullptr if colliding with wall or sth.
			ActorSpawnParams.Instigator = InstigatorCharacter;

			GetWorld()->SpawnActor<ASProjectileBase>(ProjectileClass, SpawnMT, ActorSpawnParams);
		}

		//use recoil
		InstigatorCharacter->AddControllerPitchInput(-VerticalRecoil);
		InstigatorCharacter->AddControllerYawInput(FMath::RandRange(-HorizontalRecoil,HorizontalRecoil));

		//Consume Ammo
		ConsumeMagAmmo();
		//update UI
		UMainWidget* MainUI = Cast<UMainWidget>(InstigatorCharacter->GetMainUI());
		MainUI->UpdateAmmo(GetRestMagAmmo(),TotalAmmo);
	}
	
}

void ASGunBase::SpawnImpactDecal(const FVector& SpawnLocation, const FRotator& SpawnRotation, UMaterialInterface* DecalMaterial, const FVector& DecalScale)
{
	if (!DecalActor || !DecalMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ImpactDecal] Skipped: DecalActor=%s Material=%s"), *GetNameSafe(DecalActor), *GetNameSafe(DecalMaterial));
		return;
	}

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation, DecalScale);
	AActor* SpawnedDecal = GetWorld()->SpawnActor<AActor>(DecalActor, SpawnTransform);
	if (!IsValid(SpawnedDecal))
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[ImpactDecal] Actor=%s RequestedLocation=%s ActorLocation=%s RequestedRotation=%s ActorRotation=%s Material=%s"),
		*GetNameSafe(SpawnedDecal),
		*SpawnLocation.ToCompactString(),
		*SpawnedDecal->GetActorLocation().ToCompactString(),
		*SpawnRotation.ToCompactString(),
		*SpawnedDecal->GetActorRotation().ToCompactString(),
		*GetNameSafe(DecalMaterial));

	//this component is in "BP_Decal"
	if (UDecalComponent* DecalComponent = SpawnedDecal->FindComponentByClass<UDecalComponent>())
	{
		DecalComponent->SetDecalMaterial(DecalMaterial);
		UE_LOG(LogTemp, Warning, TEXT("[ImpactDecal] Component=%s Location=%s Rotation=%s"),
			*GetNameSafe(DecalComponent),
			*DecalComponent->GetComponentLocation().ToCompactString(),
			*DecalComponent->GetComponentRotation().ToCompactString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Impact decal actor %s has no DecalComponent"), *GetNameSafe(SpawnedDecal));
	}
}

void ASGunBase::SpawnCasing()
{
	FActorSpawnParameters SpawnParams;
	FTransform Transform = GunMeshComponent->GetSocketTransform(TEXT("Casing"));
	
	GetWorld()->SpawnActor<ASGunCasing>(CasingClass,Transform,SpawnParams);
}

void ASGunBase::WeaponReload(APawn* InstigatorPawn)
{
	ASCharacter* InstigatorCharacter = Cast<ASCharacter>(InstigatorPawn);
	if (InstigatorCharacter)
	{
		USkeletalMeshComponent* Mesh1P = InstigatorCharacter->GetArm();
		
		UAnimInstance* ArmAnim = Mesh1P->GetAnimInstance();
		UAnimInstance* GunAnim = GunMeshComponent->GetAnimInstance();
		ArmAnim->Montage_Play(ArmReloadAnim);
		GunAnim->Montage_Play(WeaponReloadAnim);
		
		UGameplayStatics::PlaySoundAtLocation(this,ReloadSound,InstigatorCharacter->GetActorLocation());
		
		//Set Ammo in AnimNotify---in UE Editor
		//ReloadAmmo();
	}
}

void ASGunBase::ReloadAmmo()
{
	int32 ActualBulletToReload = MagSize - MagRestAmmo;
	if (ActualBulletToReload < TotalAmmo)
	{
		MagRestAmmo = MagSize;//or just ---- MagRestAmmo += ActualBulletToReload
		ConsumeTotalAmmo(ActualBulletToReload);
	}
	else//the case like,Total Ammo=3,but MagRestAmmo = 10(while MagSize = 30)
	{
		MagRestAmmo += TotalAmmo;
		TotalAmmo = 0;
	}
}


bool ASGunBase::MagHasAmmo() const
{
	return MagRestAmmo > 0;
}

bool ASGunBase::TotalHasAmmo() const
{
	return TotalAmmo > 0;
}

void ASGunBase::UpdateMagSize(int32 NewAmmoNumber)
{
	MagSize = NewAmmoNumber;
}

void ASGunBase::ConsumeMagAmmo()
{
	MagRestAmmo--;
}

void ASGunBase::ConsumeTotalAmmo(int32 Delta)
{
	TotalAmmo -= Delta;
}



FVector ASGunBase::GetAimSocketLocation() const
{
	return GunMeshComponent->GetSocketLocation(TEXT("AimSocket"));
}

const FWeaponFireAnimation& ASGunBase::GetFireAnimation(ESCharacterState State, bool bIsAiming) const
{
	if (bIsAiming)
	{
		return AimFire;
	}
	switch (State)
	{
		case ESCharacterState::Walk:   //use the same fire anim montage as SPRINT
		case ESCharacterState::Sprint:
			return SprintFire;
		case ESCharacterState::Idle:
		default:
			return IdleFire;
	}
}

void ASGunBase::BeginPlay()
{
	Super::BeginPlay();
	//initialize the MagRestAmmo<--->MagSize
	MagRestAmmo = MagSize;
	
}
