// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneRenderTargets.h: Scene render target definitions.
=============================================================================*/

#pragma once

#include "ShaderParameters.h"
#include "RenderTargetPool.h"
#include "../SystemTextures.h"
#include "RHIStaticStates.h"

/** Number of cube map shadow depth surfaces that will be created and used for rendering one pass point light shadows. */
static const int32 NumCubeShadowDepthSurfaces = 5;

/** 
 * Allocate enough sets of translucent volume textures to cover all the cascades, 
 * And then one more which will be used as a scratch target when doing ping-pong operations like filtering. 
 */
static const int32 NumTranslucentVolumeRenderTargetSets = TVC_MAX + 1;

/** Forward declaration of console variable controlling translucent volume blur */
extern int32 GUseTranslucencyVolumeBlur;

/** Forward declaration of console variable controlling translucent lighting volume dimensions */
extern int32 GTranslucencyLightingVolumeDim;

/** Function to select the index of the volume texture that we will hold the final translucency lighting volume texture */
inline int SelectTranslucencyVolumeTarget(ETranslucencyVolumeCascade InCascade)
{
	if (GUseTranslucencyVolumeBlur)
	{
		switch (InCascade)
		{
		case TVC_Inner:
			{
				return 2;
			}
		case TVC_Outer:
			{
				return 0;
			}
		default:
			{
				// error
				check(false);
				return 0;
			}
		}
	}
	else
	{
		switch (InCascade)
		{
		case TVC_Inner:
			{
				return 0;
			}
		case TVC_Outer:
			{
				return 1;
			}
		default:
			{
				// error
				check(false);
				return 0;
			}
		}
	}
}

/** Number of surfaces used for translucent shadows. */
static const int32 NumTranslucencyShadowSurfaces = 2;

/** Downsample factor to use on the volume texture used for computing GI on translucency. */
static const int32 TranslucentVolumeGISratchDownsampleFactor = 1;

BEGIN_UNIFORM_BUFFER_STRUCT(FGBufferResourceStruct, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferATexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferBTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferCTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferDTexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D,			GBufferETexture )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferATextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferBTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferCTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferDTextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D<float4>,	GBufferETextureNonMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferATextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferBTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferCTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferDTextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2DMS<float4>,	GBufferETextureMS )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferATextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferBTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferCTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferDTextureSampler )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_SAMPLER( SamplerState,			GBufferETextureSampler )
END_UNIFORM_BUFFER_STRUCT( FGBufferResourceStruct )

/**
 * Encapsulates the render targets used for scene rendering.
 */
class FSceneRenderTargets : public FRenderResource
{
public:
	/** Destructor. */
	virtual ~FSceneRenderTargets() {}

protected:
	/** Constructor */
	FSceneRenderTargets(): 
		bScreenSpaceAOIsValid(false),
		bCustomDepthIsValid(false),
		bPreshadowCacheNewlyAllocated(false),
		GBufferRefCount(0),
		BufferSize(0, 0), 
		SmallColorDepthDownsampleFactor(2),
		bLightAttenuationEnabled(true),
		bUseDownsizedOcclusionQueries(true),
		CurrentGBufferFormat(0),
		CurrentSceneColorFormat(0),
		bAllowStaticLighting(true),
		CurrentMaxShadowResolution(0),
		CurrentTranslucencyLightingVolumeDim(64),
		CurrentMobile32bpp(0),
		bCurrentLightPropagationVolume(false)
		{}
public:

