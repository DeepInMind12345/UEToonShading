// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Chaos/ArrayCollectionArray.h"
#include "Chaos/ImplicitObject.h"
#include "Chaos/Particles.h"
#include "Chaos/Rotation.h"
#include "Chaos/ParticleHandleFwd.h"
#include "Chaos/GeometryParticlesfwd.h"
#include "Chaos/CollisionFilterData.h"
#include "Chaos/Box.h"
#include "UObject/PhysicsObjectVersion.h"
#include "UObject/ExternalPhysicsCustomObjectVersion.h"

#ifndef CHAOS_DETERMINISTIC
#define CHAOS_DETERMINISTIC 1
#endif

namespace Chaos
{
	/** Data that is associated with geometry. If a union is used an entry is created per internal geometry */
	template <typename T, int d>
	class CHAOS_API TPerShapeData
	{
	public:
		static constexpr bool AlwaysSerializable = true;
		static TUniquePtr<TPerShapeData<T, d>> CreatePerShapeData();

		~TPerShapeData();
		TPerShapeData(const TPerShapeData<T, d>& Other) = delete;
		FCollisionFilterData QueryData;
		FCollisionFilterData SimData;
		void* UserData;
		TSerializablePtr<TImplicitObject<T, d>> Geometry;
		

		static TPerShapeData<T, d>* SerializationFactory(FChaosArchive& Ar, TPerShapeData<T, d>*);
		void Serialize(FChaosArchive& Ar);

	private:
		//use factory
		TPerShapeData();
	};

	template <typename T, int d>
	FChaosArchive& operator<<(FChaosArchive& Ar, TPerShapeData<T, d>& Shape)
	{
		Shape.Serialize(Ar);
		return Ar;
	}

	template <typename T, int d>
	using TShapesArray = TArray<TUniquePtr<TPerShapeData<T, d>>, TInlineAllocator<1>>;

	template <typename T, int d>
	void UpdateShapesArrayFromGeometry(TShapesArray<T, d>& ShapesArray, TSerializablePtr<TImplicitObject<T, d>> Geometry);

	extern template void CHAOS_API UpdateShapesArrayFromGeometry(TShapesArray<float, 3>& ShapesArray, TSerializablePtr<TImplicitObject<float, 3>> Geometry);

#if CHAOS_DETERMINISTIC
	using FParticleID = int32;	//Used to break ties when determinism is needed. Should not be used for anything else
#endif
	//Used for down casting when iterating over multiple SOAs.
	enum class EParticleType : uint8
	{
		Static,
		Kinematic,
		Dynamic,
		Clustered,	//only applicable on physics thread side
		StaticMesh,
		SkeletalMesh,
		GeometryCollection
	};

	
	template<class T, int d, EGeometryParticlesSimType SimType>
	class TGeometryParticlesImp : public TParticles<T, d>
	{
	public:

		using TArrayCollection::Size;
		using TParticles<T,d>::X;

		CHAOS_API static TGeometryParticlesImp<T, d, SimType>* SerializationFactory(FChaosArchive& Ar, TGeometryParticlesImp < T, d, SimType>* Particles);
		
