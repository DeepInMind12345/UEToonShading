// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ByteBuffer.h"
#include "RHI.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"
#include "RenderUtils.h"

class FMemsetBufferCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMemsetBufferCS,Global)
	
	FMemsetBufferCS() {}
	
	static bool ShouldCompilePermutation( const FGlobalShaderPermutationParameters& Parameters )
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment( const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Parameters, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("THREADGROUP_SIZE"), ThreadGroupSize );
	}

public:
	enum { ThreadGroupSize = 64 };

	FShaderParameter			Value;
	FShaderParameter			Size;
	FShaderParameter			DstOffset;
	FShaderResourceParameter	DstBuffer;

	FMemsetBufferCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		Value.Bind(		Initializer.ParameterMap, TEXT("Value") );
		Size.Bind(		Initializer.ParameterMap, TEXT("Size") );
		DstOffset.Bind(	Initializer.ParameterMap, TEXT("DstOffset") );
		DstBuffer.Bind(	Initializer.ParameterMap, TEXT("DstBuffer") );
	}

	virtual bool Serialize(FArchive& Ar) override
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Value;
		Ar << Size;
		Ar << DstOffset;
		Ar << DstBuffer;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FMemsetBufferCS, TEXT("/Engine/Private/ByteBuffer.usf"), TEXT("MemsetBufferCS"), SF_Compute );



void MemsetBuffer(FRHICommandList& RHICmdList, const FRWBufferStructured& DstBuffer, const FVector4& Value, uint32 NumFloat4s, uint32 DstOffsetInFloat4s)
{
	auto ShaderMap = GetGlobalShaderMap( GMaxRHIFeatureLevel );

	TShaderMapRef< FMemsetBufferCS > ComputeShader( ShaderMap );

	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader( ShaderRHI );

	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->Value, Value );
	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->Size, NumFloat4s );
	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->DstOffset, DstOffsetInFloat4s );
	SetUAVParameter( RHICmdList, ShaderRHI, ComputeShader->DstBuffer, DstBuffer.UAV );

	RHICmdList.DispatchComputeShader( FMath::DivideAndRoundUp< uint32 >( NumFloat4s, FMemsetBufferCS::ThreadGroupSize ), 1, 1 );

	SetUAVParameter( RHICmdList, ShaderRHI, ComputeShader->DstBuffer, FUnorderedAccessViewRHIRef() );
}


class FMemcpyBufferCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMemcpyBufferCS,Global)
	
	FMemcpyBufferCS() {}
	
	static bool ShouldCompilePermutation( const FGlobalShaderPermutationParameters& Parameters )
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment( const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Parameters, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("THREADGROUP_SIZE"), ThreadGroupSize );
	}

public:
	enum { ThreadGroupSize = 64 };

	FShaderParameter			Size;
	FShaderParameter			SrcOffset;
	FShaderParameter			DstOffset;
	FShaderResourceParameter	SrcBuffer;
	FShaderResourceParameter	DstBuffer;

	FMemcpyBufferCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		Size.Bind(		Initializer.ParameterMap, TEXT("Size") );
		SrcOffset.Bind(	Initializer.ParameterMap, TEXT("SrcOffset") );
		DstOffset.Bind(	Initializer.ParameterMap, TEXT("DstOffset") );
		SrcBuffer.Bind(	Initializer.ParameterMap, TEXT("SrcBuffer") );
		DstBuffer.Bind(	Initializer.ParameterMap, TEXT("DstBuffer") );
	}

	virtual bool Serialize(FArchive& Ar) override
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Size;
		Ar << SrcOffset;
		Ar << DstOffset;
		Ar << SrcBuffer;
		Ar << DstBuffer;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FMemcpyBufferCS, TEXT("/Engine/Private/ByteBuffer.usf"), TEXT("MemcpyBufferCS"), SF_Compute );

