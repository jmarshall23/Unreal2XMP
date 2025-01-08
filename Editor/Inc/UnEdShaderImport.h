/*=============================================================================
	UnEdShaderImport
	Copyright 2002 Demiurge Studios. All Rights Reserved.

	Revision history:
		* Created by Albert Reed
=============================================================================*/

// Sorry for the uglyness of this class, it will probably be replaced when ATI releases
// The RenderMonkey API. It was put together quickly in a stand-alone program and
// then shoved into a class...so it's not the prettiest block of code ever.

// DS_SHADER

#include "expat.h"

class FShaderLoader
{

public:

	XML_Parser p;

	INT Depth;

	class Constant
	{
	public:
		
		Constant()
		{
		}
		FString Name;

		enum EType
		{
			MATRIX,
			VECTOR,
			FLOATX,
			INTX
		};

		EType Type;


		FLOAT Values[16];
	};


	class StreamMapping
	{
	public:
		class RegisterMapping
		{
		public:
			INT Register;
			INT Type;
			INT Usage;
		};

		RegisterMapping RegMappings[20];
		INT NumRegMappings;

		FString Name;

		StreamMapping() : NumRegMappings(0) {}
	};

	StreamMapping AllMappings[1000];
	INT NumMappings;
	StreamMapping* CurrentMapping;

	Constant AllConstants[1000];
	INT NumConstants;

	Constant* CurrentConstant;
	INT CurrentConstMember;

	INT* RegisterValue;

	UBOOL InStreamMapRef;
	UBOOL InTextureRef;

	class ConstantMapping
	{
	public:

		ConstantMapping() : MyConstant(NULL), Register(-1) {}

		Constant* MyConstant;
		INT Register;
	};


	class Texture
	{
	public:
		FString Name;
		FString Filename;
	};	


	class Shader
	{
	public:

		Shader() :
		  NumPasses(0), CurrentPass(NULL), NumConstants(0)	  
		{
		}

		virtual ~Shader()
		{


		}

		class Pass
		{
		public:

			Pass() : 
				NumPSConstantMappings(0), NumVSConstantMappings(0), CurrentMappingArray(NULL), 
				CurrentMappingArrayCounter(NULL), MyMapping(NULL), CurrentTextureUnit(-1), CurrentRenderState(-1)
			{
				VertexShaderConstantMappings = new ConstantMapping[100];
				PixelShaderConstantMappings = new ConstantMapping[100];

				for(INT i = 0; i < 8; i++)
					Textures[i] = NULL;

				for(INT i = 0; i < 200; i++)
					RenderStates[i] = -1;
			}

			virtual ~Pass()
			{
				delete [] VertexShaderConstantMappings;
				VertexShaderConstantMappings = NULL;

				delete [] PixelShaderConstantMappings;
				PixelShaderConstantMappings = NULL;

			}

			FString PixelShaderText;
			FString VertexShaderText;

			ConstantMapping* VertexShaderConstantMappings;
			INT NumVSConstantMappings;

			ConstantMapping* PixelShaderConstantMappings;
			INT NumPSConstantMappings;

			ConstantMapping* CurrentMappingArray;
			INT* CurrentMappingArrayCounter;

			StreamMapping* MyMapping;
				
			Texture* Textures[8];
			INT CurrentTextureUnit;

			// Raw render state values
			DWORD RenderStates[200];
			INT CurrentRenderState;

			// post-processed useful render-state values.
			UBOOL AlphaBlending;
			UBOOL AlphaTest;
			BYTE AlphaRef;
			UBOOL ZTest;
			UBOOL ZWrite;
			DWORD SrcBlend,	DestBlend;
		};

		FString Name;

		Pass Passes[4];
		INT NumPasses;
		Pass* CurrentPass;

		Constant Constants[100];
		INT NumConstants;
	};

	Texture Textures[1000];
	INT NumTextures;
	Texture* CurrentTexture;