		CHAOS_API TGeometryParticlesImp()
		    : TParticles<T, d>()
		{
			MParticleType = EParticleType::Static;
			TArrayCollection::AddArray(&MR);
			TArrayCollection::AddArray(&MGeometry);
			TArrayCollection::AddArray(&MSharedGeometry);
			TArrayCollection::AddArray(&MDynamicGeometry);
			TArrayCollection::AddArray(&MParticleIDs);
			TArrayCollection::AddArray(&MShapesArray);
			TArrayCollection::AddArray(&MLocalBounds);
			TArrayCollection::AddArray(&MWorldSpaceInflatedBounds);
			TArrayCollection::AddArray(&MHasBounds);
			TArrayCollection::AddArray(&MSpatialIdx);

			if (IsRigidBodySim())
			{
				TArrayCollection::AddArray(&MGeometryParticleHandle);
				TArrayCollection::AddArray(&MGeometryParticle);
			}

		}
		TGeometryParticlesImp(const TGeometryParticlesImp<T, d, SimType>& Other) = delete;
		CHAOS_API TGeometryParticlesImp(TGeometryParticlesImp<T, d, SimType>&& Other)
		    : TParticles<T, d>(MoveTemp(Other))
			, MR(MoveTemp(Other.MR))
			, MGeometry(MoveTemp(Other.MGeometry))
			, MSharedGeometry(MoveTemp(Other.MSharedGeometry))
			, MDynamicGeometry(MoveTemp(Other.MDynamicGeometry))
			, MGeometryParticleHandle(MoveTemp(Other.MGeometryParticleHandle))
			, MGeometryParticle(MoveTemp(Other.MGeometryParticle))
			, MShapesArray(MoveTemp(Other.MShapesArray))
			, MLocalBounds(MoveTemp(Other.MLocalBounds))
			, MWorldSpaceInflatedBounds(MoveTemp(Other.MWorldSpaceInflatedBounds))
			, MHasBounds(MoveTemp(Other.MHasBounds))
			, MSpatialIdx(MoveTemp(Other.MSpatialIdx))
#if CHAOS_DETERMINISTIC
			, MParticleIDs(MoveTemp(Other.MParticleIDs))
#endif
		{
			MParticleType = EParticleType::Static;
			TArrayCollection::AddArray(&MR);
			TArrayCollection::AddArray(&MGeometry);
			TArrayCollection::AddArray(&MSharedGeometry);
			TArrayCollection::AddArray(&MDynamicGeometry);
			TArrayCollection::AddArray(&MShapesArray);
			TArrayCollection::AddArray(&MLocalBounds);
			TArrayCollection::AddArray(&MWorldSpaceInflatedBounds);
			TArrayCollection::AddArray(&MHasBounds);
			TArrayCollection::AddArray(&MSpatialIdx);
#if CHAOS_DETERMINISTIC
			TArrayCollection::AddArray(&MParticleIDs);
#endif

			if (IsRigidBodySim())
			{
				TArrayCollection::AddArray(&MGeometryParticleHandle);
				TArrayCollection::AddArray(&MGeometryParticle);
			}
		}

		static constexpr bool IsRigidBodySim() { return SimType == EGeometryParticlesSimType::RigidBodySim; }

		CHAOS_API TGeometryParticlesImp(TParticles<T, d>&& Other)
		    : TParticles<T, d>(MoveTemp(Other))
		{
			MParticleType = EParticleType::Static;
			TArrayCollection::AddArray(&MR);
			TArrayCollection::AddArray(&MGeometry);
			TArrayCollection::AddArray(&MSharedGeometry);
			TArrayCollection::AddArray(&MDynamicGeometry);
			TArrayCollection::AddArray(&MShapesArray);
			TArrayCollection::AddArray(&MLocalBounds);
			TArrayCollection::AddArray(&MWorldSpaceInflatedBounds);
			TArrayCollection::AddArray(&MHasBounds);
			TArrayCollection::AddArray(&MSpatialIdx);
#if CHAOS_DETERMINISTIC
			TArrayCollection::AddArray(&MParticleIDs);
#endif

			if (IsRigidBodySim())
			{
				TArrayCollection::AddArray(&MGeometryParticleHandle);
				TArrayCollection::AddArray(&MGeometryParticle);
			}
		}

		CHAOS_API virtual ~TGeometryParticlesImp()
		{}

		CHAOS_API const TRotation<T, d>& R(const int32 Index) const { return MR[Index]; }
		CHAOS_API TRotation<T, d>& R(const int32 Index) { return MR[Index]; }

		CHAOS_API TSerializablePtr<TImplicitObject<T, d>> Geometry(const int32 Index) const { return MGeometry[Index]; }

		CHAOS_API const TUniquePtr<TImplicitObject<T, d>>& DynamicGeometry(const int32 Index) const { return MDynamicGeometry[Index]; }

		const TSharedPtr<TImplicitObject<T, d>, ESPMode::ThreadSafe>& SharedGeometry(const int32 Index) const { return MSharedGeometry[Index]; }

		const TShapesArray<T, d>& ShapesArray(const int32 Index) const { return MShapesArray[Index]; }

#if CHAOS_DETERMINISTIC
		CHAOS_API FParticleID ParticleID(const int32 Idx) const { return MParticleIDs[Idx]; }
		CHAOS_API FParticleID& ParticleID(const int32 Idx) { return MParticleIDs[Idx]; }
#endif