void MemcpyBuffer(FRHICommandList& RHICmdList, const FRWBufferStructured& SrcBuffer, const FRWBufferStructured& DstBuffer, uint32 NumFloat4s, uint32 SrcOffset, uint32 DstOffset)
{
	auto ShaderMap = GetGlobalShaderMap( GMaxRHIFeatureLevel );

	TShaderMapRef< FMemcpyBufferCS > ComputeShader( ShaderMap );
		
	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader( ShaderRHI );

	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EResourceTransitionPipeline::EGfxToCompute, DstBuffer.UAV);

	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->SrcOffset, SrcOffset);
	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->DstOffset, DstOffset);
	SetShaderValue( RHICmdList, ShaderRHI, ComputeShader->Size, NumFloat4s);
	SetSRVParameter( RHICmdList, ShaderRHI, ComputeShader->SrcBuffer, SrcBuffer.SRV );
	SetUAVParameter( RHICmdList, ShaderRHI, ComputeShader->DstBuffer, DstBuffer.UAV );

	RHICmdList.DispatchComputeShader( FMath::DivideAndRoundUp< uint32 >( NumFloat4s, FMemcpyBufferCS::ThreadGroupSize), 1, 1 );

	SetUAVParameter( RHICmdList, ShaderRHI, ComputeShader->DstBuffer, FUnorderedAccessViewRHIRef() );

	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, DstBuffer.UAV);
}

class FMemcpyTextureToTextureCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FMemcpyTextureToTextureCS, Global)

	FMemcpyTextureToTextureCS() {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), ThreadGroupSize);
	}

public:
	enum { ThreadGroupSize = 64 };

	FShaderParameter			Size;
	FShaderParameter			SrcOffset;
	FShaderParameter			DstOffset;
	FShaderParameter			FloatsPerLine;
	FShaderResourceParameter	SrcTexture;
	FShaderResourceParameter	DstTexture;

	FMemcpyTextureToTextureCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Size.Bind(Initializer.ParameterMap, TEXT("Size"));
		SrcOffset.Bind(Initializer.ParameterMap, TEXT("SrcOffset"));
		DstOffset.Bind(Initializer.ParameterMap, TEXT("DstOffset"));
		FloatsPerLine.Bind(Initializer.ParameterMap, TEXT("FloatsPerLine"));
		SrcTexture.Bind(Initializer.ParameterMap, TEXT("SrcTexture"));
		DstTexture.Bind(Initializer.ParameterMap, TEXT("DstTexture"));
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << Size;
		Ar << SrcOffset;
		Ar << DstOffset;
		Ar << FloatsPerLine;
		Ar << SrcTexture;
		Ar << DstTexture;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FMemcpyTextureToTextureCS, TEXT("/Engine/Private/ByteBuffer.usf"), TEXT("MemcpyTextureToTextureCS"), SF_Compute);

void MemcpyTextureToTexture(FRHICommandList& RHICmdList, const FTextureRWBuffer2D& SrcBuffer, const FTextureRWBuffer2D& DstBuffer, uint32 SrcOffset, uint32 DstOffset, uint32 NumFloat4s, uint32 FloatsPerLine)
{
	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	TShaderMapRef< FMemcpyTextureToTextureCS > ComputeShader(ShaderMap);

	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, EResourceTransitionPipeline::EGfxToCompute, DstBuffer.UAV);

	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->SrcOffset, SrcOffset);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->DstOffset, DstOffset);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->FloatsPerLine, FloatsPerLine);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->Size, NumFloat4s);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->SrcTexture, SrcBuffer.SRV);
	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, DstBuffer.UAV);

	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp< uint32 >(NumFloat4s, FMemcpyBufferCS::ThreadGroupSize), 1, 1);

	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, FUnorderedAccessViewRHIRef());

	RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, DstBuffer.UAV);
}