	Shader Shaders[1000];
	INT NumShaders;
	Shader* CurrentShader;

	FString* CurrentCode;

	UBOOL VarIsColor;

	UBOOL InRenderState;

	Shader::Pass* PrevPass;
	
	Texture* FindTextureRef(const FString& MappingName)
	{
		for(INT i = 0; i < NumTextures; i++)
		{
			if(MappingName == Textures[i].Name)
				return &Textures[i];
		}

		debugf(TEXT("Failed to find texture mapping %s"), *MappingName);

		return NULL;

	}

	void HandlePass(const char** attr)
	{
		CurrentShader->CurrentPass = &(CurrentShader->Passes[CurrentShader->NumPasses]);
		// Copy the previous render-states
		if(PrevPass)
		{
			appMemcpy(CurrentShader->CurrentPass->RenderStates, PrevPass->RenderStates, sizeof(DWORD) * 200);
		}
		CurrentShader->NumPasses++;
		PrevPass = CurrentShader->CurrentPass;
	}


	void HandleStreamChannel(const char** attr)
	{
		check(CurrentMapping);
		CurrentMapping->NumRegMappings++;
	}

	void HandleStreamChannelType(const char** attr)
	{
		check(CurrentMapping);
		check(CurrentMapping->NumRegMappings > 0);

		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				CurrentMapping->RegMappings[CurrentMapping->NumRegMappings-1].Type = atoi(attr[i+1]);
			}
		}
	}

	void HandleState(const char** attr)
	{
		check(CurrentShader);
		check(CurrentShader->CurrentPass);
		if(InRenderState)
		{
			for(INT i = 0; attr[i]; i+=2)
			{
				if(FString(attr[i]) == FString("CURRENT"))
				{
					CurrentShader->CurrentPass->CurrentRenderState = atoi(attr[i+1]);
				}
			}
		}
	}

	void HandleValue(const char** attr)
	{
		check(CurrentShader);
		check(CurrentShader->CurrentPass);
	
		if(InRenderState)
		{
			for(INT i = 0; attr[i]; i+=2)
			{
				if(FString(attr[i]) == FString("CURRENT"))
				{
					CurrentShader->CurrentPass->RenderStates[CurrentShader->CurrentPass->CurrentRenderState] = atoi(attr[i+1]);
				}
			}
		}
	}


	void HandleStreamChannelReg(const char** attr)
	{
		check(CurrentMapping);

		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				CurrentMapping->RegMappings[CurrentMapping->NumRegMappings-1].Register = atoi(attr[i+1]);
			}
		}
	}



	void HandleStreamChannelUsage(const char** attr)
	{
		check(CurrentMapping);

		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				CurrentMapping->RegMappings[CurrentMapping->NumRegMappings-1].Usage = atoi(attr[i+1]);
			}
		}
	}


	void HandleStreamMapping(const char** attr)
	{
		CurrentMapping = &(AllMappings[NumMappings]);
		NumMappings++;

		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("DESC"))
			{
				CurrentMapping->Name = attr[i+1];
			}
		}
	}



	void HandleVariableElement(const char** attr)
	{
		check(CurrentConstant);
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				CurrentConstant->Values[CurrentConstMember] = (FLOAT)atof(attr[i+1]);
				CurrentConstMember++;
			}
		}
	}



	void HandlePixelShader(const char** attr)
	{
		check(CurrentShader);
		CurrentCode = &(CurrentShader->CurrentPass->PixelShaderText);
		CurrentShader->CurrentPass->CurrentMappingArray = CurrentShader->CurrentPass->PixelShaderConstantMappings;
		CurrentShader->CurrentPass->CurrentMappingArrayCounter = &(CurrentShader->CurrentPass->NumPSConstantMappings);
	}



	void HandleVertexShader(const char** attr)
	{
		check(CurrentShader);
		CurrentCode = &(CurrentShader->CurrentPass->VertexShaderText);
		CurrentShader->CurrentPass->CurrentMappingArray = CurrentShader->CurrentPass->VertexShaderConstantMappings;
		CurrentShader->CurrentPass->CurrentMappingArrayCounter = &(CurrentShader->CurrentPass->NumVSConstantMappings);
	}



	void HandleEffect(const char** attr)
	{
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("DESC"))
			{
				CurrentShader = &(Shaders[NumShaders]);
				CurrentShader->Name = FString(attr[i+1]);
				NumShaders++;
			}
		}
	}


	void NewVariable(Constant::EType Type)
	{
		// If we're currently in the middle of the shader
		if(CurrentShader)
		{	
			// Set the current constant to the next shader-specific constant
			CurrentConstant = &(CurrentShader->Constants[CurrentShader->NumConstants]);
			CurrentShader->NumConstants++;      		
		}
		else // if we're not currently in the middle of the shader
		{
			// new global constant
			CurrentConstant = &(AllConstants[NumConstants]);
			NumConstants++;
		}

		CurrentConstant->Type = Type;
		CurrentConstMember = 0;
	}



	void HandleVariable(const char** attr)
	{
		// Function can return in the middle!!!

		// construct the proper class
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("TYPE"))
			{
				if(FString(attr[i + 1]) == FString("VECTOR"))
				{
					NewVariable(Constant::VECTOR);
					break;
				}
				else if(FString(attr[i + 1]) == FString("MATRIX"))
				{
					NewVariable(Constant::MATRIX);
					break;
				}
				else if(FString(attr[i + 1]) == FString("FLOAT"))
				{
					NewVariable(Constant::FLOATX);
					break;
				}
				else if(FString(attr[i + 1]) == FString("INT"))
				{
					NewVariable(Constant::INTX);
					break;
				}
				else
				{
					// unknown type or wrong block, bail
					return;
				}
			}
		}


		// setup the new contant
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("DESC"))
			{
				CurrentConstant->Name = FString(attr[i + 1]);
			}
		}

		// these two types have the values right in the tag.
		if(CurrentConstant->Type == Constant::INTX || CurrentConstant->Type == Constant::FLOATX)
		{
			for (INT i = 0; attr[i]; i += 2) 
			{
				if(FString(attr[i]) == FString("CURRENT"))
				{
					FLOAT Val = atof(attr[i+1]);
					CurrentConstant->Values[0] = Val;
					CurrentConstant->Values[1] = Val;
					CurrentConstant->Values[2] = Val;
					CurrentConstant->Values[3] = Val;
				}
			}
		}
	}

	// looks in a shader and in the global constants for a constant
	// with the specified name and returns it
	Constant* FindConstant(Shader* LocalShader, const char* Name)
	{
		for(INT i = 0; i < 4; i++)
		{
			for(INT i = 0; i < LocalShader->NumConstants; i++)
			{
				if(LocalShader->Constants[i].Name == Name)
					return &LocalShader->Constants[i];
			}
		}

		for(INT i = 0; i < NumConstants; i++)
		{
			if(AllConstants[i].Name == Name)
				return &AllConstants[i];
		}

		return NULL;
	}


	void HandleConstant(const char** attr)
	{
		// Warning: function should return in the middle!

		// Find the DESC memeber inside of the constant
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("DESC"))
			{
				// See if we have a constant with that name
				Constant* NewConst = FindConstant(CurrentShader, attr[i + 1]);
				if(NewConst != NULL)
				{
					INT temp = *(CurrentShader->CurrentPass->CurrentMappingArrayCounter);
					CurrentShader->CurrentPass->CurrentMappingArray[temp].MyConstant = NewConst;
					RegisterValue = &(CurrentShader->CurrentPass->CurrentMappingArray[*(CurrentShader->CurrentPass->CurrentMappingArrayCounter)].Register); // this is getting deep and ugly, so we just save the register value and we'll stuff it into the proper place when we hit the handler
					(*(CurrentShader->CurrentPass->CurrentMappingArrayCounter))++;
				}
				else
				{
					debugf(TEXT("Constant %s not found in shader file"), attr[i+1]);
				}
				return;
			}
		}

		// constant without a DESC
		check(0);
	}


	void HandleRegister(const char** attr)
	{
		// find the actual 
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				*RegisterValue = atoi(attr[i+1]);
				return;
			}
		}

		check(0); // REGISTER without CURRENT
	}


	void HandleTexture(const char** attr)
	{
		// find the actual 
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("DESC"))
			{
				Textures[NumTextures].Name = attr[i+1];
				CurrentTexture = &Textures[NumTextures];
				NumTextures++;
			}
		}
	}	

	void HandleTextureUnit(const char** attr)
	{
		for (INT i = 0; attr[i]; i += 2) 
		{
			if(FString(attr[i]) == FString("CURRENT"))
			{
				CurrentShader->CurrentPass->CurrentTextureUnit = atoi(attr[i+1]);
				return;
			}
		}
	}

	void start(const XML_Char *el, const XML_Char **attr)
	{
		/*INT i;

		for (i = 0; i < Depth; i++)
			debugf(TEXT("  "));

		debugf(TEXT("%s"), el);

		for (INT i = 0; attr[i]; i += 2) 
		{
			debugf(TEXT(" %s='%s'"), attr[i], attr[i + 1]);
		}

		debugf(TEXT("\n"));*/
		Depth++;

		if(FString(el) == FString("VARIABLE"))
		{
			HandleVariable(attr);
			VarIsColor=false;
		}
		else if(FString(el) == FString("COLOR"))
		{
			HandleVariable(attr);
			VarIsColor=true;
		}
		else if(FString(el) == FString("ELEMENT"))
		{
			HandleVariableElement(attr);
		}
		else if(FString(el) == FString("EFFECT"))
		{
			HandleEffect(attr);
		}
		else if(FString(el) == FString("PIXEL_SHADER"))
		{
			HandlePixelShader(attr);
		}
		else if(FString(el) == FString("VERTEX_SHADER"))
		{
			HandleVertexShader(attr);
		}
		else if(FString(el) == FString("CONSTANT"))
		{
			HandleConstant(attr);
		}
		else if(FString(el) == FString("REGISTER"))
		{
			HandleRegister(attr);
		}
		else if(FString(el) == FString("STREAM_MAP"))
		{
			HandleStreamMapping(attr);
		}
		else if(FString(el) == FString("CHANNEL_TYPE"))
		{
			HandleStreamChannelType(attr);
		}
		else if(FString(el) == FString("CHANNEL_USAGE"))
		{
			HandleStreamChannelUsage(attr);
		}
		else if(FString(el) == FString("CHANNEL_REG"))
		{
			HandleStreamChannelReg(attr);
		}
		else if(FString(el) == FString("STREAM_CHANNEL"))
		{
			HandleStreamChannel(attr);
		}
		else if(FString(el) == FString("STREAM_MAP_REF"))
		{
			InStreamMapRef = true;
		}
		else if(FString(el) == FString("TEXTURE_REF"))
		{
			InTextureRef = true;
		}
		else if (FString(el) == FString("TEXTURE"))
		{
			HandleTexture(attr);
		}
		else if(FString(el) == FString("TEXTURE_UNIT"))
		{
			HandleTextureUnit(attr);
		}
		else if(FString(el) == FString("PASS"))
		{
			HandlePass(attr);
		}
		else if(FString(el) == FString("VALUE"))
		{
			HandleValue(attr);
		}
		else if(FString(el) == FString("STATE"))
		{
			HandleState(attr);
		}
		else if(FString(el) == FString("RENDER_STATE_BLOCK"))
		{
			InRenderState = true;
		}
	}



	void end(const char *el)
	{
		if(FString(el) == FString("VARIABLE") || FString(el) == FString("COLOR"))
		{
			if(VarIsColor)
			{
				// swap R and B
	/*			FLOAT Temp = CurrentConstant->Values[2];
				CurrentConstant->Values[2] = CurrentConstant->Values[0];
				CurrentConstant->Values[0] = Temp;*/
			}
			
			CurrentConstant = NULL;
			CurrentConstMember = -1;
		}
		else if(FString(el) == FString("EFFECT"))
		{
			CurrentShader = NULL;
		}
		else if(FString(el) == FString("VERTEX_SHADER"))
		{
			CurrentCode = NULL;
			CurrentShader->CurrentPass->CurrentMappingArray = NULL;
			CurrentShader->CurrentPass->CurrentMappingArrayCounter = NULL;
		}
		else if(FString(el) == FString("PIXEL_SHADER"))
		{
			CurrentCode = NULL;
			CurrentShader->CurrentPass->CurrentMappingArray = NULL;
			CurrentShader->CurrentPass->CurrentMappingArrayCounter = NULL;
		}
		else if(FString(el) == FString("STREAM_MAP"))
		{
			CurrentMapping = NULL;
		}
		else if(FString(el) == FString("STREAM_MAP_REF"))
		{
			InStreamMapRef = false;
		}
		else if(FString(el) == FString("TEXTURE_REF"))
		{
			InTextureRef = false;
		}
		else if(FString(el) == FString("TEXTURE"))
		{
			CurrentTexture = NULL;
		}
		else if(FString(el) == FString("PASS"))
		{
			CurrentShader->CurrentPass = NULL;
		}
		else if(FString(el) == FString("RENDER_STATE_BLOCK"))
		{
			InRenderState = false;
		}
	}

	static void StaticStart(void *FakeThis, const XML_Char *el, const XML_Char **attr)
	{
		((FShaderLoader*)FakeThis)->start(el, attr);
	}

	static void StaticEnd(void *FakeThis, const XML_Char *el)
	{
		((FShaderLoader*)FakeThis)->end(el);
	}

	StreamMapping* FindMapping(FString MappingName)
	{
		for(INT i = 0; i < NumMappings; i++)
		{
			if(AllMappings[i].Name == MappingName)
				return &AllMappings[i];
		}
		return NULL;
	}



	void cdata(const XML_Char* s, INT len)
	{
		// incoming string isn't null-terminated so we construct a new one
		check(len < 8192);
		char NewStringA[8192];
		appMemcpy(NewStringA, s, len);
		NewStringA[len] = '\0';

		const TCHAR* NewString = appFromAnsi(NewStringA);

		if(CurrentCode)
		{
			*CurrentCode += FString(NewString);
		}
		else if(CurrentShader && InStreamMapRef)
		{
			CurrentShader->CurrentPass->MyMapping = FindMapping(FString(NewString));
			check(CurrentShader->CurrentPass->MyMapping);
		}
		else if(InTextureRef)
		{
			check(CurrentShader->CurrentPass->CurrentTextureUnit >= 0 && CurrentShader->CurrentPass->CurrentTextureUnit < 8);
			
			CurrentShader->CurrentPass->Textures[CurrentShader->CurrentPass->CurrentTextureUnit] = FindTextureRef(FString(NewString));
		}
		else if(CurrentTexture)
		{
			CurrentTexture->Filename = FString(NewString);
		}
	}

	void static StaticCdata(void* FakeThis, const XML_Char* s, INT len)
	{
		((FShaderLoader*)FakeThis)->cdata(s, len);
	}


	void CloseXML()
	{
		XML_ParserFree(p);
	}



	void OpenXML()
	{
		p = XML_ParserCreate(NULL);
	}



	void Process(FString Filename)
	{
		FString Data;
		if( !appLoadFileToString( Data, *Filename))
		{
			GWarn->Logf( NAME_Error, TEXT("Could not open %s"), *Filename );
			return;
		}

		XML_SetElementHandler(p, StaticStart, StaticEnd);

		XML_SetCharacterDataHandler(p, StaticCdata);

		XML_SetUserData(p, this);

		INT CurrentCharacter = 0;

		Textures[NumTextures].Filename = TEXT(".\\normalizer.tga");
		Textures[NumTextures].Name = TEXT("normalizer");
		NumTextures++;

		// for some reason it doesn't like getting all the data at once, so we spoon-feed it a few characters at a time
		INT Size = Data.Len();
		const char* ParseData = TCHAR_TO_ANSI(*(Data.Mid(CurrentCharacter, Size)));
		CurrentCharacter+=Size;

		INT done = 0;
		if (XML_Parse(p, ParseData, Size, done) == XML_STATUS_ERROR) 
		{
			debugf(TEXT("Parse error at line %d"),
					XML_GetCurrentLineNumber(p));
		}
	}

	void PostProcess()
	{
		// Fill in the mappings that are missing. Un-fortunately RenderMonkey doesn't demand that it's XML docs are formed consistently.
		StreamMapping* PreviousMapping = NULL;

		for(INT ShaderIndex = 0; ShaderIndex < NumShaders; ShaderIndex++)
		{
			for(INT PassIndex = 0; PassIndex < Shaders[ShaderIndex].NumPasses; PassIndex++)
			{
				if(!Shaders[ShaderIndex].Passes[PassIndex].MyMapping)
				{
					Shaders[ShaderIndex].Passes[PassIndex].MyMapping = PreviousMapping;
				}

				PreviousMapping = Shaders[ShaderIndex].Passes[PassIndex].MyMapping;
			}
		}

		// go through the rendering states and fill in the handy values.
		for(INT ShaderIndex = 0; ShaderIndex < NumShaders; ShaderIndex++)
		{
			for(INT PassIndex = 0; PassIndex < Shaders[ShaderIndex].NumPasses; PassIndex++)
			{
				// post-processed useful render-state values.
				Shader::Pass* CurrentPass = &(Shaders[ShaderIndex].Passes[PassIndex]);

				CurrentPass->AlphaBlending = (UBOOL)CurrentPass->RenderStates[27];
				CurrentPass->AlphaTest = (UBOOL)CurrentPass->RenderStates[15];
				CurrentPass->AlphaRef = (UBOOL)CurrentPass->RenderStates[24];
				CurrentPass->ZTest = (UBOOL)CurrentPass->RenderStates[7];
				CurrentPass->ZWrite = (UBOOL)CurrentPass->RenderStates[14];
				CurrentPass->SrcBlend = CurrentPass->RenderStates[19];
				CurrentPass->DestBlend = CurrentPass->RenderStates[20];
			}
		}

		// Do some sanity checks
		for(INT ShaderIndex = 0; ShaderIndex < NumShaders; ShaderIndex++)
		{
			for(INT PassIndex = 0; PassIndex < Shaders[ShaderIndex].NumPasses; PassIndex++)
			{
				check((Shaders[ShaderIndex].Passes[PassIndex].VertexShaderText.Len() > 0) || 
					(Shaders[ShaderIndex].Passes[PassIndex].PixelShaderText.Len() > 0));
			}
		}
	}

	FShaderLoader(FString Filename) :
		Depth(0), NumMappings(0), CurrentMapping(NULL), NumConstants(0), CurrentConstant(NULL),
		CurrentConstMember(0), RegisterValue(NULL), InStreamMapRef(false), NumShaders(0), CurrentShader(NULL),
		CurrentCode(NULL), InTextureRef(false), NumTextures(0), CurrentTexture(NULL), PrevPass(NULL), InRenderState(false)
	{
		OpenXML();
		Process(Filename);	
		CloseXML();
		PostProcess();
	}

};