		CHAOS_API void SetDynamicGeometry(const int32 Index, TUniquePtr<TImplicitObject<T, d>>&& InUnique)
		{
			check(!SharedGeometry(Index));	// If shared geometry exists we should not be setting dynamic geometry on top
			MGeometry[Index] = MakeSerializable(InUnique);
			MDynamicGeometry[Index] = MoveTemp(InUnique);
			UpdateShapesArray(Index);
		}

		CHAOS_API void SetSharedGeometry(const int32 Index, TSharedPtr<TImplicitObject<T, d>, ESPMode::ThreadSafe> InShared)
		{
			check(!DynamicGeometry(Index));	// If dynamic geometry exists we should not be setting shared geometry on top
			MGeometry[Index] = MakeSerializable(InShared);
			MSharedGeometry[Index] = InShared;
			UpdateShapesArray(Index);
		}
		
		CHAOS_API void SetGeometry(const int32 Index, TSerializablePtr<TImplicitObject<T, d>> InGeometry)
		{
			check(!DynamicGeometry(Index));
			check(!SharedGeometry(Index));
			MGeometry[Index] = InGeometry;
			UpdateShapesArray(Index);
		}

		CHAOS_API const TBox<T,d>& LocalBounds(const int32 Index) const
		{
			return MLocalBounds[Index];
		}

		CHAOS_API TBox<T, d>& LocalBounds(const int32 Index)
		{
			return MLocalBounds[Index];
		}

		CHAOS_API bool HasBounds(const int32 Index) const
		{
			return MHasBounds[Index];
		}

		CHAOS_API bool& HasBounds(const int32 Index)
		{
			return MHasBounds[Index];
		}

		CHAOS_API FSpatialAccelerationIdx SpatialIdx(const int32 Index) const
		{
			return MSpatialIdx[Index];
		}

		CHAOS_API FSpatialAccelerationIdx& SpatialIdx(const int32 Index)
		{
			return MSpatialIdx[Index];
		}

		CHAOS_API const TBox<T, d>& WorldSpaceInflatedBounds(const int32 Index) const
		{
			return MWorldSpaceInflatedBounds[Index];
		}

		CHAOS_API TBox<T, d>& WorldSpaceInflatedBounds(const int32 Index)
		{
			return MWorldSpaceInflatedBounds[Index];
		}

		const TArray<TSerializablePtr<TImplicitObject<T, d>>>& GetAllGeometry() const { return MGeometry; }

		typedef TGeometryParticleHandle<T, d> THandleType;
		CHAOS_API THandleType* Handle(int32 Index) const { return const_cast<THandleType*>(MGeometryParticleHandle[Index].Get()); }

		CHAOS_API void SetHandle(int32 Index, TGeometryParticleHandle<T, d>* Handle);
		
		CHAOS_API TGeometryParticle<T, d>* GTGeometryParticle(const int32 Index) const { return MGeometryParticle[Index]; }
		CHAOS_API TGeometryParticle<T, d>*& GTGeometryParticle(const int32 Index) { return MGeometryParticle[Index]; }

		FString ToString(int32 index) const
		{
			FString BaseString = TParticles<T, d>::ToString(index);
			return FString::Printf(TEXT("%s, MR:%s, MGeometry:%s, IsDynamic:%d"), *BaseString, *R(index).ToString(), (Geometry(index) ? *(Geometry(index)->ToString()) : TEXT("none")), (DynamicGeometry(index) != nullptr));
		}