bool ResizeBufferIfNeeded(FRHICommandList& RHICmdList, FRWBufferStructured& Buffer, uint32 NumFloat4s)
{
	EPixelFormat BufferFormat = PF_A32B32G32R32F;
	uint32 BytesPerElement = GPixelFormats[BufferFormat].BlockBytes;

	if (Buffer.NumBytes == 0)
	{
		Buffer.Initialize(BytesPerElement, NumFloat4s);
	}
	else if (NumFloat4s * BytesPerElement != Buffer.NumBytes)
	{
		FRWBufferStructured NewBuffer;
		NewBuffer.Initialize(BytesPerElement, NumFloat4s);

		// Copy data to new buffer
		uint32 CopyFloat4s = FMath::Min(NumFloat4s, Buffer.NumBytes / BytesPerElement);
		MemcpyBuffer(RHICmdList, Buffer, NewBuffer, CopyFloat4s);

		Buffer = NewBuffer;
		return true;
	}

	return false;
}

bool ResizeTextureIfNeeded(FRHICommandList& RHICmdList, FTextureRWBuffer2D& Texture, uint32 NumFloat4s, uint32 PrimitiveStride)
{
	EPixelFormat BufferFormat = PF_A32B32G32R32F;
	uint32 BytesPerElement = GPixelFormats[BufferFormat].BlockBytes;

	uint32 TotalSize = 0;
	uint16 PrimitivesPerTextureLine = FPrimitiveSceneShaderData::GetPrimitivesPerTextureLine();
	uint32 SizeY = (NumFloat4s / (PrimitiveStride * PrimitivesPerTextureLine)) + 1;

	if (Texture.NumBytes == 0)
	{
		Texture.Initialize(BytesPerElement, PrimitiveStride * PrimitivesPerTextureLine, SizeY, PF_A32B32G32R32F, TexCreate_RenderTargetable | TexCreate_UAV);
	}
	else if ((SizeY * PrimitiveStride * PrimitivesPerTextureLine * BytesPerElement) != Texture.NumBytes)
	{
		FTextureRWBuffer2D NewTexture;
		NewTexture.Initialize(BytesPerElement, PrimitiveStride * PrimitivesPerTextureLine, SizeY, PF_A32B32G32R32F, TexCreate_RenderTargetable | TexCreate_UAV);
		MemcpyTextureToTexture(RHICmdList, Texture, NewTexture, 0, 0, NumFloat4s, PrimitiveStride * PrimitivesPerTextureLine);
		Texture = NewTexture;
		return true;
	}

	return false;
}

class FScatterCopyCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FScatterCopyCS,Global)
	
	FScatterCopyCS() {}
	
	static bool ShouldCompilePermutation( const FGlobalShaderPermutationParameters& Parameters )
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment( const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Parameters, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("THREADGROUP_SIZE"), ThreadGroupSize );
	}

public:
	enum { ThreadGroupSize = 64 };

	FShaderParameter			NumScatters;
	FShaderResourceParameter	ScatterBuffer;
	FShaderResourceParameter	UploadBuffer;
	FShaderResourceParameter	DstBuffer;
	
	FScatterCopyCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		NumScatters.Bind(Initializer.ParameterMap, TEXT("NumScatters") );
		ScatterBuffer.Bind(Initializer.ParameterMap, TEXT("ScatterBuffer") );
		UploadBuffer.Bind(Initializer.ParameterMap, TEXT("UploadBuffer") );
		DstBuffer.Bind(Initializer.ParameterMap, TEXT("DstBuffer") );
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << NumScatters;
		Ar << ScatterBuffer;
		Ar << UploadBuffer;
		Ar << DstBuffer;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FScatterCopyCS, TEXT("/Engine/Private/ByteBuffer.usf"), TEXT("ScatterCopyCS"), SF_Compute );

class FScatterCopyTextureCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FScatterCopyTextureCS, Global)

	FScatterCopyTextureCS() {}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return RHISupportsComputeShaders(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE"), ThreadGroupSize);
	}