	/**
	 * Checks that scene render targets are ready for rendering a view family of the given dimensions.
	 * If the allocated render targets are too small, they are reallocated.
	 */
	void Allocate(const FSceneViewFamily& ViewFamily);
	/**
	 *
	 */
	void SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY);
	/**
	 * Clears the GBuffer render targets to default values.
	 */
	void ClearGBufferTargets(const FLinearColor& ClearColor);
	/**
	 * Sets the scene color target and restores its contents if necessary
	 * @param bGBufferPass - Whether the pass about to be rendered is the GBuffer population pass
	 */
	void BeginRenderingSceneColor(bool bGBufferPass = false);
	/**
	 * Called when finished rendering to the scene color surface
	 * @param bKeepChanges - if true then the SceneColorSurface is resolved to the SceneColorTexture
	 */
	void FinishRenderingSceneColor(bool bKeepChanges = true, const FResolveRect& ResolveRect = FResolveRect());

	// @return true: call FinishRenderingCustomDepth after rendering, false: don't render it, feature is disabled
	bool BeginRenderingCustomDepth(bool bPrimitives);
	// only call if BeginRenderingCustomDepth() returned true
	void FinishRenderingCustomDepth(const FResolveRect& ResolveRect = FResolveRect());

	/**
	 * Resolve a previously rendered scene color surface.
	 */
	void ResolveSceneColor(const FResolveRect& ResolveRect = FResolveRect());

	/** Resolves the GBuffer targets so that their resolved textures can be sampled. */
	void ResolveGBufferSurfaces(const FResolveRect& ResolveRect = FResolveRect());

	void BeginRenderingShadowDepth();

	/** Binds the appropriate shadow depth cube map for rendering. */
	void BeginRenderingCubeShadowDepth(int32 ShadowResolution);

	/**
	 * Called when finished rendering to the subject shadow depths so the surface can be copied to texture
	 * @param ResolveParams - optional resolve params
	 */
	void FinishRenderingShadowDepth(const FResolveRect& ResolveRect = FResolveRect());

	void BeginRenderingReflectiveShadowMap( class FLightPropagationVolume* Lpv );
	void FinishRenderingReflectiveShadowMap( const FResolveRect& ResolveRect = FResolveRect() );

	/** Resolves the appropriate shadow depth cube map and restores default state. */
	void FinishRenderingCubeShadowDepth(int32 ShadowResolution, const FResolveParams& ResolveParams = FResolveParams());
	
	void BeginRenderingTranslucency(const class FViewInfo& View);

	bool BeginRenderingSeparateTranslucency(const FViewInfo& View, bool bFirstTimeThisFrame);
	void FinishRenderingSeparateTranslucency(const FViewInfo& View);
	void FreeSeparateTranslucency();

	void ResolveSceneDepthTexture();
	void ResolveSceneDepthToAuxiliaryTexture();

	void BeginRenderingPrePass();
	void FinishRenderingPrePass();

	void BeginRenderingSceneAlphaCopy();
	void FinishRenderingSceneAlphaCopy();

	void BeginRenderingLightAttenuation();
	void FinishRenderingLightAttenuation();

	/**
	 * Cleans up editor primitive targets that we no longer need
	 */
	void CleanUpEditorPrimitiveTargets();

	/**
	 * Affects the render quality of the editor 3d objects. MSAA is needed if >1
	 * @return clamped to reasonable numbers
	 */
	int32 GetEditorMSAACompositingSampleCount() const;

	bool IsStaticLightingAllowed() const { return bAllowStaticLighting; }

	/**
	 * Gets the editor primitives color target/shader resource.  This may recreate the target
	 * if the msaa settings dont match
	 */
	const FTexture2DRHIRef& GetEditorPrimitivesColor();

	/**
	 * Gets the editor primitives depth target/shader resource.  This may recreate the target
	 * if the msaa settings dont match
	 */
	const FTexture2DRHIRef& GetEditorPrimitivesDepth();


	// FRenderResource interface.
	virtual void InitDynamicRHI();
	virtual void ReleaseDynamicRHI();

	// Texture Accessors -----------

	const FTextureRHIRef& GetSceneColorTexture() const;
	const FTexture2DRHIRef& GetSceneAlphaCopyTexture() const { return (const FTexture2DRHIRef&)SceneAlphaCopy->GetRenderTargetItem().ShaderResourceTexture; }
	bool HasSceneAlphaCopyTexture() const { return SceneAlphaCopy.GetReference() != 0; }
	const FTexture2DRHIRef& GetSceneDepthTexture() const { return (const FTexture2DRHIRef&)SceneDepthZ->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetAuxiliarySceneDepthTexture() const 
	{ 
		check(!GSupportsDepthFetchDuringDepthTest);
		return (const FTexture2DRHIRef&)AuxiliarySceneDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
	}
	const FTexture2DRHIRef& GetShadowDepthZTexture(bool bInPreshadowCache = false) const 
	{ 
		if (bInPreshadowCache)
		{
			return (const FTexture2DRHIRef&)PreShadowCacheDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
		}
		else
		{
			return (const FTexture2DRHIRef&)ShadowDepthZ->GetRenderTargetItem().ShaderResourceTexture; 
		}
	}
	const FTexture2DRHIRef* GetActualDepthTexture() const
	{
		const FTexture2DRHIRef* DepthTexture = NULL;
		if((GRHIFeatureLevel >= ERHIFeatureLevel::SM4) || IsPCPlatform(GRHIShaderPlatform))
		{
			if(GSupportsDepthFetchDuringDepthTest)
			{
				DepthTexture = &GetSceneDepthTexture();
			}
			else
			{
				DepthTexture = &GetAuxiliarySceneDepthSurface();
			}
		}
		check(DepthTexture != NULL);

		return DepthTexture;
	}
	const FTexture2DRHIRef& GetReflectiveShadowMapDepthTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDepth->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapNormalTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapNormal->GetRenderTargetItem().ShaderResourceTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDiffuseTexture() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDiffuse->GetRenderTargetItem().ShaderResourceTexture; }

	const FTextureCubeRHIRef& GetCubeShadowDepthZTexture(int32 ShadowResolution) const 
	{ 
		return (const FTextureCubeRHIRef&)CubeShadowDepthZ[GetCubeShadowDepthZIndex(ShadowResolution)]->GetRenderTargetItem().ShaderResourceTexture; 
	}
	const FTexture2DRHIRef& GetGBufferATexture() const { return (const FTexture2DRHIRef&)GBufferA->GetRenderTargetItem().ShaderResourceTexture; }

	/** 
	* Allows substitution of a 1x1 white texture in place of the light attenuation buffer when it is not needed;
	* this improves shader performance and removes the need for redundant Clears
	*/
	void SetLightAttenuationMode(bool bEnabled) { bLightAttenuationEnabled = bEnabled; }
	const FTextureRHIRef& GetEffectiveLightAttenuationTexture(bool bReceiveDynamicShadows) const 
	{
		if( bLightAttenuationEnabled && bReceiveDynamicShadows )
		{
			return GetLightAttenuationTexture();
		}
		else
		{
			return GWhiteTexture->TextureRHI;
		}
	}
	const FTextureRHIRef& GetLightAttenuationTexture() const
	{
		return *(FTextureRHIRef*)&GetLightAttenuation()->GetRenderTargetItem().ShaderResourceTexture;
	}

	const FTextureRHIRef& GetSceneColorSurface() const;
	const FTexture2DRHIRef& GetSceneAlphaCopySurface() const						{ return (const FTexture2DRHIRef&)SceneAlphaCopy->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetSceneDepthSurface() const							{ return (const FTexture2DRHIRef&)SceneDepthZ->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetSmallDepthSurface() const							{ return (const FTexture2DRHIRef&)SmallDepthZ->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetShadowDepthZSurface() const						
	{ 
		return (const FTexture2DRHIRef&)ShadowDepthZ->GetRenderTargetItem().TargetableTexture; 
	}

	const FTexture2DRHIRef& GetReflectiveShadowMapNormalSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapNormal->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDiffuseSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDiffuse->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetReflectiveShadowMapDepthSurface() const { return (const FTexture2DRHIRef&)ReflectiveShadowMapDepth->GetRenderTargetItem().TargetableTexture; }

	const FTextureCubeRHIRef& GetCubeShadowDepthZSurface(int32 ShadowResolution) const						
	{ 
		return (const FTextureCubeRHIRef&)CubeShadowDepthZ[GetCubeShadowDepthZIndex(ShadowResolution)]->GetRenderTargetItem().TargetableTexture; 
	}
	const FTexture2DRHIRef& GetLightAttenuationSurface() const					{ return (const FTexture2DRHIRef&)GetLightAttenuation()->GetRenderTargetItem().TargetableTexture; }
	const FTexture2DRHIRef& GetAuxiliarySceneDepthSurface() const 
	{	
		check(!GSupportsDepthFetchDuringDepthTest); 
		return (const FTexture2DRHIRef&)AuxiliarySceneDepthZ->GetRenderTargetItem().TargetableTexture; 
	}

	// @return can be 0 if the feature is disabled
	IPooledRenderTarget* RequestCustomDepth(bool bPrimitives);

	// ---

	/** */
	bool UseDownsizedOcclusionQueries() const { return bUseDownsizedOcclusionQueries; }

	// ---

	/** Get the current translucent ambient lighting volume texture. Can vary depending on whether volume filtering is enabled */
	TRefCountPtr<IPooledRenderTarget> GetTranslucencyVolumeAmbient(ETranslucencyVolumeCascade Cascade) { return TranslucencyLightingVolumeAmbient[SelectTranslucencyVolumeTarget(Cascade)]; }

	/** Get the current translucent directional lighting volume texture. Can vary depending on whether volume filtering is enabled */
	TRefCountPtr<IPooledRenderTarget> GetTranslucencyVolumeDirectional(ETranslucencyVolumeCascade Cascade) { return TranslucencyLightingVolumeDirectional[SelectTranslucencyVolumeTarget(Cascade)]; }

	// ---
	/** Get the uniform buffer containing GBuffer resources. */
	FUniformBufferRHIParamRef GetGBufferResourcesUniformBuffer() const 
	{ 
		check(IsValidRef(GBufferResourcesUniformBuffer));
		return GBufferResourcesUniformBuffer; 
	}
	/** */
	FIntPoint GetBufferSizeXY() const { return BufferSize; }
	/** */
	uint32 GetSmallColorDepthDownsampleFactor() const { return SmallColorDepthDownsampleFactor; }
	/** Returns an index in the range [0, NumCubeShadowDepthSurfaces) given an input resolution. */
	int32 GetCubeShadowDepthZIndex(int32 ShadowResolution) const;
	/** Returns the appropriate resolution for a given cube shadow index. */
	int32 GetCubeShadowDepthZResolution(int32 ShadowIndex) const;
	/** Returns the size of the shadow depth buffer, taking into account platform limitations and game specific resolution limits. */
	FIntPoint GetShadowDepthTextureResolution() const;
	FIntPoint GetPreShadowCacheTextureResolution() const;
	FIntPoint GetTranslucentShadowDepthTextureResolution() const;

	/** Returns the size of the RSM buffer, taking into account platform limitations and game specific resolution limits. */
	FIntPoint GetReflectiveShadowMapTextureResolution() const;

	int32 GetNumGBufferTargets() const;

	// ---

	// needs to be called between AllocSceneColor() and ReleaseSceneColor()
	const TRefCountPtr<IPooledRenderTarget>& GetSceneColor() const;

	TRefCountPtr<IPooledRenderTarget>& GetSceneColor();

	void SetSceneColor(IPooledRenderTarget* In);

	// ---

	void SetLightAttenuation(IPooledRenderTarget* In);

	// needs to be called between AllocSceneColor() and SetSceneColor(0)
	const TRefCountPtr<IPooledRenderTarget>& GetLightAttenuation() const;

	TRefCountPtr<IPooledRenderTarget>& GetLightAttenuation();

	// ---

	// allows to release the GBuffer once post process materials no longer need it
	// @param 1: add a reference, -1: remove a reference
	void AdjustGBufferRefCount(int Delta);

	//
	void AllocGBufferTargets();

private: // Get...() methods instead of direct access

	// 0 before BeginRenderingSceneColor and after tone mapping, for ES2 it's not released during the frame
	TRefCountPtr<IPooledRenderTarget> SceneColor;
	// also used as LDR scene color
	TRefCountPtr<IPooledRenderTarget> LightAttenuation;
public:

	//
	TRefCountPtr<IPooledRenderTarget> SceneDepthZ;
	// Mobile without frame buffer fetch (to get depth from alpha).
	TRefCountPtr<IPooledRenderTarget> SceneAlphaCopy;
	// Auxiliary scene depth target. The scene depth is resolved to this surface when targeting SM4. 
	TRefCountPtr<IPooledRenderTarget> AuxiliarySceneDepthZ;
	// Quarter-sized version of the scene depths
	TRefCountPtr<IPooledRenderTarget> SmallDepthZ;

	// GBuffer: Geometry Buffer rendered in base pass for deferred shading, only available between AllocGBufferTargets() and FreeGBufferTargets()
	TRefCountPtr<IPooledRenderTarget> GBufferA;
	TRefCountPtr<IPooledRenderTarget> GBufferB;
	TRefCountPtr<IPooledRenderTarget> GBufferC;
	TRefCountPtr<IPooledRenderTarget> GBufferD;
	TRefCountPtr<IPooledRenderTarget> GBufferE;

	// DBuffer: For decals before base pass (only temporarily available after early z pass and until base pass)
	TRefCountPtr<IPooledRenderTarget> DBufferA;
	TRefCountPtr<IPooledRenderTarget> DBufferB;
	TRefCountPtr<IPooledRenderTarget> DBufferC;

	// for AmbientOcclusion, only valid for a short time during the frame to allow reuse
	TRefCountPtr<IPooledRenderTarget> ScreenSpaceAO;
	// used by the CustomDepth material feature, is allocated on demand or if r.CustomDepth is 2
	TRefCountPtr<IPooledRenderTarget> CustomDepth;
	// Render target for per-object shadow depths.
	TRefCountPtr<IPooledRenderTarget> ShadowDepthZ;
	// Cache of preshadow depths
	//@todo - this should go in FScene
	TRefCountPtr<IPooledRenderTarget> PreShadowCacheDepthZ;
	// Stores accumulated density for shadows from translucency
	TRefCountPtr<IPooledRenderTarget> TranslucencyShadowTransmission[NumTranslucencyShadowSurfaces];

	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapNormal;
	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapDiffuse;
	TRefCountPtr<IPooledRenderTarget> ReflectiveShadowMapDepth;

	// Render target for one pass point light shadows, 0:at the highest resolution 4:at the lowest resolution
	TRefCountPtr<IPooledRenderTarget> CubeShadowDepthZ[NumCubeShadowDepthSurfaces];

	/** 2 scratch cubemaps used for filtering reflections. */
	TRefCountPtr<IPooledRenderTarget> ReflectionColorScratchCubemap[2];

	/** Temporary storage during SH irradiance map generation. */
	TRefCountPtr<IPooledRenderTarget> DiffuseIrradianceScratchCubemap[2];

	/** Temporary storage during SH irradiance map generation. */
	TRefCountPtr<IPooledRenderTarget> SkySHIrradianceMap;

	/** Temporary storage, used during reflection capture filtering. */
	TRefCountPtr<IPooledRenderTarget> ReflectionBrightness;

	/** Volume textures used for lighting translucency. */
	TRefCountPtr<IPooledRenderTarget> TranslucencyLightingVolumeAmbient[NumTranslucentVolumeRenderTargetSets];
	TRefCountPtr<IPooledRenderTarget> TranslucencyLightingVolumeDirectional[NumTranslucentVolumeRenderTargetSets];

	/** Volume texture used to gather GI at a lower resolution than the rest of translucent lighting. */
	TRefCountPtr<IPooledRenderTarget> TranslucencyLightingVolumeGIScratch;

	/** Color and opacity for editor primitives (i.e editor gizmos). */
	TRefCountPtr<IPooledRenderTarget> EditorPrimitivesColor;

	/** Depth for editor primitives */
	TRefCountPtr<IPooledRenderTarget> EditorPrimitivesDepth;

	bool IsSeparateTranslucencyActive(const FViewInfo& View) const;

	// todo: free ScreenSpaceAO so pool can reuse
	bool bScreenSpaceAOIsValid;

	// todo: free ScreenSpaceAO so pool can reuse
	bool bCustomDepthIsValid;

	/** Whether the preshadow cache render target has been newly allocated and cached shadows need to be re-rendered. */
	bool bPreshadowCacheNewlyAllocated;

private:
	/** used by AdjustGBufferRefCount */
	int32 GBufferRefCount;

	/**
	 * Takes the requested buffer size and quantizes it to an appropriate size for the rest of the
	 * rendering pipeline. Currently ensures that sizes are multiples of 8 so that they can safely
	 * be halved in size several times.
	 */
	void QuantizeBufferSize(int32& InOutBufferSizeX, int32& InOutBufferSizeY) const;

	/**
	 * Initializes the editor primitive color render target
	 */
	void InitEditorPrimitivesColor();

	/**
	 * Initializes the editor primitive depth buffer
	 */
	void InitEditorPrimitivesDepth();

	/** Allocates render targets for use with the forward shading path. */
	void AllocateForwardShadingPathRenderTargets();

	/** Allocates render targets for use with the deferred shading path. */
	void AllocateDeferredShadingPathRenderTargets();

	/** Determine the appropriate render target dimensions. */
	FIntPoint GetSceneRenderTargetSize(const FSceneViewFamily & ViewFamily) const;

	void AllocSceneColor();

	void AllocLightAttenuation();

	// internal method, used by AdjustGBufferRefCount()
	void FreeGBufferTargets();

	EPixelFormat GetSceneColorFormat() const;

private:
	/** Uniform buffer containing GBuffer resources. */
	FUniformBufferRHIRef GBufferResourcesUniformBuffer;
	/** size of the back buffer, in editor this has to be >= than the biggest view port */
	FIntPoint BufferSize;
	/** e.g. 2 */
	uint32 SmallColorDepthDownsampleFactor;
	/** if true we use the light attenuation buffer otherwise the 1x1 white texture is used */
	bool bLightAttenuationEnabled;
	/** Whether to use SmallDepthZ for occlusion queries. */
	bool bUseDownsizedOcclusionQueries;
	/** To detect a change of the CVar r.GBufferFormat */
	int CurrentGBufferFormat;
	/** To detect a change of the CVar r.SceneColorFormat */
	int CurrentSceneColorFormat;
	/** Whether render targets were allocated with static lighting allowed. */
	bool bAllowStaticLighting;
	/** To detect a change of the CVar r.Shadow.MaxResolution */
	int32 CurrentMaxShadowResolution;
	/** To detect a change of the CVar r.TranslucencyLightingVolumeDim */
	int32 CurrentTranslucencyLightingVolumeDim;
	/** To detect a change of the CVar r.MobileHDR / r.MobileHDR32bpp */
	int32 CurrentMobile32bpp;
	/** To detect a change of the CVar r.MobileMSAA */
	int32 CurrentMobileMSAA;
	/** To detect a change of the CVar r.Shadow.MinResolution */
	int32 CurrentMinShadowResolution;
	/** To detect a change of the CVar r.LightPropagationVolume */
	bool bCurrentLightPropagationVolume;
};

/** The global render targets used for scene rendering. */
extern TGlobalResource<FSceneRenderTargets> GSceneRenderTargets;
