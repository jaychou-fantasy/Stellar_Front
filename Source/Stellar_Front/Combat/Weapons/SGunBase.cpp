// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Weapons/SGunBase.h"
#include "Character/SCharacter.h"
#include "Combat/Impact/SPhysicalSurfaceTypes.h"
#include "Combat/Weapons/SGunCasing.h"
#include "Combat/Projectiles/SProjectileBase.h"
#include "Animation/AnimInstance.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MetasoundSource.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Sets default values
ASGunBase::ASGunBase()
{
	bReplicates = true;
	SetReplicateMovement(true);//replicate the Attachment of the gunActor
	bNetUseOwnerRelevancy = true;//control which client need to receive this gun (set relevancy between its owner & this actor)
	//who can see the owner ---> who can see this actor
	//while "bOnlyRelevantToOwner" ---> only rep this gun to its owners (used in sth. of protected like AMMO AMOUNT) ---- pairing with DOREPLIFETIME_CONDITION(COND_OwnerOnly)
	
	PrimaryActorTick.bCanEverTick = false;
	GunMuzzleName = "Muzzle";
	
	// Create a gun mesh component
	GunMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	RootComponent = GunMeshComponent;
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

void ASGunBase::PlayOnHitFeedback(const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}
	const EPhysicalSurface SurfaceType =  UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
	MulticastPlayOnHitFX(static_cast<uint8>(SurfaceType),Hit.ImpactPoint,Hit.ImpactNormal);
}

void ASGunBase::MulticastPlayOnHitFX_Implementation(uint8 SurfaceType, FVector_NetQuantize ImpactPoint,FVector_NetQuantizeNormal ImpactNormal)
{
	const FOnHitFlashSound* HitFeedback = nullptr;
	switch (SurfaceType)
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
		SpawnImpactDecal(ImpactPoint, (-ImpactNormal).Rotation(), HitFeedback->DecalMaterial, HitFeedback->DecalScale);

		if (HitFeedback->OnHitFlash)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitFeedback->OnHitFlash, ImpactPoint, ImpactNormal.Rotation());
		}

		if (HitFeedback->OnHitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, HitFeedback->OnHitSound, ImpactPoint,1.6f);
		}
	}
}


void ASGunBase::WeaponFire(APawn* InstigatorPawn, bool bIsAiming)
{
	if (!HasAuthority())
	{
		return;
	}
	ASCharacter* InstigatorCharacter = Cast<ASCharacter>(InstigatorPawn);
	if (!IsValid(InstigatorCharacter) || InstigatorCharacter != GetInstigator())
	{
		return;
	}
	if (!MagHasAmmo() || !ensure(ProjectileClass))
	{
		return;
	}
	//check in Server
	//multicast to play FX
	
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
			}


			//Begin Spawn  Projectile at the Muzzle
			FTransform SpawnMT = FTransform(ProjRotation , MuzzleLocation);//combine Location w/ Rotation
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;// can be nullptr if colliding with wall or sth.
			ActorSpawnParams.Instigator = InstigatorCharacter;

			ASProjectileBase* SpawnedProjectile = GetWorld()->SpawnActor<ASProjectileBase>(ProjectileClass, SpawnMT, ActorSpawnParams);
			if (SpawnedProjectile)
			{
				SpawnedProjectile->SetSourceGun(this);
			}
		}
	ConsumeMagAmmo();//only change ammo amount in server 
	MulticastPlayFireFX(InstigatorCharacter,bIsAiming);
	
}