public:
	enum { ThreadGroupSize = 64 };

	FShaderParameter			NumScatters;
	FShaderParameter			FloatsPerLine;
	FShaderParameter			Size;
	FShaderResourceParameter	ScatterBuffer;
	FShaderResourceParameter	UploadBuffer;
	FShaderResourceParameter	DstTexture;

	FScatterCopyTextureCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		NumScatters.Bind(Initializer.ParameterMap, TEXT("NumScatters"));
		FloatsPerLine.Bind(Initializer.ParameterMap, TEXT("FloatsPerLine"));
		Size.Bind(Initializer.ParameterMap, TEXT("Size"));
		ScatterBuffer.Bind(Initializer.ParameterMap, TEXT("ScatterBuffer"));
		UploadBuffer.Bind(Initializer.ParameterMap, TEXT("UploadBuffer"));
		DstTexture.Bind(Initializer.ParameterMap, TEXT("DstTexture"));
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << NumScatters;
		Ar << FloatsPerLine;
		Ar << Size;
		Ar << ScatterBuffer;
		Ar << UploadBuffer;
		Ar << DstTexture;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(, FScatterCopyTextureCS, TEXT("/Engine/Private/ByteBuffer.usf"), TEXT("ScatterCopyTextureCS"), SF_Compute);

FScatterUploadBuilder::FScatterUploadBuilder(uint32 NumUploads, uint32 InStrideInFloat4s, FReadBuffer& InScatterBuffer, FReadBuffer& InUploadBuffer)
	: ScatterBuffer(InScatterBuffer)
	, UploadBuffer(InUploadBuffer)
{
	StrideInFloat4s = InStrideInFloat4s;
	AllocatedNumScatters = NumUploads * StrideInFloat4s;

	EPixelFormat ScatterIndexFormat = PF_R32_UINT;
	uint32 ScatterBytes = AllocatedNumScatters * GPixelFormats[ScatterIndexFormat].BlockBytes;

	if (ScatterBytes > ScatterBuffer.NumBytes)
	{
		const uint32 BytesPerElement = GPixelFormats[ScatterIndexFormat].BlockBytes;
		const uint32 NumElements = FMath::RoundUpToPowerOfTwo(FMath::Max(ScatterBytes, 256u)) / BytesPerElement;

		// Resize Scatter Buffer
		ScatterBuffer.Release();
		ScatterBuffer.Initialize(BytesPerElement, NumElements, ScatterIndexFormat, BUF_Volatile);
	}

	EPixelFormat UploadDataFormat = PF_A32B32G32R32F;
	uint32 UploadBytes = NumUploads * StrideInFloat4s * GPixelFormats[UploadDataFormat].BlockBytes;

	if (UploadBytes > UploadBuffer.NumBytes)
	{
		// Resize Upload Buffer

		const uint32 BytesPerElement = GPixelFormats[UploadDataFormat].BlockBytes;
		const uint32 NumElements = FMath::RoundUpToPowerOfTwo(FMath::Max(UploadBytes, 256u)) / BytesPerElement;

		UploadBuffer.Release();
		UploadBuffer.Initialize(BytesPerElement, NumElements, UploadDataFormat, BUF_Volatile);
	}
	
	ScatterData = (uint32*)RHILockVertexBuffer(ScatterBuffer.Buffer, 0, ScatterBytes, RLM_WriteOnly);
	UploadData  = (FVector4*)RHILockVertexBuffer(UploadBuffer.Buffer, 0, UploadBytes, RLM_WriteOnly);

	// Track the actual number of scatters added
	NumScatters = 0;
}

void FScatterUploadBuilder::UploadTo(FRHICommandList& RHICmdList, FRWBufferStructured& DstBuffer)
{
	RHIUnlockVertexBuffer(ScatterBuffer.Buffer);
	RHIUnlockVertexBuffer(UploadBuffer.Buffer);

	ScatterData = nullptr;
	UploadData = nullptr;

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	TShaderMapRef<FScatterCopyCS> ComputeShader(ShaderMap);

	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->NumScatters, NumScatters);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->ScatterBuffer, ScatterBuffer.SRV);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->UploadBuffer, UploadBuffer.SRV);
	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstBuffer, DstBuffer.UAV);

	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp<uint32>(NumScatters, FScatterCopyCS::ThreadGroupSize), 1, 1);

	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstBuffer, FUnorderedAccessViewRHIRef());
}