		CHAOS_API virtual void Serialize(FChaosArchive& Ar)
		{
			TParticles<T, d>::Serialize(Ar);
			Ar << MGeometry << MDynamicGeometry << MR;
			Ar.UsingCustomVersion(FPhysicsObjectVersion::GUID);
			if (Ar.CustomVer(FPhysicsObjectVersion::GUID) >= FPhysicsObjectVersion::PerShapeData)
			{
				Ar << MShapesArray;
			}

			if (Ar.CustomVer(FPhysicsObjectVersion::GUID) >= FPhysicsObjectVersion::SerializeGTGeometryParticles)
			{
				SerializeGeometryParticleHelper(Ar, this);
			}

			Ar.UsingCustomVersion(FExternalPhysicsCustomObjectVersion::GUID);
			if (Ar.CustomVer(FExternalPhysicsCustomObjectVersion::GUID) >= FExternalPhysicsCustomObjectVersion::SerializeParticleBounds)
			{
				Ar << MLocalBounds;
				Ar << MWorldSpaceInflatedBounds;
				Ar << MHasBounds;
			}
			else
			{
				//just assume all bounds come from geometry (technically wrong for pbd rigids with only sample points, but backwards compat is not that important right now)
				for (int32 Idx = 0; Idx < MGeometry.Num(); ++Idx)
				{
					MHasBounds[Idx] = MGeometry[Idx] && MGeometry[Idx]->HasBoundingBox();
					if (MHasBounds[Idx])
					{
						MLocalBounds[Idx] = MGeometry[Idx]->BoundingBox();
						MWorldSpaceInflatedBounds[Idx] = MLocalBounds[Idx].TransformedBox(TRigidTransform<T,d>(X(Idx), R(Idx)));	//ignore velocity too, really just trying to get something reasonable
					}
				}
			}

			if (Ar.CustomVer(FExternalPhysicsCustomObjectVersion::GUID) < FExternalPhysicsCustomObjectVersion::SpatialIdxSerialized)
			{
				MSpatialIdx.AddZeroed(MGeometry.Num());
			}
			else
			{
				Ar << MSpatialIdx;
			}
		}

		CHAOS_API EParticleType ParticleType() const { return MParticleType; }

	protected:
		EParticleType MParticleType;

	private:
		TArrayCollectionArray<TRotation<T, d>> MR;
		// MGeometry contains raw ptrs to every entry in both MSharedGeometry and MDynamicGeometry.
		// It may also contain raw ptrs to geometry which is managed outside of Chaos.
		TArrayCollectionArray<TSerializablePtr<TImplicitObject<T, d>>> MGeometry;
		// MSharedGeometry entries are owned by the solver, shared between *representations* of a particle.
		// This is NOT for sharing geometry resources between particle's A and B, this is for sharing the
		// geometry between particle A's various representations.
		TArrayCollectionArray<TSharedPtr<TImplicitObject<T, d>, ESPMode::ThreadSafe>> MSharedGeometry;
		// MDynamicGeometry entries are used for geo which is by the evolution. It is not set from the game side.
		TArrayCollectionArray<TUniquePtr<TImplicitObject<T, d>>> MDynamicGeometry;
		TArrayCollectionArray<TSerializablePtr<TGeometryParticleHandle<T, d>>> MGeometryParticleHandle;
		TArrayCollectionArray<TGeometryParticle<T, d>*> MGeometryParticle;
		TArrayCollectionArray<TShapesArray<T,d>> MShapesArray;
		TArrayCollectionArray<TBox<T,d>> MLocalBounds;
		TArrayCollectionArray<TBox<T, d>> MWorldSpaceInflatedBounds;
		TArrayCollectionArray<bool> MHasBounds;
		TArrayCollectionArray<FSpatialAccelerationIdx> MSpatialIdx;

		void UpdateShapesArray(const int32 Index)
		{
			UpdateShapesArrayFromGeometry(MShapesArray[Index], MGeometry[Index]);
		}

		template <typename T2, int d2, EGeometryParticlesSimType SimType2>
		friend class TGeometryParticlesImp;

		CHAOS_API void SerializeGeometryParticleHelper(FChaosArchive& Ar, TGeometryParticlesImp<T, d, EGeometryParticlesSimType::RigidBodySim>* GeometryParticles);
		
		void SerializeGeometryParticleHelper(FChaosArchive& Ar, TGeometryParticlesImp<T, d, EGeometryParticlesSimType::Other>* GeometryParticles)
		{
			check(false);	//cannot serialize this sim type
		}

#if CHAOS_DETERMINISTIC
		TArrayCollectionArray<FParticleID> MParticleIDs;
#endif

	};

	template <typename T, int d, EGeometryParticlesSimType SimType>
	FChaosArchive& operator<<(FChaosArchive& Ar, TGeometryParticlesImp<T, d, SimType>& Particles)
	{
		Particles.Serialize(Ar);
		return Ar;
	}

	template <>
	void TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::Other>::SetHandle(int32 Index, TGeometryParticleHandle<float, 3>* Handle);

	template<>
	TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::Other>* TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::Other>::SerializationFactory(FChaosArchive& Ar, TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::Other>* Particles);

	extern template class TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::RigidBodySim>;
	extern template class TGeometryParticlesImp<float, 3, EGeometryParticlesSimType::Other>;
}