void ASGunBase::MulticastPlayFireFX_Implementation(ASCharacter* InstigatorChar,bool bIsAiming)
{
	//no need to check Instigator again ,because only when it's valid can we enter this funct 
	if (GetNetMode() == NM_DedicatedServer)//pure server(not listen server)   so no need to play fx again.only play in client
	{
		return;
	}
	
	USkeletalMeshComponent* Mesh1P = InstigatorChar->GetArm();

		// Get the animation object for the arms mesh
		//aim fire
		//idle fire
		//sprint | walk fire
		//Play Arm|Weapon Animation && Play Muzzle FX
		UAnimInstance* ArmAnim = Mesh1P->GetAnimInstance();
		UAnimInstance* GunAnim = GunMeshComponent->GetAnimInstance();
		const FWeaponFireAnimation& FireAnimation = GetFireAnimation(InstigatorChar->GetCharacterState(),bIsAiming);

		if (ArmAnim && GunAnim)
		{
			if (InstigatorChar->IsLocallyControlled())
			{
				UAnimMontage* ArmMontage = FireAnimation.ArmMontage;
				ArmAnim->Montage_Play(ArmMontage);//only play in Client that control this pawn
				//@fixme:: 3p-arm montage-->same as gun montage
			}
			UAnimMontage* WeaponMontage = FireAnimation.WeaponMontage;
			GunAnim->Montage_Play(WeaponMontage);//gun montage play in every client who can see this gun
		}
	
		//spawn emitter
		if (MuzzleFlash)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlash,Barrel,TEXT("Muzzle"),FVector::ZeroVector,FRotator::ZeroRotator,EAttachLocation::KeepRelativeOffset,true);
		}
	
		//play meta sound
		if (FireSound)
		{
			//in local env, just play 2d
			if (InstigatorChar->IsLocallyControlled())
			{
				UGameplayStatics::PlaySound2D(this,FireSound);
			}
			//in other client, play 3d sound 
			else
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, InstigatorChar->GetActorLocation());
				//they are in the same location,no need to get gun location
			}
		}
	
		//spawn casing
		SpawnCasing();


		//use recoil
		if (InstigatorChar->IsLocallyControlled())
		{
			InstigatorChar->AddControllerPitchInput(-VerticalRecoil);
			InstigatorChar->AddControllerYawInput(FMath::RandRange(-HorizontalRecoil,HorizontalRecoil));
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
	if (!CasingClass || !GunMeshComponent)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	const FTransform Transform = GunMeshComponent->GetSocketTransform(TEXT("Casing"));
	
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
	if (!HasAuthority())
	{
		return;
	}

	const int32 MissingAmmo = MagSize - MagRestAmmo;
	const int32 ActualReloadAmount = FMath::Min(MissingAmmo, TotalAmmo);

	if (ActualReloadAmount <= 0)
	{
		return;
	}

	MagRestAmmo += ActualReloadAmount;
	TotalAmmo -= ActualReloadAmount;

	//because server don't reply on "Rep_Ammo"-->To call "RefreshAmmoUI"
	//so server RefreshAmmoUI in ReloadAmmo Funct (HasAuthority)
	//client use OnRep_Ammo to RefreshUI
	RefreshAmmoUI();
}


bool ASGunBase::MagHasAmmo() const
{
	return MagRestAmmo > 0;
}

bool ASGunBase::TotalHasAmmo() const
{
	return TotalAmmo > 0;
}

void ASGunBase::RefreshAmmoUI() const
{
	ASCharacter* OwnerCharacter = Cast<ASCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	UMainWidget* MainUI = OwnerCharacter->GetMainUI();
	if (MainUI)
	{
		//set UI_Ammo text
		MainUI->UpdateAmmo(MagRestAmmo, TotalAmmo);
	}
}

void ASGunBase::OnRep_Ammo()
{
	RefreshAmmoUI();
}

void ASGunBase::UpdateMagSize(int32 NewAmmoNumber)
{
	if (!HasAuthority())
	{
		return;
	}

	MagSize = FMath::Max(1, NewAmmoNumber);
	MagRestAmmo = FMath::Min(MagRestAmmo, MagSize);

	RefreshAmmoUI();
}

void ASGunBase::ConsumeMagAmmo()
{
	if (!HasAuthority())
	{
		return;
	}

	MagRestAmmo = FMath::Max(0, MagRestAmmo - 1);

	RefreshAmmoUI();
}

void ASGunBase::ConsumeTotalAmmo(int32 Delta)
{
	if (!HasAuthority())
	{
		return;
	}

	TotalAmmo = FMath::Max(0, TotalAmmo - Delta);

	RefreshAmmoUI();
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
	if (HasAuthority())
	{
		MagRestAmmo = MagSize;
		RefreshAmmoUI();
	}
	
}

void ASGunBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ASGunBase,TotalAmmo,COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASGunBase,MagRestAmmo,COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASGunBase,MagSize,COND_OwnerOnly);
}