int32 FTextureScatterUploadBuilder::GetMaxPrimitivesUpdate(uint32 NumUploads, uint32 InStrideInFloat4s)
{
	return GMaxTextureBufferSize == 0 ? NumUploads : FMath::Min((uint32)(GMaxTextureBufferSize / InStrideInFloat4s), NumUploads);
}

void FTextureScatterUploadBuilder::TextureUploadTo(FRHICommandList& RHICmdList, FTextureRWBuffer2D& DstTexture, uint32 NumFloat4, uint32 FloatsPerLine)
{
	RHIUnlockVertexBuffer(ScatterBuffer.Buffer);
	RHIUnlockVertexBuffer(UploadBuffer.Buffer);

	ScatterData = nullptr;
	UploadData = nullptr;

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	TShaderMapRef<FScatterCopyTextureCS> ComputeShader(ShaderMap);

	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	
	RHICmdList.SetComputeShader(ShaderRHI);

	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->NumScatters, NumScatters);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->ScatterBuffer, ScatterBuffer.SRV);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->FloatsPerLine, FloatsPerLine);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->Size, NumFloat4);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->UploadBuffer, UploadBuffer.SRV);
	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, DstTexture.UAV);

	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp<uint32>(NumScatters, FScatterCopyTextureCS::ThreadGroupSize), 1, 1);

	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, FUnorderedAccessViewRHIRef());
}

void FScatterUploadBuilder::UploadTo_Flush(FRHICommandList& RHICmdList, FRWBufferStructured& DstBuffer)
{
	RHIUnlockVertexBuffer(ScatterBuffer.Buffer);
	RHIUnlockVertexBuffer(UploadBuffer.Buffer);

	ScatterData = nullptr;
	UploadData = nullptr;

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	TShaderMapRef<FScatterCopyCS> ComputeShader(ShaderMap);
		
	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->NumScatters, NumScatters);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->ScatterBuffer, ScatterBuffer.SRV);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->UploadBuffer, UploadBuffer.SRV);
	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstBuffer, DstBuffer.UAV);

	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp<uint32>(NumScatters, FScatterCopyCS::ThreadGroupSize), 1, 1);

	FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);

	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstBuffer, FUnorderedAccessViewRHIRef());
}

void FTextureScatterUploadBuilder::TextureUploadTo_Flush(FRHICommandList& RHICmdList, FTextureRWBuffer2D& DstTexture, uint32 NumFloat4, uint32 FloatsPerLine)
{
	RHIUnlockVertexBuffer(ScatterBuffer.Buffer);
	RHIUnlockVertexBuffer(UploadBuffer.Buffer);

	ScatterData = nullptr;
	UploadData = nullptr;

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	TShaderMapRef<FScatterCopyTextureCS> ComputeShader(ShaderMap);

	FRHIComputeShader* ShaderRHI = ComputeShader->GetComputeShader();
	RHICmdList.SetComputeShader(ShaderRHI);

	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->NumScatters, NumScatters);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->ScatterBuffer, ScatterBuffer.SRV);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->FloatsPerLine, FloatsPerLine);
	SetShaderValue(RHICmdList, ShaderRHI, ComputeShader->Size, NumFloat4);
	SetSRVParameter(RHICmdList, ShaderRHI, ComputeShader->UploadBuffer, UploadBuffer.SRV);
	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, DstTexture.UAV);

	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp<uint32>(NumScatters, FScatterCopyTextureCS::ThreadGroupSize), 1, 1);

	FRHICommandListExecutor::GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);

	SetUAVParameter(RHICmdList, ShaderRHI, ComputeShader->DstTexture, FUnorderedAccessViewRHIRef());
}